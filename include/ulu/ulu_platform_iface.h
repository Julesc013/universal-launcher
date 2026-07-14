// SPDX-FileCopyrightText: 2026 Jules C
// SPDX-License-Identifier: MIT

#ifndef ULU_PLATFORM_IFACE_H
#define ULU_PLATFORM_IFACE_H

#include "ulu_abi.h"

typedef struct ulu_platform_iface_v1 {
    ulu_size struct_size;
    void* user;
    int (*resolve_user_data_root)(void* user, ulu_string_view product_id, char* out_path, ulu_size out_path_size);
} ulu_platform_iface_v1;

#endif
