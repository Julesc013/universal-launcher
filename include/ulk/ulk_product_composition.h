// SPDX-FileCopyrightText: 2026 Jules C
// SPDX-License-Identifier: MIT

#ifndef ULK_PRODUCT_COMPOSITION_H
#define ULK_PRODUCT_COMPOSITION_H

#include "ulk_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum ulk_launch_capability_kind_v1 {
    ULK_LAUNCH_CAPABILITY_INVALID = 0,
    ULK_LAUNCH_CAPABILITY_SINGLE_PROCESS = 1,
    ULK_LAUNCH_CAPABILITY_OPEN_DOCUMENT = 2,
    ULK_LAUNCH_CAPABILITY_MULTI_INSTANCE = 3,
    ULK_LAUNCH_CAPABILITY_PROFILE_SELECTION = 4,
    ULK_LAUNCH_CAPABILITY_ARTIFACT_SETS = 5,
    ULK_LAUNCH_CAPABILITY_SESSION_SUPERVISION = 6,
    ULK_LAUNCH_CAPABILITY_BACKGROUND_SERVICE = 7,
    ULK_LAUNCH_CAPABILITY_SERVER = 8
} ulk_launch_capability_kind_v1;

typedef struct ulk_contract_set_identity_v1 {
    ulk_size struct_size;
    ulk_string_view contract_set_id;
    ulk_string_view version;
    ulk_string_view sha256;
} ulk_contract_set_identity_v1;

typedef struct ulk_launch_capability_v1 {
    ulk_size struct_size;
    ulk_launch_capability_kind_v1 kind;
    ulk_bool required;
} ulk_launch_capability_v1;

typedef struct ulk_entrypoint_descriptor_v1 {
    ulk_size struct_size;
    ulk_string_view entrypoint_id;
    ulk_string_view relative_path;
    ulk_string_view artifact_set_id;
    const ulk_launch_capability_v1* capabilities;
    ulk_size capability_count;
} ulk_entrypoint_descriptor_v1;

typedef struct ulk_product_descriptor_v2 {
    ulk_size struct_size;
    ulk_string_view product_id;
    ulk_string_view exact_version;
    ulk_string_view publisher_id;
    ulk_string_view composition_revision;
} ulk_product_descriptor_v2;

typedef struct ulk_product_composition_v1 {
    ulk_size struct_size;
    const ulk_product_descriptor_v2* product;
    const ulk_entrypoint_descriptor_v1* entrypoints;
    ulk_size entrypoint_count;
    const ulk_contract_set_identity_v1* contract_set;
} ulk_product_composition_v1;

ULK_API int ULK_CALL ulk_contract_set_identity_validate_v1(
    const ulk_contract_set_identity_v1* identity
);

ULK_API int ULK_CALL ulk_launch_capability_validate_v1(
    const ulk_launch_capability_v1* capability
);

ULK_API int ULK_CALL ulk_entrypoint_descriptor_validate_v1(
    const ulk_entrypoint_descriptor_v1* entrypoint
);

ULK_API int ULK_CALL ulk_product_descriptor_validate_v2(
    const ulk_product_descriptor_v2* product
);

ULK_API int ULK_CALL ulk_product_composition_validate_v1(
    const ulk_product_composition_v1* composition
);

#ifdef __cplusplus
}
#endif

#endif
