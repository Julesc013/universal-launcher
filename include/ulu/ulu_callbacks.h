// SPDX-FileCopyrightText: 2026 Jules C
// SPDX-License-Identifier: MIT

#ifndef ULU_CALLBACKS_H
#define ULU_CALLBACKS_H

#include "ulu_abi.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*ulu_log_callback_v1)(void* user, int level, ulu_string_view message);
typedef void (*ulu_progress_callback_v1)(void* user, ulu_string_view event_json);

typedef struct ulu_callbacks_v1 {
    ulu_size struct_size;
    void* user;
    ulu_log_callback_v1 log;
    ulu_progress_callback_v1 progress;
} ulu_callbacks_v1;

#ifdef __cplusplus
}
#endif

#endif
