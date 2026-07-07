/// @file android_log.h
/// Early logging for Android init service debugging

#ifndef VERE_ANDROID_LOG_H
#define VERE_ANDROID_LOG_H

#ifdef __ANDROID__

#include <stdio.h>
#include <stdarg.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

#define VERE_EARLY_LOG_DIR  "/data/nativeplanet/logs"
#define VERE_EARLY_LOG_PATH "/data/nativeplanet/logs/vere-early.log"

static FILE* _android_early_log_f = NULL;

static inline void
android_early_log_open(void)
{
  if ( _android_early_log_f ) return;

  // ensure log directory exists
  mkdir(VERE_EARLY_LOG_DIR, 0700);

  _android_early_log_f = fopen(VERE_EARLY_LOG_PATH, "a");
  if ( _android_early_log_f ) {
    setbuf(_android_early_log_f, NULL);  // unbuffered for crash safety

    time_t now = time(NULL);
    struct tm* tm = localtime(&now);
    fprintf(_android_early_log_f,
            "\n=== vere early log %04d-%02d-%02d %02d:%02d:%02d ===\n",
            tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
            tm->tm_hour, tm->tm_min, tm->tm_sec);
  }
}

static inline void
android_early_log(const char* fmt, ...)
{
  if ( !_android_early_log_f ) {
    android_early_log_open();
  }

  if ( _android_early_log_f ) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(_android_early_log_f, fmt, ap);
    va_end(ap);
    fprintf(_android_early_log_f, "\n");
    fflush(_android_early_log_f);
  }
}

static inline void
android_early_log_argv(int argc, char** argv)
{
  android_early_log("argc: %d", argc);
  for ( int i = 0; i < argc; i++ ) {
    android_early_log("argv[%d]: %s", i, argv[i] ? argv[i] : "(null)");
  }
}

static inline void
android_early_log_close(void)
{
  if ( _android_early_log_f ) {
    fclose(_android_early_log_f);
    _android_early_log_f = NULL;
  }
}

#else

// no-op on non-Android
#define android_early_log_open()       ((void)0)
#define android_early_log(fmt, ...)    ((void)0)
#define android_early_log_argv(a, b)   ((void)0)
#define android_early_log_close()      ((void)0)

#endif // __ANDROID__

#endif // VERE_ANDROID_LOG_H
