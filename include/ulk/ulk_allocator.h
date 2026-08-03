// SPDX-FileCopyrightText: 2026 Jules C
// SPDX-License-Identifier: MIT

#ifndef ULK_ALLOCATOR_H
#define ULK_ALLOCATOR_H

#include "ulk_types.h"

typedef void* (ULK_CALL *ulk_alloc_fn_v1)(void* user, ulk_size size);
typedef void (ULK_CALL *ulk_free_fn_v1)(void* user, void* ptr);

typedef struct ulk_allocator_v1 {
    ulk_size struct_size;
    void* user;
    ulk_alloc_fn_v1 alloc;
    ulk_free_fn_v1 free;
} ulk_allocator_v1;

#endif
