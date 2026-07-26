// SPDX-FileCopyrightText: 2026 Jules C
// SPDX-License-Identifier: MIT

#include "ulk/ulk_operation.h"

#include <string.h>

static int ulk_view_is_valid(ulk_string_view value)
{
    return value.size == 0u || value.data != 0;
}

static int ulk_identifier_is_valid(ulk_string_view value, int allow_empty)
{
    ulk_size index;
    unsigned char first;
    if (!ulk_view_is_valid(value)) {
        return 0;
    }
    if (value.size == 0u) {
        return allow_empty;
    }
    if (value.size > 128u) {
        return 0;
    }
    first = (unsigned char)value.data[0];
    if (
        !(
            (first >= (unsigned char)'a' && first <= (unsigned char)'z') ||
            (first >= (unsigned char)'A' && first <= (unsigned char)'Z') ||
            (first >= (unsigned char)'0' && first <= (unsigned char)'9')
        )
    ) {
        return 0;
    }
    for (index = 0u; index < value.size; ++index) {
        const unsigned char character = (unsigned char)value.data[index];
        const int alpha =
            (character >= (unsigned char)'a' && character <= (unsigned char)'z') ||
            (character >= (unsigned char)'A' && character <= (unsigned char)'Z');
        const int digit =
            character >= (unsigned char)'0' && character <= (unsigned char)'9';
        if (
            !alpha &&
            !digit &&
            character != (unsigned char)'.' &&
            character != (unsigned char)'_' &&
            character != (unsigned char)':' &&
            character != (unsigned char)'-'
        ) {
            return 0;
        }
    }
    return 1;
}

static int ulk_command_name_is_valid(ulk_string_view value)
{
    ulk_size index;
    int previous_was_dot = 0;
    int saw_dot = 0;
    if (!ulk_view_is_valid(value) || value.size == 0u || value.size > 128u) {
        return 0;
    }
    if (
        value.data[0] < 'a' ||
        value.data[0] > 'z'
    ) {
        return 0;
    }
    for (index = 0u; index < value.size; ++index) {
        const unsigned char character = (unsigned char)value.data[index];
        const int lower =
            character >= (unsigned char)'a' && character <= (unsigned char)'z';
        const int digit =
            character >= (unsigned char)'0' && character <= (unsigned char)'9';
        if (character == (unsigned char)'.') {
            if (index == 0u || index + 1u == value.size || previous_was_dot) {
                return 0;
            }
            previous_was_dot = 1;
            saw_dot = 1;
            continue;
        }
        if (previous_was_dot && !lower) {
            return 0;
        }
        if (
            !lower &&
            !digit &&
            character != (unsigned char)'_'
        ) {
            return 0;
        }
        previous_was_dot = 0;
    }
    return saw_dot;
}

static int ulk_outcome_is_valid(ulk_operation_outcome_v1 outcome)
{
    return
        outcome == ULK_OPERATION_CANCELLED_BEFORE_DISPATCH ||
        outcome == ULK_OPERATION_REFUSED_BEFORE_EFFECTS ||
        outcome == ULK_OPERATION_COMPLETED ||
        outcome == ULK_OPERATION_CANCELLATION_REQUESTED_BUT_COMPLETED ||
        outcome == ULK_OPERATION_RECOVERY_REQUIRED ||
        outcome == ULK_OPERATION_OUTCOME_UNKNOWN;
}

int ULK_CALL ulk_operation_identity_validate_v1(
    const ulk_operation_identity_v1* identity
)
{
    if (
        identity == 0 ||
        identity->struct_size < (ulk_size)sizeof(*identity) ||
        !ulk_identifier_is_valid(identity->operation_id, 0) ||
        !ulk_identifier_is_valid(identity->attempt_id, 0)
    ) {
        return ULK_STATUS_INVALID_ARGUMENT;
    }
    return ULK_STATUS_OK;
}

int ULK_CALL ulk_operation_result_validate_v1(
    const ulk_operation_result_v1* result
)
{
    int pre_effect_outcome;
    int uncertain_outcome;
    if (
        result == 0 ||
        result->struct_size < (ulk_size)sizeof(*result) ||
        ulk_operation_identity_validate_v1(&result->identity) != ULK_STATUS_OK ||
        !ulk_outcome_is_valid(result->outcome) ||
        (result->effects_may_have_occurred != 0 &&
         result->effects_may_have_occurred != 1) ||
        result->recovery.struct_size < (ulk_size)sizeof(result->recovery) ||
        (result->recovery.required != 0 && result->recovery.required != 1) ||
        !ulk_identifier_is_valid(result->recovery.transaction_id, 1) ||
        !ulk_view_is_valid(result->recovery.inspect_command)
    ) {
        return ULK_STATUS_INVALID_ARGUMENT;
    }

    pre_effect_outcome =
        result->outcome == ULK_OPERATION_CANCELLED_BEFORE_DISPATCH ||
        result->outcome == ULK_OPERATION_REFUSED_BEFORE_EFFECTS;
    uncertain_outcome =
        result->outcome == ULK_OPERATION_RECOVERY_REQUIRED ||
        result->outcome == ULK_OPERATION_OUTCOME_UNKNOWN;

    if (
        pre_effect_outcome &&
        (result->effects_may_have_occurred != 0 || result->recovery.required != 0)
    ) {
        return ULK_STATUS_INVALID_ARGUMENT;
    }
    if (
        uncertain_outcome &&
        (
            result->effects_may_have_occurred != 1 ||
            result->recovery.required != 1 ||
            !ulk_command_name_is_valid(result->recovery.inspect_command)
        )
    ) {
        return ULK_STATUS_INVALID_ARGUMENT;
    }
    if (
        !uncertain_outcome &&
        (
            result->recovery.required != 0 ||
            result->recovery.transaction_id.size != 0u ||
            result->recovery.inspect_command.size != 0u
        )
    ) {
        return ULK_STATUS_INVALID_ARGUMENT;
    }
    return ULK_STATUS_OK;
}

ulk_string_view ULK_CALL ulk_operation_outcome_name_v1(
    ulk_operation_outcome_v1 outcome
)
{
    const char* name = "unknown";
    ulk_string_view result;
    if (outcome == ULK_OPERATION_CANCELLED_BEFORE_DISPATCH) {
        name = "cancelled_before_dispatch";
    } else if (outcome == ULK_OPERATION_REFUSED_BEFORE_EFFECTS) {
        name = "refused_before_effects";
    } else if (outcome == ULK_OPERATION_COMPLETED) {
        name = "completed";
    } else if (outcome == ULK_OPERATION_CANCELLATION_REQUESTED_BUT_COMPLETED) {
        name = "cancellation_requested_but_completed";
    } else if (outcome == ULK_OPERATION_RECOVERY_REQUIRED) {
        name = "recovery_required";
    } else if (outcome == ULK_OPERATION_OUTCOME_UNKNOWN) {
        name = "outcome_unknown";
    }
    result.data = name;
    result.size = (ulk_size)strlen(name);
    return result;
}
