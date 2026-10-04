#include "error_handler.h"

#include <cstdarg>
#include <cstdio>
#include <exception>
#include <filesystem>
#include <intrin.h>
#include <string>
#include <thread>

#include "build_info.h"
#include "config.h"
#include "utils.h"

#include "sentry.h"
#include "spdlog/spdlog.h"

#include <signal.h>
#include <windows.h>
#include <psapi.h>

#ifndef STATUS_FATAL_APP_EXIT
#define STATUS_FATAL_APP_EXIT 0x40000015L
#endif

namespace {
constexpr DWORD kAbortExitCode = 0xC0000409;
constexpr int kMaxFrames = 64;
constexpr int kMaxStackModules = 8;

void (*g_previous_sigabrt_handler)(int) = nullptr;
LPTOP_LEVEL_EXCEPTION_FILTER g_previous_filter = nullptr;

wchar_t g_marker_path[MAX_PATH * 2];
DWORD64 g_self_begin = 0;
DWORD64 g_self_end = 0;
ULONGLONG g_start_tick = 0;
volatile LONG g_marker_written = 0;

volatile DWORD64 g_terminate_frame = 0;
volatile DWORD g_terminate_tid = 0;

struct stack_verdict {
    int frames = 0;
    int first_breeze_frame = -1;
    int breeze_frames = 0;
    DWORD64 module_bases[kMaxStackModules] = {};
    int module_count = 0;
};

bool in_self(DWORD64 pc) { return pc >= g_self_begin && pc < g_self_end; }

DWORD64 image_base_of(DWORD64 pc) {
    PVOID base = nullptr;
    RtlPcToFileHeader(reinterpret_cast<PVOID>(pc), &base);
    return reinterpret_cast<DWORD64>(base);
}

const char *image_name(DWORD64 base) {
    if (!base)
        return "?";
    __try {
        auto dos = reinterpret_cast<const IMAGE_DOS_HEADER *>(base);
        auto nt = reinterpret_cast<const IMAGE_NT_HEADERS *>(base + dos->e_lfanew);
        auto &dir =
            nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
        if (!dir.VirtualAddress)
            return "?";
        auto exports = reinterpret_cast<const IMAGE_EXPORT_DIRECTORY *>(
            base + dir.VirtualAddress);
        return reinterpret_cast<const char *>(base + exports->Name);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return "?";
    }
}

void classify_frame(DWORD64 pc, stack_verdict &v) {
    if (in_self(pc)) {
        if (v.first_breeze_frame < 0)
            v.first_breeze_frame = v.frames;
        v.breeze_frames++;
    }
    v.frames++;

    DWORD64 base = image_base_of(pc);
    if (!base)
        return;
    for (int i = 0; i < v.module_count; i++)
        if (v.module_bases[i] == base)
            return;
    if (v.module_count < kMaxStackModules)
        v.module_bases[v.module_count++] = base;
}

void scan_stack(CONTEXT ctx, DWORD64 skip_until_sp, bool skip_leading_self,
                stack_verdict &v) {
#if defined(_M_AMD64)
    __try {
        for (int i = 0; i < kMaxFrames && ctx.Rip; i++) {
            DWORD64 pc = ctx.Rip;
            if (skip_leading_self && !in_self(pc))
                skip_leading_self = false;
            if (ctx.Rsp > skip_until_sp && !skip_leading_self)
                classify_frame(pc, v);

            DWORD64 image_base = 0;
            auto fn = RtlLookupFunctionEntry(pc, &image_base, nullptr);
            if (!fn) {
                ctx.Rip = *reinterpret_cast<DWORD64 *>(ctx.Rsp);
                ctx.Rsp += 8;
                continue;
            }
            PVOID handler_data = nullptr;
            DWORD64 establisher = 0;
            RtlVirtualUnwind(UNW_FLAG_NHANDLER, image_base, pc, fn, &ctx,
                             &handler_data, &establisher, nullptr);
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
    }
#else
    (void)skip_until_sp;
    (void)skip_leading_self;
    classify_frame(ctx.Pc, v);
#endif
}

int append(char *buf, size_t size, int used, const char *fmt, ...) {
    if (used < 0 || static_cast<size_t>(used) >= size)
        return used;
    va_list ap;
    va_start(ap, fmt);
    int n = _vsnprintf_s(buf + used, size - used, _TRUNCATE, fmt, ap);
    va_end(ap);
    return n < 0 ? static_cast<int>(size) - 1 : used + n;
}

void write_crash_marker(const EXCEPTION_RECORD *record, const CONTEXT *context,
                        DWORD64 skip_until_sp, bool skip_leading_self) {
    if (!g_marker_path[0] || InterlockedExchange(&g_marker_written, 1))
        return;

    stack_verdict verdict;
    if (context)
        scan_stack(*context, skip_until_sp, skip_leading_self, verdict);

    DWORD64 address =
        record ? reinterpret_cast<DWORD64>(record->ExceptionAddress) : 0;
    bool synthetic = skip_until_sp || skip_leading_self;

    char text[1024];
    int used = append(text, sizeof(text), 0,
                      "code=0x%08lX\r\naddress=0x%016llX\r\nmodule=%s\r\n"
                      "first_breeze_frame=%d\r\nbreeze_frames=%d\r\n"
                      "frames=%d\r\nuptime_ms=%llu\r\nversion=%s\r\n"
                      "stack_modules=",
                      record ? record->ExceptionCode : 0, address,
                      synthetic ? "-" : image_name(image_base_of(address)),
                      verdict.first_breeze_frame, verdict.breeze_frames,
                      verdict.frames, GetTickCount64() - g_start_tick,
                      BREEZE_VERSION);
    for (int i = 0; i < verdict.module_count; i++)
        used = append(text, sizeof(text), used, i ? ";%s" : "%s",
                      image_name(verdict.module_bases[i]));
    used = append(text, sizeof(text), used, "\r\n");

    HANDLE file = CreateFileW(g_marker_path, GENERIC_WRITE, FILE_SHARE_READ,
                              nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL,
                              nullptr);
    if (file == INVALID_HANDLE_VALUE)
        return;
    DWORD written = 0;
    WriteFile(file, text, static_cast<DWORD>(used), &written, nullptr);
    FlushFileBuffers(file);
    CloseHandle(file);
}

__declspec(noinline) void capture_synthetic_crash(DWORD code) {
    CONTEXT context;
    RtlCaptureContext(&context);

    EXCEPTION_RECORD record;
    memset(&record, 0, sizeof(record));
    record.ExceptionCode = code;
    record.ExceptionFlags = EXCEPTION_NONCONTINUABLE;

#if defined(_M_AMD64)
    record.ExceptionAddress = (PVOID)context.Rip;
#elif defined(_M_ARM64)
    record.ExceptionAddress = (PVOID)context.Pc;
#endif

    DWORD64 terminate_frame =
        g_terminate_tid == GetCurrentThreadId() ? g_terminate_frame : 0;
    write_crash_marker(&record, &context, terminate_frame, !terminate_frame);

    EXCEPTION_POINTERS exception_pointers;
    exception_pointers.ContextRecord = &context;
    exception_pointers.ExceptionRecord = &record;

    sentry_ucontext_t uctx;
    uctx.exception_ptrs = exception_pointers;
    sentry_handle_exception(&uctx);
}

void handle_sigabrt(int signum) {
    capture_synthetic_crash(STATUS_FATAL_APP_EXIT);

    if (g_previous_sigabrt_handler && g_previous_sigabrt_handler != SIG_DFL &&
        g_previous_sigabrt_handler != SIG_IGN) {
        g_previous_sigabrt_handler(signum);
    }

    TerminateProcess(GetCurrentProcess(), kAbortExitCode);
}

void handle_purecall() {
    capture_synthetic_crash(STATUS_FATAL_APP_EXIT);
    TerminateProcess(GetCurrentProcess(), kAbortExitCode);
}

void handle_invalid_parameter(const wchar_t *, const wchar_t *,
                              const wchar_t *, unsigned int, uintptr_t) {
    capture_synthetic_crash(STATUS_INVALID_CRUNTIME_PARAMETER);
    TerminateProcess(GetCurrentProcess(), kAbortExitCode);
}

[[noreturn]] void on_terminate() {
    g_terminate_frame = reinterpret_cast<DWORD64>(_AddressOfReturnAddress());
    g_terminate_tid = GetCurrentThreadId();
    if (auto eptr = std::current_exception()) {
        try {
            std::rethrow_exception(eptr);
        } catch (const std::exception &e) {
            spdlog::critical("Uncaught exception: {}", e.what());
        } catch (...) {
            spdlog::critical("Uncaught exception of unknown type");
        }
    }
    std::abort();
}

LONG WINAPI fallback_filter(EXCEPTION_POINTERS *ep) {
    write_crash_marker(ep->ExceptionRecord, ep->ContextRecord, 0, false);
    return g_previous_filter ? g_previous_filter(ep)
                             : EXCEPTION_CONTINUE_SEARCH;
}

sentry_value_t on_crash(const sentry_ucontext_t *uctx, sentry_value_t event,
                        void *) {
    if (uctx)
        write_crash_marker(uctx->exception_ptrs.ExceptionRecord,
                           uctx->exception_ptrs.ContextRecord, 0, false);
    return event;
}

void prepare_crash_marker() {
    HMODULE self = nullptr;
    GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                           GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                       reinterpret_cast<LPCWSTR>(&prepare_crash_marker), &self);
    MODULEINFO info{};
    if (GetModuleInformation(GetCurrentProcess(), self, &info, sizeof(info))) {
        g_self_begin = reinterpret_cast<DWORD64>(info.lpBaseOfDll);
        g_self_end = g_self_begin + info.SizeOfImage;
    }
    g_start_tick = GetTickCount64();

    std::error_code ec;
    auto dir = mb_shell::config::data_directory() / "crashes";
    std::filesystem::create_directories(dir, ec);
    auto path = (dir / (std::to_wstring(GetCurrentProcessId()) + L".txt"))
                    .wstring();
    std::filesystem::remove(path, ec);
    wcsncpy_s(g_marker_path, path.c_str(), _TRUNCATE);
}
void init_sentry() {
    sentry_options_t *options = sentry_options_new();
    if (!GetEnvironmentVariableA("SENTRY_DSN", nullptr, 0)) {
        sentry_options_set_dsn(
            options, "https://"
                     "159f800f906ad4d9e04d139d5db21fbd@o4510630644744192."
                     "ingest.de.sentry.io/4510987356274768");
    }

    sentry_options_set_release(options, "breeze-shell@" BREEZE_VERSION);
    sentry_options_set_environment(options, "production");

    auto db_path = mb_shell::config::data_directory() / ".sentry-native";
    sentry_options_set_database_pathw(options, db_path.c_str());
    sentry_options_set_handler_path(options, nullptr);
    sentry_options_set_on_crash(options, on_crash, nullptr);

    if (sentry_init(options) != 0) {
        spdlog::error("sentry_init failed, only local crash markers remain");
        return;
    }
    sentry_set_tag("git_commit", BREEZE_GIT_COMMIT_HASH);
    sentry_set_tag("git_branch", BREEZE_GIT_BRANCH_NAME);
    sentry_set_tag("windows_version",
                   mb_shell::is_win11_or_later() ? "11+" : "10-");
}
} // namespace

