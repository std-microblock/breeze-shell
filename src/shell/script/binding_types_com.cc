#include "../utils.h"
#include "binding_types.hpp"

#include "../contextmenu/menu_render.h"
#include "../entry.h"

#include <algorithm>
#include <atlcomcli.h>
#include <filesystem>
#include <iostream>
#include <memory>
#include <optional>
#include <print>
#include <ranges>

// Windows Headers
#include <atlalloc.h>
#include <atlbase.h>
#include <exdisp.h>
#include <shellapi.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <shobjidl_core.h>
#include <stdio.h>

#include "../logger.h"

#include <thread>
#include <unordered_map>
#include <windows.h>

#include <psapi.h>
#include <winuser.h>

#include "propkey.h"
#include "ui.h"

std::string folder_id_to_path(PIDLIST_ABSOLUTE pidl) {
  wchar_t *path = new wchar_t[MAX_PATH];
  SHGetPathFromIDListW(pidl, path);
  std::wstring wpath = path;
  delete[] path;
  return mb_shell::wstring_to_utf8(std::wstring(wpath.begin(), wpath.end()));
}

PIDLIST_ABSOLUTE path_to_folder_id(std::string path) {
  std::wstring wpath = mb_shell::utf8_to_wstring(path);
  PIDLIST_ABSOLUTE pidl;
  if (SUCCEEDED(
          SHParseDisplayName(wpath.c_str(), nullptr, &pidl, 0, nullptr))) {
    return pidl;
  }
  return nullptr;
}

std::optional<std::pair<HWND, CComPtr<IShellBrowser>>>
hwnd_from_disp(CComPtr<IDispatch> spdisp) {
  if (!spdisp)
    return {};
  CComPtr<IServiceProvider> spServiceProvider;

  if (FAILED(spdisp->QueryInterface(IID_PPV_ARGS(&spServiceProvider))))
    return {};

  CComPtr<IShellBrowser> spBrowser;
  if (FAILED(spServiceProvider->QueryService(SID_STopLevelBrowser,
                                             IID_PPV_ARGS(&spBrowser))))
    return {};

  if (!spBrowser)
    return {};

  HWND hwnd;
  if (FAILED(spBrowser->GetWindow(&hwnd)))
    return {};

  return std::make_pair(hwnd, spBrowser);
}

std::vector<std::tuple<HWND, CComPtr<IShellBrowser>>> GetIShellBrowsers() {
  std::vector<std::tuple<HWND, CComPtr<IShellBrowser>>> res;
  CComPtr<IShellWindows> spShellWindows;
  spShellWindows.CoCreateInstance(CLSID_ShellWindows);

  if (!spShellWindows) return {};
  CComVariant vtLoc(CSIDL_DESKTOP);
  CComVariant vtEmpty;

  // iterate through all shell windows
  CComPtr<IDispatch> spdisp;

  long cnt = 0;
  spShellWindows->get_Count(&cnt);
  for (long i = 0; i < cnt; i++) {
    CComVariant index(i);
    spShellWindows->Item(index, &spdisp);
    if (auto r = hwnd_from_disp(spdisp))
      res.push_back(*r);
  }

  return res;
}

auto GetDesktopIShellBrowser() {
  auto spShellWindows = CComPtr<IShellWindows>();
  spShellWindows.CoCreateInstance(CLSID_ShellWindows);

  if (!spShellWindows)
    return std::optional<std::pair<HWND, CComPtr<IShellBrowser>>>();
  CComVariant vtLoc(CSIDL_DESKTOP);
  CComVariant vtEmpty;

  long lhwnd;
  CComPtr<IDispatch> spdisp;
  spShellWindows->FindWindowSW(&vtLoc, &vtEmpty, SWC_DESKTOP, &lhwnd,
                               SWFO_NEEDDISPATCH, &spdisp);

  return hwnd_from_disp(spdisp);
}

