#include "fix_win11_menu.h"

#include <cstdlib>
#include <string>
#include <thread>
#include <vector>

#include <spdlog/spdlog.h>

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

    // RegGetValueHook->install(
    //     (void *)+[](HKEY hkey, LPCWSTR lpSubKey, LPCWSTR lpValue, DWORD
    //     dwFlags,
    //                 LPDWORD pdwType, PVOID pvData, LPDWORD pcbData) {
    //       // simulate
    //       // reg.exe add
    //       //
    //       //
    //       //
    //       "HKCU\Software\Classes\CLSID\{86ca1aa0-34aa-4e8b-a509-50c905bae2a2}\InprocServer32"
    //       // /f /ve
    //       auto path = wstring_to_utf8(RegQueryKeyPath(hkey));
    //       if
    //       (path.ends_with("\\CLSID\\{86ca1aa0-34aa-4e8b-a509-50c905bae2a2}"
    //                          "\\InprocServer32")) {
    //         if (pvData != nullptr && pcbData != nullptr) {
    //           *pcbData = 0;
    //         }
    //         if (pdwType != nullptr) {
    //           *pdwType = REG_SZ;
    //         }
    //         return ERROR_SUCCESS;
    //       } else
    //         return RegGetValueHook->call_trampoline<long>(
    //             hkey, lpSubKey, lpValue, dwFlags, pdwType, pvData, pcbData);
    //     });

    // approch 2: patch the shell32.dll to predent the shift key is pressed
    std::thread([=]() {
        try {

            auto patch_area = [&](auto mem) {
                for (auto it = mem.begin(); it != mem.end(); ++it) {
                    auto &insn = *it;
                    if (insn->getMnemonic() == zasm::x86::Mnemonic::Mov) {
                        if (insn->getOperand(0) == zasm::x86::ecx &&
                            insn->getOperand(1).template holds<zasm::Imm>() &&
                            insn->getOperand(1)
                                    .template get<zasm::Imm>()
                                    .template value<int>() == 0x10) {
                            auto &next = *std::next(it);
                            if (next->getMnemonic() ==
                                    zasm::x86::Mnemonic::Call &&
                                next->getOperand(0)
                                    .template holds<zasm::Mem>()) {
                                insn.ptr()
                                    .reassembly([](auto a) {
                                        a.mov(zasm::x86::ecx, 0x10);
                                        a.mov(zasm::x86::eax, 0xffff);
                                        a.nop();
                                        a.nop();
                                    })
                                    .patch();

                                return true;
                            }
                        }
                    }
                }

                return false;
            };

            if (auto shell32 = proc->module("shell32.dll")) {
                // mov ecx, 10
                // call GetKeyState/GetAsyncKeyState
                auto disasm = shell32.value()->section(".text")->disassembly();

                // the function to determine if show win10 menu or win11 menu
                // calls SetMessageExtraInfo, so we use it as a hint
                auto extraInfo = GetProcAddress(LoadLibraryA("user32.dll"),
                                                "SetMessageExtraInfo");
                for (auto &ins : disasm) {
                    if (ins->getMnemonic() == zasm::x86::Mnemonic::Call) {
                        auto xrefs = ins.xrefs();
                        if (xrefs.empty())
                            continue;
                        if (auto ptr = xrefs[0].try_read_pointer();
                            ptr && ptr->data() == extraInfo) {
                            if (patch_area(ins.ptr()
                                               .find_upwards({0xCC, 0xCC, 0xCC,
                                                              0xCC, 0xCC})
                                               ->range_size(0xB50)
                                               .disassembly())) {
                                spdlog::info(
                                    "Patched shell32.dll for win11 menu fix");
                                break;
                            }
                        }
                    }
                }
            }

            if (auto explorerframe = proc->module("ExplorerFrame.dll")) {
                /*
                CNscTree::ShouldShowMiniMenu

                1801ca353  b910000000         mov     ecx, 0x10
                1801ca358  48ff1521cb0700     call    qword [rel GetKeyState]
                */

                if (auto res =
                        explorerframe.value()->section(".text")->find_one(
                            {0xb9, 0x10, 0x00, 0x00, 0x00, 0x48, 0xff, 0x15,
                             0x21, 0xcb, 0x07, 0x00})) {
                    if (patch_area(res->range_size(0x20).disassembly()))
                        spdlog::info(
                            "Patched explorerframe.dll for win11 menu fix");
                }
            }
        } catch (...) {
            spdlog::error("Failed to patch shell32.dll for win11 menu fix");
        }
    }).detach();
}
