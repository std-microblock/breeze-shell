#include "../utils.h"
#include "binding_types.h"

#include <filesystem>
#include <iostream>
#include <print>

#include "shlobj_core.h"
#include <memory>
#include <oleacc.h>
#include <optional>
#include <shobjidl_core.h>
#include <windows.h>
#include <winuser.h>

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

IShellBrowser *GetIShellBrowser(HWND hWnd) {
  __try {
    auto res = (IShellBrowser *)::SendMessageW(hWnd, WM_USER + 7, 0, 0);
    if (!res) {
      return nullptr;
    }
    res->AddRef();
    res->Release();
    return res;
  } __except (EXCEPTION_EXECUTE_HANDLER) {
    return nullptr;
  }
}

IShellBrowser *GetIShellBrowserRecursive(HWND hWnd) {
  if (IShellBrowser *psb = GetIShellBrowser(hWnd)) {
    return psb;
  }

  return GetIShellBrowserRecursive(GetParent(hWnd));
}

namespace mb_shell::js {

js_menu_context js_menu_context::$from_window(void *_hwnd) {
  js_menu_context event_data;

  HWND hWnd = reinterpret_cast<HWND>(_hwnd);

  // Check if the foreground window is an edit control
  char className[256];
  auto focused_hwnd = GetFocus();
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

  CoInitializeEx(NULL, COINIT_MULTITHREADED);
  // Check if the foreground window is an Explorer window

  if (IShellBrowser *psb = GetIShellBrowserRecursive(hWnd)) {
    IShellView *psv;
    std::printf("shell browser: %p\n", psb);
    if (SUCCEEDED(psb->QueryActiveShellView(&psv))) {
      IFolderView *pfv;
      if (SUCCEEDED(psv->QueryInterface(IID_IFolderView, (void **)&pfv))) {
        // It's an Explorer window
        event_data.folder_view = std::make_shared<folder_view_controller>();
        auto fv = *event_data.folder_view;
        fv->$hwnd = hWnd;
        fv->$controller = psb;

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
          if (SUCCEEDED(psi->GetDisplayName(SIGDN_FILESYSPATH, &pszPath))) {
            fv->current_path = mb_shell::wstring_to_utf8(pszPath);
            CoTaskMemFree(pszPath);
          }
          psi->Release();
        }

        IShellItemArray *psia;
        if (SUCCEEDED(pfv->Items(SVGIO_SELECTION, IID_IShellItemArray, (void **)&psia))) {
          DWORD count;
          if (SUCCEEDED(psia->GetCount(&count))) {
            for (DWORD i = 0; i < count; i++) {
              IShellItem *psi;
              if (SUCCEEDED(psia->GetItemAt(i, &psi))) {
                PWSTR pszPath;
                if (SUCCEEDED(psi->GetDisplayName(SIGDN_FILESYSPATH, &pszPath))) {
                  fv->selected_files.push_back(mb_shell::wstring_to_utf8(pszPath));
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
  }

  event_data.window_titlebar = std::make_shared<window_titlebar_controller>();
  (*event_data.window_titlebar)->$hwnd = hWnd;

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

void folder_view_controller::focus_file(std::string file_path) {
  IShellBrowser *browser = static_cast<IShellBrowser *>($controller);
  IShellView *view;
  if (SUCCEEDED(browser->QueryActiveShellView(&view))) {
    IFolderView *folder_view;
    if (SUCCEEDED(
            view->QueryInterface(IID_IFolderView, (void **)&folder_view))) {
      std::wstring wpath = mb_shell::utf8_to_wstring(file_path);
      PIDLIST_ABSOLUTE pidl;
      if (SUCCEEDED(
              SHParseDisplayName(wpath.c_str(), nullptr, &pidl, 0, nullptr))) {
        folder_view->SelectItem(SVSI_SELECT | SVSI_DESELECTOTHERS, -1);
        LPCITEMIDLIST pidll[1] = {pidl};
        folder_view->SelectAndPositionItems(1, pidll, 0, 0);
        CoTaskMemFree(pidl);
      }
      folder_view->Release();
    }
    view->Release();
  }
}

void folder_view_controller::open_file(std::string file_path) {
  ShellExecuteA(NULL, "open", file_path.c_str(), NULL, NULL, SW_SHOW);
}

void folder_view_controller::open_folder(std::string folder_path) {
  change_folder(folder_path);
}

void folder_view_controller::scroll_to_file(std::string file_path) {
  focus_file(file_path);
}

void folder_view_controller::refresh() {
  IShellBrowser *browser = static_cast<IShellBrowser *>($controller);
  IShellView *view;
  if (SUCCEEDED(browser->QueryActiveShellView(&view))) {
    view->Refresh();
    view->Release();
  }
}

void folder_view_controller::select_all() {
  IShellBrowser *browser = static_cast<IShellBrowser *>($controller);
  IShellView *view;
  if (SUCCEEDED(browser->QueryActiveShellView(&view))) {
    if (IFolderView * folder_view; SUCCEEDED(
            view->QueryInterface(IID_IFolderView, (void **)&folder_view))) {
      folder_view->SelectItem(SVSI_SELECT, -1);
      folder_view->Release();
    }
  }
}

void folder_view_controller::select_none() {
  IShellBrowser *browser = static_cast<IShellBrowser *>($controller);
  IShellView *view;
  if (SUCCEEDED(browser->QueryActiveShellView(&view))) {
    if (IFolderView * folder_view; SUCCEEDED(
            view->QueryInterface(IID_IFolderView, (void **)&folder_view))) {
      folder_view->SelectItem(SVSI_DESELECTOTHERS, -1);
      folder_view->Release();
    }
  }
}

void folder_view_controller::invert_selection() {
  IShellBrowser *browser = static_cast<IShellBrowser *>($controller);
  IShellView *view;
  if (SUCCEEDED(browser->QueryActiveShellView(&view))) {
    if (IFolderView * folder_view; SUCCEEDED(
            view->QueryInterface(IID_IFolderView, (void **)&folder_view))) {
      folder_view->SelectItem(SVSI_DESELECTOTHERS | SVSI_SELECT, -1);
      folder_view->Release();
    }
  }
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
  HICON hIcon = (HICON)LoadImageW(NULL, mb_shell::utf8_to_wstring(icon_path).c_str(), 
      IMAGE_ICON, 0, 0, LR_LOADFROMFILE);
  if (hIcon) {
    SendMessageW((HWND)$hwnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
    SendMessageW((HWND)$hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
  }
}

void window_titlebar_controller::set_position(int new_x, int new_y) {
  SetWindowPos((HWND)$hwnd, NULL, new_x, new_y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
}

void window_titlebar_controller::set_size(int new_width, int new_height) {
  SetWindowPos((HWND)$hwnd, NULL, 0, 0, new_width, new_height, SWP_NOMOVE | SWP_NOZORDER);
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

void window_titlebar_controller::focus() {
  SetForegroundWindow((HWND)$hwnd);
}

void window_titlebar_controller::show() {
  ShowWindow((HWND)$hwnd, SW_SHOW);
}

void window_titlebar_controller::hide() {
  ShowWindow((HWND)$hwnd, SW_HIDE);
}

void input_box_controller::set_text(std::string new_text) {
  SetWindowTextW((HWND)$hwnd, mb_shell::utf8_to_wstring(new_text).c_str());
}

void input_box_controller::set_placeholder(std::string new_placeholder) {
  SendMessageW((HWND)$hwnd, EM_SETCUEBANNER, TRUE, 
               (LPARAM)mb_shell::utf8_to_wstring(new_placeholder).c_str());
}

void input_box_controller::set_position(int new_x, int new_y) {
  SetWindowPos((HWND)$hwnd, NULL, new_x, new_y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
}

void input_box_controller::set_size(int new_width, int new_height) {
  SetWindowPos((HWND)$hwnd, NULL, 0, 0, new_width, new_height, SWP_NOMOVE | SWP_NOZORDER);
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

void input_box_controller::focus() {
  SetFocus((HWND)$hwnd);
}

void input_box_controller::blur() {
  SetFocus(NULL);
}

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

void input_box_controller::clear() {
  SetWindowTextW((HWND)$hwnd, L"");
}
} // namespace mb_shell::js