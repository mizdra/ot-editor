#pragma once

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_DOCUMENT_SIZE 1024
#define MAX_ACTION_SIZE 1000

#define PANIC(fmt, ...)                                                     \
  {                                                                         \
    err_msg(__FILE__, __FUNCTION__, __LINE__, "panic", fmt, ##__VA_ARGS__); \
    exit(EXIT_FAILURE);                                                     \
  }
#define ERROR(fmt, ...) \
  err_msg(__FILE__, __FUNCTION__, __LINE__, "error", fmt, ##__VA_ARGS__)
#define WARNING(fmt, ...) \
  err_msg(__FILE__, __FUNCTION__, __LINE__, "warning", fmt, ##__VA_ARGS__)

#define RED "\033[0;31m"
#define BOLD "\033[1m"
#define RESET_COLOR "\033[0m"

int min(int a, int b) { return a > b ? b : a; }

void err_msg(const char *file, const char *function, int line, const char *type,
             const char *fmt, ...) {
  fprintf(stderr, RED);           // set color
  fprintf(stderr, "%s: ", type);  // print type

  // print message
  va_list va;
  va_start(va, fmt);
  vfprintf(stderr, fmt, va);  // fmtの書式に従って可変個数引数を順次呼び出す
  va_end(va);

  fprintf(stderr, RESET_COLOR);  // reset color
  fputc('\n', stderr);           // new line

  fprintf(stderr, BOLD);                                       // set color
  fprintf(stderr, " --> `%s` (%s:%d)", function, file, line);  // print location
  fprintf(stderr, RESET_COLOR);                                // reset color
  fputc('\n', stderr);                                         // new line

  if (errno != 0) {
    fprintf(stderr, " = errno: %s(%d)", strerror(errno),
            errno);       // print errno
    fputc('\n', stderr);  // new line
    errno = 0;
  }
}