CComPtr<IShellBrowser> GetIShellBrowserRecursive(HWND hWnd) {
  if (!hWnd)
    return nullptr;

  std::unordered_set<HWND> recorded_hwnds;

  static auto browsers = GetIShellBrowsers();
  auto GetIShellBrowser = [&](HWND hwnd) -> CComPtr<IShellBrowser> {
    for (auto &[h, b] : browsers) {
      CComPtr<IShellView> psv;
      if (h == hwnd && SUCCEEDED(b->QueryActiveShellView(&psv))) {
        mb_shell::dbgout("Found: {} {}", (void *)h, (void *)b.p);
        return b;
      }
    }
    return nullptr;
  };

  auto dfs = [&](auto &self, HWND hwnd) -> CComPtr<IShellBrowser> {
    auto rrecorded_hwnds = recorded_hwnds;
    if (!hwnd || rrecorded_hwnds.count(hwnd))
      return nullptr;
    recorded_hwnds.insert(hwnd);

    if (CComPtr<IShellBrowser> psb = GetIShellBrowser(hwnd))
      return psb;

    // iterate through child windows
    for (HWND hwnd = GetWindow(hWnd, GW_CHILD); hwnd != NULL;
         hwnd = GetWindow(hwnd, GW_HWNDNEXT)) {
      CComPtr<IShellBrowser> psb = self(self, hwnd);
      if (psb)
        return psb;
    }
    // iterate through parent windows
    for (HWND hwnd = GetParent(hWnd); hwnd; hwnd = GetParent(hwnd)) {
      CComPtr<IShellBrowser> psb = self(self, hwnd);
      if (psb)
        return psb;
    }
    return nullptr;
  };

  auto res = dfs(dfs, hWnd);

  if (!res) {
    // first, we rescan all shell windows
    browsers = GetIShellBrowsers();
    res = dfs(dfs, hWnd);
    if (res)
      return res;

    // if still no result and the window is shell window, we assume it's the
    // desktop
    if (auto res = GetDesktopIShellBrowser())
      return res->second;
  }

  return res;
}

