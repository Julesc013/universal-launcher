// SPDX-FileCopyrightText: 2026 Jules C
// SPDX-License-Identifier: MIT

#ifndef ULK_OPERATION_H
#define ULK_OPERATION_H

#include "ulk_error.h"
#include "ulk_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum ulk_operation_outcome_v1 {
    ULK_OPERATION_CANCELLED_BEFORE_DISPATCH = 1,
    ULK_OPERATION_REFUSED_BEFORE_EFFECTS = 2,
    ULK_OPERATION_COMPLETED = 3,
    ULK_OPERATION_CANCELLATION_REQUESTED_BUT_COMPLETED = 4,
    ULK_OPERATION_RECOVERY_REQUIRED = 5,
    ULK_OPERATION_OUTCOME_UNKNOWN = 6
} ulk_operation_outcome_v1;

typedef struct ulk_operation_identity_v1 {
    ulk_size struct_size;
    ulk_string_view operation_id;
    ulk_string_view attempt_id;
} ulk_operation_identity_v1;

typedef struct ulk_operation_recovery_v1 {
    ulk_size struct_size;
    ulk_bool required;
    ulk_string_view transaction_id;
    ulk_string_view inspect_command;
} ulk_operation_recovery_v1;

typedef struct ulk_operation_result_v1 {
    ulk_size struct_size;
    ulk_operation_identity_v1 identity;
    ulk_operation_outcome_v1 outcome;
    ulk_bool effects_may_have_occurred;
    ulk_operation_recovery_v1 recovery;
} ulk_operation_result_v1;

ULK_API int ULK_CALL ulk_operation_identity_validate_v1(
    const ulk_operation_identity_v1* identity
);

ULK_API int ULK_CALL ulk_operation_result_validate_v1(
    const ulk_operation_result_v1* result
);

ULK_API ulk_string_view ULK_CALL ulk_operation_outcome_name_v1(
    ulk_operation_outcome_v1 outcome
);

/*
 * String views are borrowed from the producer. Operation IDs identify one
 * durable logical operation; attempt IDs identify one dispatch attempt.
 * A timeout or cancellation after dispatch is never proof that no effects
 * occurred. Recovery-required and outcome-unknown results therefore require a
 * non-empty inspect command and effects_may_have_occurred = true.
 */

#ifdef __cplusplus
}
#endif

#endif
