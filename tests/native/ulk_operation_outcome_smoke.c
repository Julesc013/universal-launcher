// SPDX-FileCopyrightText: 2026 Jules C
// SPDX-License-Identifier: MIT

#include "ulk/ulk_api.h"

#include <string.h>

#define CHECK(condition, code) do { if (!(condition)) return (code); } while (0)

static ulk_string_view view(const char* text)
{
    ulk_string_view result;
    result.data = text;
    result.size = text == 0 ? 0u : (ulk_size)strlen(text);
    return result;
}

static void fill_result(
    ulk_operation_result_v1* result,
    ulk_operation_outcome_v1 outcome
)
{
    memset(result, 0, sizeof(*result));
    result->struct_size = sizeof(*result);
    result->identity.struct_size = sizeof(result->identity);
    result->identity.operation_id = view("operation-550e8400-e29b-41d4-a716-446655440000");
    result->identity.attempt_id = view("attempt-550e8400-e29b-41d4-a716-446655440001");
    result->outcome = outcome;
    result->recovery.struct_size = sizeof(result->recovery);
}

static int name_is(ulk_operation_outcome_v1 outcome, const char* expected)
{
    const ulk_string_view name = ulk_operation_outcome_name_v1(outcome);
    const size_t expected_size = strlen(expected);
    return
        name.size == (ulk_size)expected_size &&
        memcmp(name.data, expected, expected_size) == 0;
}