namespace mb_shell::js {

js_menu_context js_menu_context::$from_window(void *_hwnd) {
  js_menu_context event_data;
  perf_counter perf("js_menu_context::$from_window");
  HWND hWnd = reinterpret_cast<HWND>(_hwnd);

  // Check if the foreground window is an edit control
  char className[256];
  auto activeWindowHandle = GetForegroundWindow();
  auto activeWindowThread =
      GetWindowThreadProcessId(activeWindowHandle, nullptr);
  auto thisWindowThread = GetCurrentThreadId();
  AttachThreadInput(activeWindowThread, thisWindowThread, true);
  auto focused_hwnd = GetFocus();
  AttachThreadInput(activeWindowThread, thisWindowThread, false);
  if (GetClassNameA(focused_hwnd, className, sizeof(className))) {
    std::string class_name = className;
    std::printf("Focused window class name: %s\n", class_name.c_str());

    if (class_name == "Edit") {
      std::printf("Focused window is an edit control\n");
      event_data.input_box = std::make_shared<input_box_controller>();
      (*event_data.input_box)->$hwnd = focused_hwnd;
      return event_data;
    }
  }

  perf.end("Edit");

  __try {
    if (GetClassNameA(hWnd, className, sizeof(className))) {
      std::string class_name = className;
      if (class_name == "SysListView32" || class_name == "DirectUIHWND" ||
          class_name == "SHELLDLL_DefView" || class_name == "CabinetWClass") {
        dbgout("Target window is a folder view (hwnd: {})\n", (void *)hWnd);
        // Check if the foreground window is an Explorer window

        if (IShellBrowser *psb = GetIShellBrowserRecursive(hWnd)) {
          IShellView *psv;
          perf.end("IShellBrowser - GetIShellBrowserRecursive");
          dbgout("shell browser: {}\n", (void *)psb);
          if (SUCCEEDED(psb->QueryActiveShellView(&psv))) {
            IFolderView *pfv;
            if (SUCCEEDED(
                    psv->QueryInterface(IID_IFolderView, (void **)&pfv))) {
              // It's an Explorer window
              event_data.folder_view =
                  std::make_shared<folder_view_controller>();
              auto fv = *event_data.folder_view;
              fv->$hwnd = hWnd;
              fv->$controller = psb;
              fv->$render_target = menu_render::current.value()->rt.get();

              int focusIndex;
              pfv->GetFocusedItem(&focusIndex);
              if (focusIndex >= 0) {
                PIDLIST_ABSOLUTE pidl;
                if (SUCCEEDED(pfv->Item(focusIndex, &pidl))) {
                  fv->focused_file_path = folder_id_to_path(pidl);
                  CoTaskMemFree(pidl);
                }
              }

              IShellItem *psi;
              if (SUCCEEDED(pfv->GetFolder(IID_IShellItem, (void **)&psi))) {
                PWSTR pszPath;
                if (SUCCEEDED(
                        psi->GetDisplayName(SIGDN_FILESYSPATH, &pszPath))) {
                  fv->current_path = mb_shell::wstring_to_utf8(pszPath);
                  CoTaskMemFree(pszPath);
                }
                psi->Release();
              }

              IShellItemArray *psia;
              if (SUCCEEDED(pfv->Items(SVGIO_SELECTION, IID_IShellItemArray,
                                       (void **)&psia))) {
                DWORD count;
                if (SUCCEEDED(psia->GetCount(&count))) {
                  for (DWORD i = 0; i < count; i++) {
                    IShellItem *psi;
                    if (SUCCEEDED(psia->GetItemAt(i, &psi))) {
                      PWSTR pszPath;
                      if (SUCCEEDED(psi->GetDisplayName(SIGDN_FILESYSPATH,
                                                        &pszPath))) {
                        fv->selected_files.push_back(
                            mb_shell::wstring_to_utf8(pszPath));
                        CoTaskMemFree(pszPath);
                      }
                      psi->Release();
                    }
                  }
                }
                psia->Release();
              }

              pfv->Release();
            }
            psv->Release();
          }
        } else {
          dbgout("Failed to get IShellBrowser");
        }
      }
    }

  } __except (EXCEPTION_CONTINUE_EXECUTION) {
    dbgout("Get IShellBrowser crashed!");
  }
  perf.end("IShellBrowser");

  if (hWnd) {
    // get window position
    RECT rect;
    GetWindowRect(hWnd, &rect);
    caller_window_data window_info;
    window_info.x = rect.left;
    window_info.y = rect.top;
    window_info.width = rect.right - rect.left;
    window_info.height = rect.bottom - rect.top;
    // get window title
    wchar_t title[256];
    GetWindowTextW(hWnd, title, sizeof(title));
    window_info.title = mb_shell::wstring_to_utf8(title);

    // get executable path
    DWORD pid;
    GetWindowThreadProcessId(hWnd, &pid);
    HANDLE hProcess =
        OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);

    if (hProcess) {
      wchar_t path[MAX_PATH];
      if (GetModuleFileNameExW(hProcess, NULL, path, MAX_PATH)) {
        window_info.executable_path = mb_shell::wstring_to_utf8(path);
      }
      CloseHandle(hProcess);
    }

    // get props
    // EnumProps
    // static std::unordered_map<std::string, size_t> prop_map;
    // prop_map = {};
    // EnumPropsW(hWnd, [](HWND hWnd, auto lpszString, HANDLE hData) -> BOOL {
    //   if (is_memory_readable(lpszString))
    //     prop_map[mb_shell::wstring_to_utf8(lpszString)] = (size_t)hData;
    //   else
    //     prop_map[std::to_string((size_t)lpszString)] = (size_t)hData;
    //   return TRUE;
    // });

    // std::ranges::copy(prop_map | std::ranges::views::transform([](auto &pair)
    // {
    //                     return window_prop_data{pair.first, pair.second};
    //                   }),
    //                   std::back_inserter(window_info.props));

    event_data.window_info = window_info;
  }

  perf.end("Window");
  // event_data.window_titlebar =
  // std::make_shared<window_titlebar_controller>();
  // (*event_data.window_titlebar)->$hwnd = hWnd;

  return event_data;
}

void folder_view_controller::change_folder(std::string new_folder_path) {
  IShellBrowser *browser = static_cast<IShellBrowser *>($controller);
  std::wstring wpath = mb_shell::utf8_to_wstring(new_folder_path);

  PIDLIST_ABSOLUTE pidl;
  if (SUCCEEDED(
          SHParseDisplayName(wpath.c_str(), nullptr, &pidl, 0, nullptr))) {
    browser->BrowseObject(pidl, SBSP_ABSOLUTE);
    CoTaskMemFree(pidl);
  }
}

