#include <log/colorize.h>

#ifdef _WIN32
#include <windows.h>
#endif

#ifdef _WIN32
void enable_ansi_support() {
    HANDLE hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode;
    if (GetConsoleMode(hStdout, &mode)) {
        mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        SetConsoleMode(hStdout, mode);
    }
}
#else
// For Linux/MacOS, ANSI support is enabled by default
void enable_ansi_support() {}
#endif

// Print a formatted string with color
void colorized_printf(const char* color, const char* format, ...) {
    enable_ansi_support();

    // Print color code
    if (color) {
        printf("%s", color);
    }

    // Print formatted string
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);

    // Reset color
    printf("%s", RESET_COLOR);
}

void colorized_vprintf(const char* color, const char* format, va_list args) {
    enable_ansi_support();

    // Print color code
    if (color) {
        printf("%s", color);
    }

    // Print formatted color
    vprintf(format, args);

    // Reset color
    printf("%s", RESET_COLOR);
}