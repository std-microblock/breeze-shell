#include "fix_win11_menu.h"

#include <cstdlib>
#include <print>
#include <string>
#include <thread>
#include <vector>

#include "blook/blook.h"
#include "blook/memo.h"
#include "utils.h"
#include "zasm/base/immediate.hpp"
#include "zasm/x86/mnemonic.hpp"
#include "zasm/x86/register.hpp"

// https://stackoverflow.com/questions/937044/determine-path-to-registry-key-from-hkey-handle-in-c
#include <ntstatus.h>
#define WIN32_NO_STATUS
#include <windows.h>
#include <winternl.h>
#pragma comment(lib, "ntdll")
#include <string>
#include <vector>

#define REG_KEY_PATH_LENGTH 1024

typedef enum _KEY_INFORMATION_CLASS {
  KeyBasicInformation,
  KeyNodeInformation,
  KeyFullInformation,
  KeyNameInformation,
  KeyCachedInformation,
  KeyFlagsInformation,
  KeyVirtualizationInformation,
  KeyHandleTagsInformation,
  KeyTrustInformation,
  KeyLayerInformation,
  MaxKeyInfoClass
} KEY_INFORMATION_CLASS;

typedef struct _KEY_NAME_INFORMATION {
  ULONG NameLength;
  WCHAR Name[1];
} KEY_NAME_INFORMATION, *PKEY_NAME_INFORMATION;

EXTERN_C NTSYSAPI NTSTATUS NTAPI NtQueryKey(
    __in HANDLE /* KeyHandle */,
    __in KEY_INFORMATION_CLASS /* KeyInformationClass */,
    __out_opt PVOID /* KeyInformation */, __in ULONG /* Length */,
    __out ULONG * /* ResultLength */
);

std::wstring RegQueryKeyPath(HKEY hKey) {
  std::wstring keyPath;

  NTSTATUS Status;

  std::vector<UCHAR> Buffer(FIELD_OFFSET(KEY_NAME_INFORMATION, Name) +
                            sizeof(WCHAR) * REG_KEY_PATH_LENGTH);
  KEY_NAME_INFORMATION *pkni;
  ULONG Length;

TryAgain:
  Status = NtQueryKey(hKey, KeyNameInformation, Buffer.data(), Buffer.size(),
                      &Length);
  switch (Status) {
  case STATUS_BUFFER_TOO_SMALL:
  case STATUS_BUFFER_OVERFLOW:
    Buffer.resize(Length);
    goto TryAgain;
  case STATUS_SUCCESS:
    pkni = reinterpret_cast<KEY_NAME_INFORMATION *>(Buffer.data());
    keyPath.assign(pkni->Name, pkni->NameLength / sizeof(WCHAR));
  default:
    break;
  }

  return keyPath;
}
void mb_shell::fix_win11_menu::install() {
  auto proc = blook::Process::self();

  // approch 1: simulated reg edit
  auto advapi32 = proc->module("kernelbase.dll");

  auto RegGetValueW = advapi32.value()->exports("RegGetValueW");
  static auto RegGetValueHook = RegGetValueW->inline_hook();

  RegGetValueHook->install(
      (void *)+[](HKEY hkey, LPCWSTR lpSubKey, LPCWSTR lpValue, DWORD dwFlags,
                  LPDWORD pdwType, PVOID pvData, LPDWORD pcbData) {
        // simulate
        // reg.exe add
        //
        //
        // "HKCU\Software\Classes\CLSID\{86ca1aa0-34aa-4e8b-a509-50c905bae2a2}\InprocServer32"
        // /f /ve
        auto path = wstring_to_utf8(RegQueryKeyPath(hkey));
        if (path.ends_with("\\CLSID\\{86ca1aa0-34aa-4e8b-a509-50c905bae2a2}"
                           "\\InprocServer32")) {
          if (pvData != nullptr && pcbData != nullptr) {
            *pcbData = 0;
          }
          if (pdwType != nullptr) {
            *pdwType = REG_SZ;
          }
          return ERROR_SUCCESS;
        } else
          return RegGetValueHook->call_trampoline<long>(
              hkey, lpSubKey, lpValue, dwFlags, pdwType, pvData, pcbData);
      });
}