void folder_view_controller::open_file(std::string file_path) {
  ShellExecuteW(NULL, L"open", mb_shell::utf8_to_wstring(file_path).c_str(),
                NULL, NULL, SW_SHOW);
}

void folder_view_controller::open_folder(std::string folder_path) {
  change_folder(folder_path);
}

void folder_view_controller::refresh() { change_folder(current_path); }

std::vector<std::shared_ptr<folder_view_folder_item>>
folder_view_controller::items() {
  std::vector<std::shared_ptr<folder_view_folder_item>> items;
  IShellBrowser *browser = static_cast<IShellBrowser *>($controller);
  IShellView *view;

  if (SUCCEEDED(browser->QueryActiveShellView(&view))) {
    IFolderView *folder_view;
    if (SUCCEEDED(
            view->QueryInterface(IID_IFolderView, (void **)&folder_view))) {
      int item_count;
      if (SUCCEEDED(folder_view->ItemCount(SVGIO_ALLVIEW, &item_count))) {
        for (int i = 0; i < item_count; i++) {
          PIDLIST_ABSOLUTE pidl;
          if (SUCCEEDED(folder_view->Item(i, &pidl))) {
            auto item = std::make_shared<folder_view_folder_item>();
            item->$handler = pidl;
            item->$controller = view;
            item->index = i;
            item->parent_path = current_path;
            item->$render_target = $render_target;
            items.push_back(item);
          }
        }
      }

      folder_view->Release();
    }
    view->Release();
  }

  return items;
}

void folder_view_controller::select(int index, int state) {
  IShellBrowser *browser = static_cast<IShellBrowser *>($controller);

  entry::main_window_loop_hook.add_task([=]() {
    IShellView *view;
    if (SUCCEEDED(browser->QueryActiveShellView(&view))) {
      IFolderView *folder_view;
      if (SUCCEEDED(
              view->QueryInterface(IID_IFolderView, (void **)&folder_view))) {
        folder_view->SelectItem(index, state);
      }
    }
  });
}

void folder_view_controller::select_none() {
  IShellBrowser *browser = static_cast<IShellBrowser *>($controller);
  entry::main_window_loop_hook.add_task([=]() {
    IShellView *view;
    if (SUCCEEDED(browser->QueryActiveShellView(&view))) {
      view->SelectItem(nullptr, SVSI_DESELECTOTHERS);
      view->Release();
    }
  });
}

std::string folder_view_folder_item::name() {
  PIDLIST_ABSOLUTE pidl = static_cast<PIDLIST_ABSOLUTE>($handler);

  IShellItem *item;
  if (SUCCEEDED(SHCreateItemFromIDList(pidl, IID_IShellItem, (void **)&item))) {
    LPWSTR name;
    if (SUCCEEDED(item->GetDisplayName(SIGDN_NORMALDISPLAY, &name))) {
      std::wstring wname(name);
      CoTaskMemFree(name);
      item->Release();
      return mb_shell::wstring_to_utf8(wname);
    }
    item->Release();
  }
  return "";
}
std::string folder_view_folder_item::modify_date() {
  PIDLIST_ABSOLUTE pidl = static_cast<PIDLIST_ABSOLUTE>($handler);
  IShellItem *item;
  if (SUCCEEDED(SHCreateItemFromIDList(pidl, IID_IShellItem, (void **)&item))) {
    IShellItem2 *item2;
    if (SUCCEEDED(item->QueryInterface(IID_IShellItem2, (void **)&item2))) {
      FILETIME ft;
      if (SUCCEEDED(item2->GetFileTime(PKEY_DateModified, &ft))) {
        SYSTEMTIME st;
        FileTimeToSystemTime(&ft, &st);
        wchar_t date[100];
        GetDateFormatW(LOCALE_USER_DEFAULT, DATE_SHORTDATE, &st, NULL, date,
                       100);
        item2->Release();
        item->Release();
        return mb_shell::wstring_to_utf8(date);
      }
      item2->Release();
    }
    item->Release();
  }
  return "";
}

