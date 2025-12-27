#ifndef LOG_COLORIZE_H
#define LOG_COLORIZE_H

#include <stdio.h>
#include <stdarg.h>

// Color codes
#define RESET_COLOR   "\033[0m"
#define LOG_RED           "\033[31m"
#define LOG_YELLOW        "\033[33m"
#define LOG_CYAN          "\033[36m"

// Function declarations
void enable_ansi_support();
void colorized_printf(const char* color, const char* format, ...);
void colorized_vprintf(const char* color, const char* format, va_list args);

#endif // LOG_COLORIZE_H