void mb_shell::install_error_handlers() {
    prepare_crash_marker();
    std::set_terminate(on_terminate);
    g_previous_filter = SetUnhandledExceptionFilter(fallback_filter);
    g_previous_sigabrt_handler = signal(SIGABRT, handle_sigabrt);
    _set_purecall_handler(handle_purecall);
    _set_invalid_parameter_handler(handle_invalid_parameter);

    /* sentry_init 会调 WinHttpOpen, 在 DllMain 的 loader lock 下会死锁 */
    std::thread(init_sentry).detach();
}

void mb_shell::report_warning(const char *message) {
    sentry_value_t event = sentry_value_new_event();
    sentry_value_set_by_key(event, "level", sentry_value_new_string("warning"));

    sentry_value_t exc = sentry_value_new_exception("Warning", message);
    sentry_value_t stacktrace = sentry_value_new_stacktrace(nullptr, 0);
    sentry_value_set_by_key(exc, "stacktrace", stacktrace);
    sentry_value_set_by_key(event, "exception", sentry_value_new_list());
    sentry_value_append(sentry_value_get_by_key(event, "exception"), exc);

    sentry_capture_event(event);
}

void mb_shell::cleanup_error_handlers() {
    if (g_previous_sigabrt_handler) {
        signal(SIGABRT, g_previous_sigabrt_handler);
        g_previous_sigabrt_handler = nullptr;
    }

    sentry_close();
}