std::string folder_view_folder_item::path() {
  PIDLIST_ABSOLUTE pidl = static_cast<PIDLIST_ABSOLUTE>($handler);

  CComPtr<IShellItem> item;
  if (SUCCEEDED(SHCreateItemFromIDList(pidl, IID_IShellItem, (void **)&item))) {
    LPWSTR path;
    if (SUCCEEDED(item->GetDisplayName(SIGDN_NORMALDISPLAY, &path))) {
      std::wstring name(path);
      CoTaskMemFree(path);
      std::filesystem::path p(parent_path);
      p /= name;
      return p.string();
    }
  }
  return "";
}

size_t folder_view_folder_item::size() {
  PIDLIST_ABSOLUTE pidl = static_cast<PIDLIST_ABSOLUTE>($handler);
  IShellItem *item;
  if (SUCCEEDED(SHCreateItemFromIDList(pidl, IID_IShellItem, (void **)&item))) {
    IShellItem2 *item2;
    if (SUCCEEDED(item->QueryInterface(IID_IShellItem2, (void **)&item2))) {
      ULONGLONG size;
      if (SUCCEEDED(item2->GetUInt64(PKEY_Size, &size))) {
        item2->Release();
        item->Release();
        return static_cast<size_t>(size);
      }
      item2->Release();
    }
    item->Release();
  }
  return 0;
}

std::string folder_view_folder_item::type() {
  PIDLIST_ABSOLUTE pidl = static_cast<PIDLIST_ABSOLUTE>($handler);
  IShellItem *item;
  if (SUCCEEDED(SHCreateItemFromIDList(pidl, IID_IShellItem, (void **)&item))) {
    LPWSTR type;
    if (SUCCEEDED(item->GetDisplayName(SIGDN_FILESYSPATH, &type))) {
      wchar_t fileType[MAX_PATH];
      SHFILEINFOW sfi = {0};
      if (SHGetFileInfoW(type, 0, &sfi, sizeof(sfi), SHGFI_TYPENAME)) {
        CoTaskMemFree(type);
        item->Release();
        return mb_shell::wstring_to_utf8(sfi.szTypeName);
      }
      CoTaskMemFree(type);
    }
    item->Release();
  }
  return "";
}

void folder_view_folder_item::select(int state) {
  CComPtr<IShellView> sv = static_cast<IShellView *>($controller);
  entry::main_window_loop_hook.add_task(
      [=, h = $handler]() { sv->SelectItem((PCUITEMID_CHILD)h, state); });
}

constexpr UINT ID_EDIT_COPY = 0x0001;
constexpr UINT ID_EDIT_CUT = 0x0002;
constexpr UINT ID_EDIT_PASTE = 0x0003;

void folder_view_controller::copy() {
  SendMessageW((HWND)$hwnd, WM_COMMAND, MAKEWPARAM(ID_EDIT_COPY, 0), 0);
}

void folder_view_controller::cut() {
  SendMessageW((HWND)$hwnd, WM_COMMAND, MAKEWPARAM(ID_EDIT_CUT, 0), 0);
}

void folder_view_controller::paste() {
  SendMessageW((HWND)$hwnd, WM_COMMAND, MAKEWPARAM(ID_EDIT_PASTE, 0), 0);
}

void window_titlebar_controller::set_title(std::string new_title) {
  SetWindowTextW((HWND)$hwnd, mb_shell::utf8_to_wstring(new_title).c_str());
}

