#ifndef LOG_LOG_H
#define LOG_LOG_H

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <log/colorize.h>
#include <iostream>

void compilation_info(const char* filename,int line,int column,const char* msg,...);
void compilation_warning(const char* filename,int line,int column,const char* msg,...);
void compilation_error(const char* filename,int line,int column,const char* msg,...);
void compiler_fatal(const char* msg,...);
void warning_error_summary();

#endif // LOG_LOG_H