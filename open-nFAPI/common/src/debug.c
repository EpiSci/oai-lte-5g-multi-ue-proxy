/*
 * Copyright 2017 Cisco Systems, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "debug.h"
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <stdbool.h>

static const char log_name[] = "nfapi.log";

static nfapi_trace_level_t trace_level = NFAPI_TRACE_WARN;

static void nfapi_trace_init()
{
    static bool initialized;
    if (initialized)
        return;
    initialized = true;

    const char *env = getenv("NFAPI_TRACE_LEVEL");
    if (!env)
        return;
    if (strcmp(env, "none") == 0)
        trace_level = NFAPI_TRACE_NONE;
    else if (strcmp(env, "error") == 0)
        trace_level = NFAPI_TRACE_ERROR;
    else if (strcmp(env, "warn") == 0)
        trace_level = NFAPI_TRACE_WARN;
    else if (strcmp(env, "note") == 0)
        trace_level = NFAPI_TRACE_NOTE;
    else if (strcmp(env, "info") == 0)
        trace_level = NFAPI_TRACE_INFO;
    else if (strcmp(env, "debug") == 0)
        trace_level = NFAPI_TRACE_DEBUG;
    else
    {
        nfapi_trace(NFAPI_TRACE_ERROR, __func__, "Invalid NFAPI_TRACE_LEVEL='%s'", env);
        return;
    }
    nfapi_trace(trace_level, __func__, "NFAPI_TRACE_LEVEL='%s'", env);
}

nfapi_trace_level_t nfapi_trace_level()
{
    nfapi_trace_init();
    return trace_level;
}

void nfapi_trace(nfapi_trace_level_t level,
                 char const *caller,
                 char const *format, ...)
{
    FILE *fp = fopen(log_name, "a");
    if (fp == NULL)
    {
        fprintf(stderr, "open %s: %s\n", log_name, ERR);
        abort();
    }
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    fprintf(fp, "%ld%06ld [%c] %10u: %s: ",
            ts.tv_sec,
            ts.tv_nsec / 1000,
            "XEWNID"[level], // NFAPI_TRACE_NONE, NFAPI_TRACE_ERROR, ...
            (unsigned) pthread_self(),
            caller);

    va_list ap;
    va_start(ap, format);
    vfprintf(fp, format, ap);
    va_end(ap);

    // Add a newline if the format string didn't have one
    int len = strlen(format);
    if (len == 0 || format[len - 1] != '\n')
        putc('\n', fp);

    fclose(fp);
}