void window_titlebar_controller::set_icon(std::string icon_path) {
  HICON hIcon =
      (HICON)LoadImageW(NULL, mb_shell::utf8_to_wstring(icon_path).c_str(),
                        IMAGE_ICON, 0, 0, LR_LOADFROMFILE);
  if (hIcon) {
    SendMessageW((HWND)$hwnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
    SendMessageW((HWND)$hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
  }
}

void window_titlebar_controller::set_position(int new_x, int new_y) {
  SetWindowPos((HWND)$hwnd, NULL, new_x, new_y, 0, 0,
               SWP_NOSIZE | SWP_NOZORDER);
}

void window_titlebar_controller::set_size(int new_width, int new_height) {
  SetWindowPos((HWND)$hwnd, NULL, 0, 0, new_width, new_height,
               SWP_NOMOVE | SWP_NOZORDER);
}

void window_titlebar_controller::maximize() {
  ShowWindow((HWND)$hwnd, SW_MAXIMIZE);
}

void window_titlebar_controller::minimize() {
  ShowWindow((HWND)$hwnd, SW_MINIMIZE);
}

void window_titlebar_controller::restore() {
  ShowWindow((HWND)$hwnd, SW_RESTORE);
}

void window_titlebar_controller::close() {
  SendMessageW((HWND)$hwnd, WM_CLOSE, 0, 0);
}

void window_titlebar_controller::focus() { SetForegroundWindow((HWND)$hwnd); }

void window_titlebar_controller::show() { ShowWindow((HWND)$hwnd, SW_SHOW); }

void window_titlebar_controller::hide() { ShowWindow((HWND)$hwnd, SW_HIDE); }

void input_box_controller::set_text(std::string new_text) {
  SetWindowTextW((HWND)$hwnd, mb_shell::utf8_to_wstring(new_text).c_str());
}

void input_box_controller::set_placeholder(std::string new_placeholder) {
  SendMessageW((HWND)$hwnd, EM_SETCUEBANNER, TRUE,
               (LPARAM)mb_shell::utf8_to_wstring(new_placeholder).c_str());
}

void input_box_controller::set_position(int new_x, int new_y) {
  SetWindowPos((HWND)$hwnd, NULL, new_x, new_y, 0, 0,
               SWP_NOSIZE | SWP_NOZORDER);
}

void input_box_controller::set_size(int new_width, int new_height) {
  SetWindowPos((HWND)$hwnd, NULL, 0, 0, new_width, new_height,
               SWP_NOMOVE | SWP_NOZORDER);
}

void input_box_controller::set_multiline(bool new_multiline) {
  LONG style = GetWindowLongW((HWND)$hwnd, GWL_STYLE);
  if (new_multiline) {
    style |= ES_MULTILINE;
  } else {
    style &= ~ES_MULTILINE;
  }
  SetWindowLongW((HWND)$hwnd, GWL_STYLE, style);
}

void input_box_controller::set_password(bool new_password) {
  LONG style = GetWindowLongW((HWND)$hwnd, GWL_STYLE);
  if (new_password) {
    style |= ES_PASSWORD;
  } else {
    style &= ~ES_PASSWORD;
  }
  SetWindowLongW((HWND)$hwnd, GWL_STYLE, style);
}

void input_box_controller::set_readonly(bool new_readonly) {
  SendMessageW((HWND)$hwnd, EM_SETREADONLY, new_readonly ? TRUE : FALSE, 0);
}

void input_box_controller::set_disabled(bool new_disabled) {
  EnableWindow((HWND)$hwnd, !new_disabled);
}

void input_box_controller::focus() { SetFocus((HWND)$hwnd); }

void input_box_controller::blur() { SetFocus(NULL); }

void input_box_controller::select_all() {
  SendMessageW((HWND)$hwnd, EM_SETSEL, 0, -1);
}

void input_box_controller::select_range(int start, int end) {
  SendMessageW((HWND)$hwnd, EM_SETSEL, start, end);
}

void input_box_controller::set_selection(int start, int end) {
  SendMessageW((HWND)$hwnd, EM_SETSEL, start, end);
}

void input_box_controller::insert_text(std::string new_text) {
  SendMessageW((HWND)$hwnd, EM_REPLACESEL, TRUE,
               (LPARAM)mb_shell::utf8_to_wstring(new_text).c_str());
}

void input_box_controller::delete_text(int start, int end) {
  SendMessageW((HWND)$hwnd, EM_SETSEL, start, end);
  SendMessageW((HWND)$hwnd, EM_REPLACESEL, TRUE, (LPARAM)L"");
}

void input_box_controller::clear() { SetWindowTextW((HWND)$hwnd, L""); }
} // namespace mb_shell::js
