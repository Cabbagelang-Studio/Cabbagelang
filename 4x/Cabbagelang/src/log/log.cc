#include <log/log.h>

int warnings_count=0, errors_count=0;

void compilation_info(const char* filename, int line, int column, const char* msg,...) {
    va_list args;
    va_start(args, msg);
    colorized_printf(LOG_CYAN, "[INFO] %s %d:%d: ", filename, line, column);
    vprintf(msg, args);
    va_end(args);
    printf("\n");
}

void compilation_warning(const char* filename, int line, int column, const char* msg,...) {
    va_list args;
    va_start(args, msg);
    colorized_printf(LOG_YELLOW, "[WARNING] %s %d:%d: ", filename, line, column);
    vprintf(msg, args);
    va_end(args);
    printf("\n");
    warnings_count++;
}

void compilation_error(const char* filename, int line, int column, const char* msg,...) {
    va_list args;
    va_start(args, msg);
    colorized_printf(LOG_RED, "[ERROR] %s %d:%d: ", filename, line, column);
    vprintf(msg, args);
    va_end(args);
    printf("\n");
    errors_count++;
}

void compiler_fatal(const char* msg,...) {
    va_list args;
    va_start(args, msg);
    colorized_printf(LOG_RED, "[FATAL] ");
    colorized_vprintf(LOG_RED, msg, args);
    va_end(args);
    printf("\n");
    exit(1);
}

void warning_error_summary() {
    if (warnings_count > 0)
        colorized_printf(LOG_YELLOW, "%d warning(s) detected\n", warnings_count);
    if (errors_count > 0) {
        colorized_printf(LOG_RED, "%d error(s) detected\n", errors_count);
        exit(1);
    }
}