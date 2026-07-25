// SPDX-FileCopyrightText: 2026 Jules C
// SPDX-License-Identifier: MIT

#ifndef ULK_REFERENCE_MODEL_H
#define ULK_REFERENCE_MODEL_H

#include "ulk_artifact_set.h"
#include "ulk_error.h"
#include "ulk_instance.h"
#include "ulk_launch_plan.h"
#include "ulk_product.h"
#include "ulk_profile.h"
#include "ulk_setup_handoff.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum ulk_reference_issue_v1 {
    ULK_REFERENCE_ISSUE_NONE = 0,
    ULK_REFERENCE_ISSUE_INVALID_GRAPH = 1,
    ULK_REFERENCE_ISSUE_INVALID_PRODUCT = 2,
    ULK_REFERENCE_ISSUE_INVALID_INSTALL_REFERENCE = 3,
    ULK_REFERENCE_ISSUE_INVALID_INSTANCE = 4,
    ULK_REFERENCE_ISSUE_INVALID_PROFILE = 5,
    ULK_REFERENCE_ISSUE_INVALID_ARTIFACT_SET = 6,
    ULK_REFERENCE_ISSUE_INVALID_LAUNCH_PLAN = 7,
    ULK_REFERENCE_ISSUE_PRODUCT_MISMATCH = 8,
    ULK_REFERENCE_ISSUE_INSTALL_MISMATCH = 9,
    ULK_REFERENCE_ISSUE_INSTANCE_MISMATCH = 10,
    ULK_REFERENCE_ISSUE_PROFILE_MISMATCH = 11,
    ULK_REFERENCE_ISSUE_ARTIFACT_SET_MISMATCH = 12
} ulk_reference_issue_v1;

typedef struct ulk_reference_graph_v1 {
    ulk_size struct_size;
    const ulk_product_ref_v1* product;
    const ulk_install_reference_v2* install_reference;
    const ulk_instance_ref_v2* instance;
    const ulk_profile_ref_v2* profile;
    const ulk_artifact_set_ref_v1* artifact_set;
    const ulk_launch_plan_ref_v2* launch_plan;
} ulk_reference_graph_v1;

typedef struct ulk_reference_validation_v1 {
    ulk_size struct_size;
    ulk_bool valid;
    ulk_reference_issue_v1 issue;
    ulk_launch_plan_status_v1 launch_plan_status;
    ulk_string_view status_code;
} ulk_reference_validation_v1;

ULK_API int ULK_CALL ulk_product_ref_validate_v1(
    const ulk_product_ref_v1* product
);

ULK_API int ULK_CALL ulk_install_reference_validate_v2(
    const ulk_install_reference_v2* install_reference
);

ULK_API int ULK_CALL ulk_instance_ref_validate_v2(
    const ulk_instance_ref_v2* instance
);

ULK_API int ULK_CALL ulk_profile_ref_validate_v2(
    const ulk_profile_ref_v2* profile
);

ULK_API int ULK_CALL ulk_artifact_set_ref_validate_v1(
    const ulk_artifact_set_ref_v1* artifact_set
);

ULK_API int ULK_CALL ulk_launch_plan_ref_validate_v2(
    const ulk_launch_plan_ref_v2* launch_plan
);

ULK_API int ULK_CALL ulk_reference_graph_validate_v1(
    const ulk_reference_graph_v1* graph,
    ulk_reference_validation_v1* validation
);

#ifdef __cplusplus
}
#endif

#endif
