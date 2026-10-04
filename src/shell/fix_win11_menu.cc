#include "fix_win11_menu.h"

#include <cstdlib>
#include <string>
#include <thread>
#include <vector>

#include <spdlog/spdlog.h>

#include "blook/blook.h"
#include "blook/memo.h"
#include "config.h"
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

    // patch the shell32.dll to predent the shift key is pressed
    std::thread([=]() {
        try {
            auto user32 = LoadLibraryA("user32.dll");
            auto extraInfo = user32
                                 ? GetProcAddress(user32, "SetMessageExtraInfo")
                                 : nullptr;
            auto getKeyState =
                user32 ? GetProcAddress(user32, "GetKeyState") : nullptr;
            auto getAsyncKeyState =
                user32 ? GetProcAddress(user32, "GetAsyncKeyState") : nullptr;
            if (!extraInfo || !getKeyState) {
                spdlog::error(
                    "Failed to resolve user32 exports for win11 menu fix");
                return;
            }

            auto imported_call_target = [](auto &insn) -> void * {
                if (insn->getMnemonic() != zasm::x86::Mnemonic::Call)
                    return nullptr;

                auto xrefs = insn.xrefs();
                if (xrefs.empty())
                    return nullptr;

                if (auto ptr = xrefs[0].try_read_pointer())
                    return ptr->data();

                return nullptr;
            };

            auto is_mov_ecx_imm = [](auto &insn, int value) {
                return insn->getMnemonic() == zasm::x86::Mnemonic::Mov &&
                       insn->getOperand(0) == zasm::x86::ecx &&
                       insn->getOperand(1).template holds<zasm::Imm>() &&
                       insn->getOperand(1)
                               .template get<zasm::Imm>()
                               .template value<int>() == value;
            };

            auto is_key_state_call = [&](auto &insn) {
                auto target = imported_call_target(insn);
                return target == getKeyState || target == getAsyncKeyState;
            };

            auto find_key_state_check = [&](auto mem, int value) -> void * {
                for (auto it = mem.begin(); it != mem.end(); ++it) {
                    auto next = std::next(it);
                    if (next == mem.end())
                        break;

                    auto &insn = *it;
                    if (is_mov_ecx_imm(insn, value) &&
                        is_key_state_call(*next)) {
                        return insn.ptr().data();
                    }
                }

                return nullptr;
            };

            auto has_key_state_check = [&](auto mem, int value) {
                return find_key_state_check(mem, value) != nullptr;
            };

            auto patch_key_state_check = [&](auto mem, int value) {
                if (auto addr = find_key_state_check(mem, value)) {
                    spdlog::info("Patching key state check at: {}",
                                 (void *)addr);
                    blook::Pointer(addr)
                        .range_next_instr(2)
                        .reassembly_with_padding(
                            [](auto a) { a.mov(zasm::x86::rax, 0xffff); })
                        .patch();
                    return true;
                }

                return false;
            };

            // Classic menu or not is decided from the extended-verbs flag;
            // forcing that decision keeps the flag out of the shell handlers.
            constexpr int kExtendedVerbs = 0x100;

            auto patch_classic_menu_decision = [&](auto mem, auto module) {
                auto shift_check = find_key_state_check(mem, 0x10);
                if (!shift_check)
                    return false;

                void *decision = nullptr;
                bool after_shift_check = false;
                for (auto it = mem.begin(); it != mem.end(); ++it) {
                    auto &insn = *it;
                    if (insn.ptr().data() == shift_check)
                        after_shift_check = true;
                    if (!after_shift_check)
                        continue;
                    if (insn->getMnemonic() != zasm::x86::Mnemonic::Call)
                        continue;
                    const auto &operand = insn->getOperand(0);
                    if (!operand.template holds<zasm::Imm>())
                        continue;

                    decision = (void *)operand.template get<zasm::Imm>()
                                   .template value<uintptr_t>();
                    break;
                }

                auto module_begin = (uintptr_t)module->base().data();
                auto module_end = module_begin + module->size();
                if (!decision || (uintptr_t)decision < module_begin ||
                    (uintptr_t)decision >= module_end) {
                    return false;
                }

                auto decision_code =
                    blook::Pointer(decision).range_size(0x200).disassembly();
                for (auto it = decision_code.begin(); it != decision_code.end();
                     ++it) {
                    auto &insn = *it;
                    if (insn->getMnemonic() == zasm::x86::Mnemonic::Ret)
                        return false;
                    if (insn->getMnemonic() != zasm::x86::Mnemonic::And ||
                        insn->getOperandCount() < 2)
                        continue;

                    const auto &dst = insn->getOperand(0);
                    const auto &src = insn->getOperand(1);
                    if (!dst.template holds<zasm::x86::Reg>() ||
                        !src.template holds<zasm::Imm>())
                        continue;
                    if (src.template get<zasm::Imm>().template value<int>() !=
                        kExtendedVerbs)
                        continue;

                    auto reg = zasm::x86::Gp(
                        dst.template get<zasm::x86::Reg>().getId());
                    auto patched =
                        insn.ptr().range_next_instr(1).try_reassembly_with_padding(
                            [reg](zasm::x86::Assembler a) {
                                a.or_(reg, kExtendedVerbs);
                            });
                    if (!patched)
                        return false;

                    patched->patch();
                    spdlog::info(
                        "Patched shell32.dll extended-verbs menu decision: {}",
                        (void *)decision);
                    return true;
                }

                return false;
            };

            if (auto shell32 = proc->module("shell32.dll")) {
                auto disasm = shell32.value()->section(".text")->disassembly();

                // the function to determine if show win10 menu or win11 menu
                // calls SetMessageExtraInfo, so we use it as a hint
                for (auto &ins : disasm) {
                    if (imported_call_target(ins) != extraInfo)
                        continue;

                    auto function =
                        ins.ptr()
                            .find_upwards({0xCC, 0xCC, 0xCC, 0xCC, 0xCC})
                            ->range_size(0xB50);

                    if (patch_classic_menu_decision(function.disassembly(),
                                                    shell32.value())) {
                        break;
                    }

                    if (patch_key_state_check(function.disassembly(), 0x10)) {
                        spdlog::warn("Patched shell32.dll by faking the SHIFT "
                                     "key state; \"Send To\" may list the user "
                                     "profile folders");
                        break;
                    }
                }
            }

            if (config::current->context_menu.patch_explorerframe_dll) {
                if (auto explorerframe = proc->module("ExplorerFrame.dll")) {
                    auto disasm =
                        explorerframe.value()->section(".text")->disassembly();

                    for (auto &ins : disasm) {
                        if (!is_key_state_call(ins))
                            continue;

                        auto function =
                            ins.ptr()
                                .find_upwards({0xCC, 0xCC, 0xCC, 0xCC, 0xCC})
                                ->range_size(0x200)
                                .disassembly();
                        if (!has_key_state_check(function, 0x10) ||
                            !has_key_state_check(function, 0x79)) {
                            continue;
                        }

                        if (patch_key_state_check(function, 0x10)) {
                            spdlog::info("Patched explorerframe.dll for win11 "
                                         "menu fix: {}",
                                         ins.ptr().data());
                            break;
                        }
                    }
                }
            }
        } catch (...) {
            spdlog::error("Failed to patch win11 menu fix");
        }
    }).detach();
}

