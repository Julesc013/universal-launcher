// SPDX-FileCopyrightText: 2026 Jules C
// SPDX-License-Identifier: MIT

#ifndef ULK_INSTANCE_H
#define ULK_INSTANCE_H

#include "ulk_types.h"

typedef struct ulk_instance_ref_v1 {
    ulk_size struct_size;
    ulk_string_view instance_id;
    ulk_string_view install_id;
} ulk_instance_ref_v1;

typedef struct ulk_instance_ref_v2 {
    ulk_size struct_size;
    ulk_string_view instance_id;
    ulk_string_view product_id;
    ulk_string_view install_id;
    ulk_string_view profile_id;
    ulk_string_view artifact_set_id;
    ulk_string_view binding_revision;
} ulk_instance_ref_v2;

#endif