int main(void)
{
    ulk_operation_result_v1 result;
    ulk_operation_identity_v1 identity;

    memset(&identity, 0, sizeof(identity));
    identity.struct_size = sizeof(identity);
    identity.operation_id = view("operation-1");
    identity.attempt_id = view("attempt-1");
    CHECK(ulk_operation_identity_validate_v1(&identity) == ULK_STATUS_OK, 1);
    CHECK(ulk_operation_identity_validate_v1(0) == ULK_STATUS_INVALID_ARGUMENT, 2);
    identity.struct_size = sizeof(identity) - 1u;
    CHECK(ulk_operation_identity_validate_v1(&identity) == ULK_STATUS_INVALID_ARGUMENT, 3);
    identity.struct_size = sizeof(identity);
    identity.operation_id = view(0);
    CHECK(ulk_operation_identity_validate_v1(&identity) == ULK_STATUS_INVALID_ARGUMENT, 4);
    identity.operation_id = view("-operation");
    CHECK(ulk_operation_identity_validate_v1(&identity) == ULK_STATUS_INVALID_ARGUMENT, 5);
    identity.operation_id = view("operation/1");
    CHECK(ulk_operation_identity_validate_v1(&identity) == ULK_STATUS_INVALID_ARGUMENT, 6);
    identity.operation_id = view("operation-1");
    identity.attempt_id.data = 0;
    identity.attempt_id.size = 1u;
    CHECK(ulk_operation_identity_validate_v1(&identity) == ULK_STATUS_INVALID_ARGUMENT, 7);

    fill_result(&result, ULK_OPERATION_CANCELLED_BEFORE_DISPATCH);
    CHECK(ulk_operation_result_validate_v1(&result) == ULK_STATUS_OK, 8);
    result.effects_may_have_occurred = 1;
    CHECK(ulk_operation_result_validate_v1(&result) == ULK_STATUS_INVALID_ARGUMENT, 9);

    fill_result(&result, ULK_OPERATION_REFUSED_BEFORE_EFFECTS);
    CHECK(ulk_operation_result_validate_v1(&result) == ULK_STATUS_OK, 10);
    result.recovery.required = 1;
    result.recovery.inspect_command = view("workspace.recovery.inspect");
    CHECK(ulk_operation_result_validate_v1(&result) == ULK_STATUS_INVALID_ARGUMENT, 11);

    fill_result(&result, ULK_OPERATION_COMPLETED);
    result.effects_may_have_occurred = 1;
    CHECK(ulk_operation_result_validate_v1(&result) == ULK_STATUS_OK, 12);
    result.recovery.transaction_id = view("transaction-1");
    CHECK(ulk_operation_result_validate_v1(&result) == ULK_STATUS_INVALID_ARGUMENT, 13);

    fill_result(&result, ULK_OPERATION_CANCELLATION_REQUESTED_BUT_COMPLETED);
    result.effects_may_have_occurred = 1;
    CHECK(ulk_operation_result_validate_v1(&result) == ULK_STATUS_OK, 14);

    fill_result(&result, ULK_OPERATION_RECOVERY_REQUIRED);
    result.effects_may_have_occurred = 1;
    result.recovery.required = 1;
    result.recovery.transaction_id = view("transaction-1");
    result.recovery.inspect_command = view("workspace.recovery.inspect");
    CHECK(ulk_operation_result_validate_v1(&result) == ULK_STATUS_OK, 15);
    result.recovery.inspect_command = view(0);
    CHECK(ulk_operation_result_validate_v1(&result) == ULK_STATUS_INVALID_ARGUMENT, 16);
    result.recovery.inspect_command = view("Workspace.Recovery.Inspect");
    CHECK(ulk_operation_result_validate_v1(&result) == ULK_STATUS_INVALID_ARGUMENT, 17);
    result.recovery.inspect_command = view("recovery");
    CHECK(ulk_operation_result_validate_v1(&result) == ULK_STATUS_INVALID_ARGUMENT, 34);
    result.recovery.inspect_command = view("workspace.1inspect");
    CHECK(ulk_operation_result_validate_v1(&result) == ULK_STATUS_INVALID_ARGUMENT, 35);
    result.recovery.inspect_command = view("workspace.recovery.inspect");
    result.effects_may_have_occurred = 0;
    CHECK(ulk_operation_result_validate_v1(&result) == ULK_STATUS_INVALID_ARGUMENT, 18);

    fill_result(&result, ULK_OPERATION_OUTCOME_UNKNOWN);
    result.effects_may_have_occurred = 1;
    result.recovery.required = 1;
    result.recovery.inspect_command = view("workspace.recovery.inspect");
    CHECK(ulk_operation_result_validate_v1(&result) == ULK_STATUS_OK, 19);
    result.recovery.transaction_id = view("transaction/1");
    CHECK(ulk_operation_result_validate_v1(&result) == ULK_STATUS_INVALID_ARGUMENT, 20);
    result.recovery.transaction_id = view(0);
    result.recovery.required = 0;
    CHECK(ulk_operation_result_validate_v1(&result) == ULK_STATUS_INVALID_ARGUMENT, 21);

    fill_result(&result, (ulk_operation_outcome_v1)99);
    CHECK(ulk_operation_result_validate_v1(&result) == ULK_STATUS_INVALID_ARGUMENT, 22);
    fill_result(&result, ULK_OPERATION_COMPLETED);
    result.struct_size = sizeof(result) - 1u;
    CHECK(ulk_operation_result_validate_v1(&result) == ULK_STATUS_INVALID_ARGUMENT, 23);
    result.struct_size = sizeof(result);
    result.identity.struct_size = sizeof(result.identity) - 1u;
    CHECK(ulk_operation_result_validate_v1(&result) == ULK_STATUS_INVALID_ARGUMENT, 24);
    result.identity.struct_size = sizeof(result.identity);
    result.recovery.struct_size = sizeof(result.recovery) - 1u;
    CHECK(ulk_operation_result_validate_v1(&result) == ULK_STATUS_INVALID_ARGUMENT, 25);
    result.recovery.struct_size = sizeof(result.recovery);
    result.effects_may_have_occurred = 2;
    CHECK(ulk_operation_result_validate_v1(&result) == ULK_STATUS_INVALID_ARGUMENT, 26);

    CHECK(name_is(ULK_OPERATION_CANCELLED_BEFORE_DISPATCH, "cancelled_before_dispatch"), 27);
    CHECK(name_is(ULK_OPERATION_REFUSED_BEFORE_EFFECTS, "refused_before_effects"), 28);
    CHECK(name_is(ULK_OPERATION_COMPLETED, "completed"), 29);
    CHECK(name_is(
        ULK_OPERATION_CANCELLATION_REQUESTED_BUT_COMPLETED,
        "cancellation_requested_but_completed"), 30);
    CHECK(name_is(ULK_OPERATION_RECOVERY_REQUIRED, "recovery_required"), 31);
    CHECK(name_is(ULK_OPERATION_OUTCOME_UNKNOWN, "outcome_unknown"), 32);
    CHECK(name_is((ulk_operation_outcome_v1)99, "unknown"), 33);
    return 0;
}
