// SPDX-FileCopyrightText: 2026 Jules C
// SPDX-License-Identifier: MIT

#ifndef ULK_SETUP_HANDOFF_H
#define ULK_SETUP_HANDOFF_H

#include "ulk_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum ulk_setup_operation_v1 {
    ULK_SETUP_OPERATION_INSTALL = 1,
    ULK_SETUP_OPERATION_VERIFY = 2,
    ULK_SETUP_OPERATION_REPAIR = 3,
    ULK_SETUP_OPERATION_MOVE = 4,
    ULK_SETUP_OPERATION_UNINSTALL = 5,
    ULK_SETUP_OPERATION_RECOVERY = 6
} ulk_setup_operation_v1;

typedef enum ulk_setup_result_status_v1 {
    ULK_SETUP_RESULT_COMPLETED = 1,
    ULK_SETUP_RESULT_REFUSED = 2,
    ULK_SETUP_RESULT_FAILED = 3,
    ULK_SETUP_RESULT_RECOVERY_REQUIRED = 4
} ulk_setup_result_status_v1;

typedef enum ulk_install_ownership_v1 {
    ULK_INSTALL_OWNERSHIP_MANAGED = 1,
    ULK_INSTALL_OWNERSHIP_IMPORTED = 2,
    ULK_INSTALL_OWNERSHIP_FOREIGN_OWNED = 3
} ulk_install_ownership_v1;

typedef enum ulk_install_lifecycle_v1 {
    ULK_INSTALL_LIFECYCLE_ACTIVE = 1,
    ULK_INSTALL_LIFECYCLE_VERIFICATION_FAILED = 2,
    ULK_INSTALL_LIFECYCLE_RECOVERY_REQUIRED = 3,
    ULK_INSTALL_LIFECYCLE_RETIRED = 4,
    ULK_INSTALL_LIFECYCLE_UNINSTALLED = 5
} ulk_install_lifecycle_v1;

typedef enum ulk_install_refresh_transition_v1 {
    ULK_INSTALL_REFRESH_CREATED = 1,
    ULK_INSTALL_REFRESH_REFRESHED = 2,
    ULK_INSTALL_REFRESH_ARCHIVED = 3,
    ULK_INSTALL_REFRESH_UNCHANGED = 4
} ulk_install_refresh_transition_v1;

typedef enum ulk_dependent_instance_status_v1 {
    ULK_DEPENDENT_INSTANCE_READY = 1,
    ULK_DEPENDENT_INSTANCE_INSTALL_CHANGED = 2,
    ULK_DEPENDENT_INSTANCE_INSTALL_UNAVAILABLE = 3,
    ULK_DEPENDENT_INSTANCE_RECOVERY_REQUIRED = 4
} ulk_dependent_instance_status_v1;

typedef enum ulk_launch_plan_status_v1 {
    ULK_LAUNCH_PLAN_FRESH = 1,
    ULK_LAUNCH_PLAN_STALE = 2
} ulk_launch_plan_status_v1;

typedef struct ulk_setup_plan_reference_v1 {
    ulk_size struct_size;
    ulk_setup_operation_v1 operation;
    ulk_string_view plan_id;
    ulk_string_view plan_digest;
    ulk_string_view input_identity_digest;
    ulk_string_view product_id;
    ulk_string_view install_id;
    ulk_string_view setup_provider;
    ulk_string_view provider_revision;
    ulk_bool reviewed;
} ulk_setup_plan_reference_v1;

typedef struct ulk_setup_apply_request_v1 {
    ulk_size struct_size;
    ulk_setup_operation_v1 operation;
    ulk_string_view plan_id;
    ulk_string_view plan_digest;
    ulk_string_view input_identity_digest;
    ulk_string_view product_id;
    ulk_string_view install_id;
    ulk_string_view setup_provider;
    ulk_string_view provider_revision;
} ulk_setup_apply_request_v1;

typedef struct ulk_installed_state_reference_v1 {
    ulk_size struct_size;
    ulk_string_view setup_state_ref;
    ulk_string_view install_id;
    ulk_string_view product_id;
    ulk_string_view exact_product_version;
    ulk_string_view entrypoint;
    ulk_string_view capabilities_json;
    ulk_install_lifecycle_v1 lifecycle_status;
    ulk_string_view last_verification_identity;
    ulk_string_view state_revision;
} ulk_installed_state_reference_v1;

typedef struct ulk_setup_result_v1 {
    ulk_size struct_size;
    ulk_setup_operation_v1 operation;
    ulk_setup_result_status_v1 status;
    ulk_string_view result_id;
    ulk_string_view plan_id;
    ulk_string_view plan_digest;
    ulk_string_view product_id;
    ulk_string_view setup_provider;
    ulk_string_view provider_revision;
    ulk_string_view audit_identity;
    const ulk_installed_state_reference_v1* installed_state;
} ulk_setup_result_v1;

typedef struct ulk_install_reference_v2 {
    ulk_size struct_size;
    ulk_string_view install_id;
    ulk_string_view product_id;
    ulk_install_ownership_v1 ownership;
    ulk_string_view setup_state_ref;
    ulk_string_view exact_product_version;
    ulk_string_view entrypoint;
    ulk_string_view capabilities_json;
    ulk_install_lifecycle_v1 lifecycle_status;
    ulk_string_view last_verification_identity;
    ulk_string_view state_revision;
} ulk_install_reference_v2;

typedef struct ulk_install_reference_refresh_v1 {
    ulk_size struct_size;
    ulk_bool has_install_reference;
    ulk_install_refresh_transition_v1 transition;
    ulk_install_reference_v2 install_reference;
    ulk_dependent_instance_status_v1 dependent_instance_status;
    ulk_launch_plan_status_v1 launch_plan_status;
} ulk_install_reference_refresh_v1;

typedef struct ulk_launch_plan_install_binding_v1 {
    ulk_size struct_size;
    ulk_string_view install_id;
    ulk_string_view state_revision;
    ulk_string_view verification_identity;
} ulk_launch_plan_install_binding_v1;

typedef struct ulk_setup_authority_status_v1 {
    ulk_size struct_size;
    ulk_string_view mutation_owner;
    ulk_string_view launcher_role;
    uint32_t allowed_operation_mask;
    ulk_bool launcher_can_mutate_setup;
} ulk_setup_authority_status_v1;

ULK_API int ULK_CALL ulk_setup_apply_request_validate_v1(
    const ulk_setup_apply_request_v1* request,
    const ulk_setup_plan_reference_v1* reviewed_plan
);

ULK_API int ULK_CALL ulk_setup_result_project_v1(
    const ulk_setup_plan_reference_v1* reviewed_plan,
    const ulk_install_reference_v2* current_reference,
    const ulk_setup_result_v1* result,
    ulk_install_reference_refresh_v1* refresh
);

ULK_API int ULK_CALL ulk_launch_plan_install_status_v1(
    const ulk_launch_plan_install_binding_v1* binding,
    const ulk_install_reference_v2* current_reference,
    ulk_launch_plan_status_v1* status
);

ULK_API int ULK_CALL ulk_dependent_instance_status_code_v1(
    ulk_dependent_instance_status_v1 status,
    ulk_string_view* status_code
);

ULK_API int ULK_CALL ulk_setup_authority_status_get_v1(
    ulk_setup_authority_status_v1* status
);

#ifdef __cplusplus
}
#endif

#endif
