// SPDX-FileCopyrightText: 2026 Jules C
// SPDX-License-Identifier: MIT

#ifndef ULK_TYPES_H
#define ULK_TYPES_H

#include <stdint.h>

#define ULK_API_VERSION_MAJOR 1
#define ULK_API_VERSION_MINOR 6

typedef uint64_t ulk_size;
typedef int ulk_bool;

#if defined(_WIN32)
#define ULK_CALL __cdecl
#if defined(ULK_BUILD_SHARED)
#define ULK_API __declspec(dllexport)
#elif defined(ULK_USE_SHARED)
#define ULK_API __declspec(dllimport)
#else
#define ULK_API
#endif
#else
#define ULK_CALL
#if defined(__GNUC__) && defined(ULK_BUILD_SHARED)
#define ULK_API __attribute__((visibility("default")))
#else
#define ULK_API
#endif
#endif

typedef struct ulk_string_view {
    const char* data;
    ulk_size size;
} ulk_string_view;

#endif
