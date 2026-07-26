// SPDX-FileCopyrightText: 2026 Jules C
// SPDX-License-Identifier: MIT

#ifndef ULK_LAUNCH_PLAN_H
#define ULK_LAUNCH_PLAN_H

#include "ulk_types.h"

typedef struct ulk_launch_plan_ref_v1 {
    ulk_size struct_size;
    ulk_string_view instance_id;
    ulk_string_view profile_id;
    ulk_string_view json_payload;
} ulk_launch_plan_ref_v1;

typedef struct ulk_launch_plan_ref_v2 {
    ulk_size struct_size;
    ulk_string_view plan_id;
    ulk_string_view product_id;
    ulk_string_view instance_id;
    ulk_string_view install_id;
    ulk_string_view profile_id;
    ulk_string_view artifact_set_id;
    ulk_string_view install_state_revision;
    ulk_string_view instance_binding_revision;
    ulk_string_view composition_digest;
} ulk_launch_plan_ref_v2;

#endif
