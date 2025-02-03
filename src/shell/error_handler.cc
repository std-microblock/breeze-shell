#include "error_handler.h"

#include <iostream>
#include <stacktrace>
#include <string>
#include <exception>

#include "Windows.h"

void show_console () {
    if (!GetConsoleWindow()) {
        AllocConsole();
        freopen("CONOUT$", "w", stdout);
        freopen("CONOUT$", "w", stderr);
    }
    ShowWindow(GetConsoleWindow(), SW_SHOW);
    SetForegroundWindow(GetConsoleWindow());
}

void mb_shell::install_error_handlers() {
    SetUnhandledExceptionFilter([](PEXCEPTION_POINTERS ex) -> LONG {
        show_console();
        std::cerr << "Unhandled exception: " << std::hex << ex->ExceptionRecord->ExceptionCode << std::endl;
        std::cerr << std::stacktrace::current() << std::endl;
        Sleep(5000);
        return EXCEPTION_CONTINUE_EXECUTION;
    });
    
    std::set_terminate([]() {
        show_console();
        try {
            throw std::current_exception();
        } catch (const std::exception &e) {
            std::cerr << "Uncaught exception: " << e.what() << std::endl;
        } catch (...) {
            std::cerr << "Uncaught exception of unknown type" << std::endl;
        }
        Sleep(5000);
    });
}
