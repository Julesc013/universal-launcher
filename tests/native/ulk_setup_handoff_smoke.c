// SPDX-FileCopyrightText: 2026 Jules C
// SPDX-License-Identifier: MIT

#include "ulk/ulk_api.h"

#include <stdio.h>
#include <string.h>

#define CHECK(condition, code) \
    do { \
        if (!(condition)) { \
            fprintf(stderr, "setup handoff check failed at line %d (code %d)\n", __LINE__, code); \
            return code; \
        } \
    } while (0)

static ulk_string_view view(const char* text)
{
    ulk_string_view result;
    result.data = text;
    result.size = text == 0 ? 0 : (ulk_size)strlen(text);
    return result;
}

static void fill_plan(
    ulk_setup_plan_reference_v1* plan,
    ulk_setup_operation_v1 operation
)
{
    memset(plan, 0, sizeof(*plan));
    plan->struct_size = sizeof(*plan);
    plan->operation = operation;
    plan->plan_id = view("plan-001");
    plan->plan_digest = view("sha256:plan");
    plan->input_identity_digest = view("sha256:inputs");
    plan->product_id = view("fixture.product");
    plan->install_id = operation == ULK_SETUP_OPERATION_INSTALL
        ? view(0)
        : view("install-001");
    plan->setup_provider = view("universal-setup");
    plan->provider_revision = view("setup-revision-001");
    plan->reviewed = 1;
}

static void fill_request(
    ulk_setup_apply_request_v1* request,
    const ulk_setup_plan_reference_v1* plan
)
{
    memset(request, 0, sizeof(*request));
    request->struct_size = sizeof(*request);
    request->operation = plan->operation;
    request->plan_id = plan->plan_id;
    request->plan_digest = plan->plan_digest;
    request->input_identity_digest = plan->input_identity_digest;
    request->product_id = plan->product_id;
    request->install_id = plan->install_id;
    request->setup_provider = plan->setup_provider;
    request->provider_revision = plan->provider_revision;
}

static void fill_state(ulk_installed_state_reference_v1* state)
{
    memset(state, 0, sizeof(*state));
    state->struct_size = sizeof(*state);
    state->setup_state_ref = view("state://install-001/revision-002");
    state->install_id = view("install-001");
    state->product_id = view("fixture.product");
    state->exact_product_version = view("2.0.77");
    state->entrypoint = view("entrypoint://primary");
    state->capabilities_json = view("[\"launch.preview\"]");
    state->lifecycle_status = ULK_INSTALL_LIFECYCLE_ACTIVE;
    state->last_verification_identity = view("verify-002");
    state->state_revision = view("revision-002");
}

static void fill_current(ulk_install_reference_v2* current)
{
    memset(current, 0, sizeof(*current));
    current->struct_size = sizeof(*current);
    current->install_id = view("install-001");
    current->product_id = view("fixture.product");
    current->ownership = ULK_INSTALL_OWNERSHIP_MANAGED;
    current->setup_state_ref = view("state://install-001/revision-001");
    current->exact_product_version = view("2.0.77");
    current->entrypoint = view("entrypoint://primary");
    current->capabilities_json = view("[\"launch.preview\"]");
    current->lifecycle_status = ULK_INSTALL_LIFECYCLE_ACTIVE;
    current->last_verification_identity = view("verify-001");
    current->state_revision = view("revision-001");
}

static void fill_result(
    ulk_setup_result_v1* result,
    const ulk_setup_plan_reference_v1* plan,
    ulk_setup_result_status_v1 status,
    const ulk_installed_state_reference_v1* state
)
{
    memset(result, 0, sizeof(*result));
    result->struct_size = sizeof(*result);
    result->operation = plan->operation;
    result->status = status;
    result->result_id = view("result-001");
    result->plan_id = plan->plan_id;
    result->plan_digest = plan->plan_digest;
    result->product_id = plan->product_id;
    result->setup_provider = plan->setup_provider;
    result->provider_revision = plan->provider_revision;
    result->audit_identity = status == ULK_SETUP_RESULT_COMPLETED
        ? view("audit-001")
        : view(0);
    result->installed_state = state;
}

static int test_apply_identity_binding(void)
{
    ulk_setup_plan_reference_v1 plan;
    ulk_setup_apply_request_v1 request;

    fill_plan(&plan, ULK_SETUP_OPERATION_REPAIR);
    fill_request(&request, &plan);
    CHECK(ulk_setup_apply_request_validate_v1(&request, &plan) == ULK_STATUS_OK, 10);

    request.plan_digest = view("sha256:changed-plan");
    CHECK(ulk_setup_apply_request_validate_v1(&request, &plan) == ULK_STATUS_ERROR, 11);
    fill_request(&request, &plan);
    request.input_identity_digest = view("sha256:changed-inputs");
    CHECK(ulk_setup_apply_request_validate_v1(&request, &plan) == ULK_STATUS_ERROR, 12);
    fill_request(&request, &plan);
    request.provider_revision = view("setup-revision-002");
    CHECK(ulk_setup_apply_request_validate_v1(&request, &plan) == ULK_STATUS_ERROR, 13);
    fill_request(&request, &plan);
    request.install_id = view("other-install");
    CHECK(ulk_setup_apply_request_validate_v1(&request, &plan) == ULK_STATUS_ERROR, 14);
    fill_request(&request, &plan);
    plan.reviewed = 0;
    CHECK(
        ulk_setup_apply_request_validate_v1(&request, &plan) ==
        ULK_STATUS_INVALID_ARGUMENT,
        15);
    return 0;
}

static int test_install_creation(void)
{
    ulk_setup_plan_reference_v1 plan;
    ulk_installed_state_reference_v1 state;
    ulk_setup_result_v1 result;
    ulk_install_reference_refresh_v1 refresh;

    fill_plan(&plan, ULK_SETUP_OPERATION_INSTALL);
    fill_state(&state);
    fill_result(&result, &plan, ULK_SETUP_RESULT_COMPLETED, &state);
    memset(&refresh, 0, sizeof(refresh));
    refresh.struct_size = sizeof(refresh);
    CHECK(ulk_setup_result_project_v1(&plan, 0, &result, &refresh) == ULK_STATUS_OK, 20);
    CHECK(refresh.has_install_reference, 21);
    CHECK(refresh.transition == ULK_INSTALL_REFRESH_CREATED, 22);
    CHECK(refresh.install_reference.ownership == ULK_INSTALL_OWNERSHIP_MANAGED, 23);
    CHECK(
        refresh.install_reference.lifecycle_status == ULK_INSTALL_LIFECYCLE_ACTIVE,
        24);
    CHECK(refresh.launch_plan_status == ULK_LAUNCH_PLAN_STALE, 25);
    return 0;
}

static int test_repair_and_move_law(void)
{
    ulk_setup_plan_reference_v1 plan;
    ulk_installed_state_reference_v1 state;
    ulk_install_reference_v2 current;
    ulk_setup_result_v1 result;
    ulk_install_reference_refresh_v1 refresh;

    fill_plan(&plan, ULK_SETUP_OPERATION_REPAIR);
    fill_state(&state);
    fill_current(&current);
    fill_result(&result, &plan, ULK_SETUP_RESULT_COMPLETED, &state);
    memset(&refresh, 0, sizeof(refresh));
    refresh.struct_size = sizeof(refresh);
    CHECK(
        ulk_setup_result_project_v1(&plan, &current, &result, &refresh) ==
        ULK_STATUS_OK,
        30);
    CHECK(refresh.transition == ULK_INSTALL_REFRESH_REFRESHED, 31);
    CHECK(refresh.dependent_instance_status == ULK_DEPENDENT_INSTANCE_INSTALL_CHANGED, 32);
    CHECK(refresh.launch_plan_status == ULK_LAUNCH_PLAN_STALE, 33);

    state.exact_product_version = view("2.0.78");
    refresh.struct_size = sizeof(refresh);
    CHECK(
        ulk_setup_result_project_v1(&plan, &current, &result, &refresh) ==
        ULK_STATUS_ERROR,
        34);

    fill_plan(&plan, ULK_SETUP_OPERATION_MOVE);
    fill_state(&state);
    state.lifecycle_status = ULK_INSTALL_LIFECYCLE_VERIFICATION_FAILED;
    fill_result(&result, &plan, ULK_SETUP_RESULT_COMPLETED, &state);
    refresh.struct_size = sizeof(refresh);
    CHECK(
        ulk_setup_result_project_v1(&plan, &current, &result, &refresh) ==
        ULK_STATUS_ERROR,
        35);

    current.ownership = ULK_INSTALL_OWNERSHIP_FOREIGN_OWNED;
    state.lifecycle_status = ULK_INSTALL_LIFECYCLE_ACTIVE;
    refresh.struct_size = sizeof(refresh);
    CHECK(
        ulk_setup_result_project_v1(&plan, &current, &result, &refresh) ==
        ULK_STATUS_ERROR,
        36);
    return 0;
}

static int test_result_status_projection(void)
{
    ulk_setup_plan_reference_v1 plan;
    ulk_install_reference_v2 current;
    ulk_setup_result_v1 result;
    ulk_install_reference_refresh_v1 refresh;

    fill_plan(&plan, ULK_SETUP_OPERATION_REPAIR);
    fill_current(&current);
    fill_result(&result, &plan, ULK_SETUP_RESULT_REFUSED, 0);
    memset(&refresh, 0, sizeof(refresh));
    refresh.struct_size = sizeof(refresh);
    CHECK(
        ulk_setup_result_project_v1(&plan, &current, &result, &refresh) ==
        ULK_STATUS_OK,
        40);
    CHECK(refresh.transition == ULK_INSTALL_REFRESH_UNCHANGED, 41);
    CHECK(refresh.launch_plan_status == ULK_LAUNCH_PLAN_FRESH, 42);

    fill_result(&result, &plan, ULK_SETUP_RESULT_RECOVERY_REQUIRED, 0);
    refresh.struct_size = sizeof(refresh);
    CHECK(
        ulk_setup_result_project_v1(&plan, &current, &result, &refresh) ==
        ULK_STATUS_OK,
        43);
    CHECK(
        refresh.install_reference.lifecycle_status ==
        ULK_INSTALL_LIFECYCLE_RECOVERY_REQUIRED,
        44);
    CHECK(
        refresh.dependent_instance_status ==
        ULK_DEPENDENT_INSTANCE_RECOVERY_REQUIRED,
        45);
    CHECK(refresh.launch_plan_status == ULK_LAUNCH_PLAN_STALE, 46);

    result.plan_digest = view("sha256:other");
    refresh.struct_size = sizeof(refresh);
    CHECK(
        ulk_setup_result_project_v1(&plan, &current, &result, &refresh) ==
        ULK_STATUS_INVALID_ARGUMENT,
        47);
    return 0;
}

static int test_uninstall_and_launch_staleness(void)
{
    ulk_setup_plan_reference_v1 plan;
    ulk_install_reference_v2 current;
    ulk_setup_result_v1 result;
    ulk_install_reference_refresh_v1 refresh;
    ulk_launch_plan_install_binding_v1 binding;
    ulk_launch_plan_status_v1 status;

    fill_plan(&plan, ULK_SETUP_OPERATION_UNINSTALL);
    fill_current(&current);
    fill_result(&result, &plan, ULK_SETUP_RESULT_COMPLETED, 0);
    memset(&refresh, 0, sizeof(refresh));
    refresh.struct_size = sizeof(refresh);
    CHECK(
        ulk_setup_result_project_v1(&plan, &current, &result, &refresh) ==
        ULK_STATUS_OK,
        50);
    CHECK(refresh.transition == ULK_INSTALL_REFRESH_ARCHIVED, 51);
    CHECK(
        refresh.install_reference.lifecycle_status == ULK_INSTALL_LIFECYCLE_UNINSTALLED,
        52);
    CHECK(
        refresh.dependent_instance_status == ULK_DEPENDENT_INSTANCE_INSTALL_UNAVAILABLE,
        53);

    memset(&binding, 0, sizeof(binding));
    binding.struct_size = sizeof(binding);
    binding.install_id = view("install-001");
    binding.state_revision = view("revision-001");
    binding.verification_identity = view("verify-001");
    CHECK(
        ulk_launch_plan_install_status_v1(&binding, &current, &status) == ULK_STATUS_OK,
        54);
    CHECK(status == ULK_LAUNCH_PLAN_FRESH, 55);
    current.state_revision = view("revision-002");
    CHECK(
        ulk_launch_plan_install_status_v1(&binding, &current, &status) == ULK_STATUS_OK,
        56);
    CHECK(status == ULK_LAUNCH_PLAN_STALE, 57);
    return 0;
}

static int test_authority_status(void)
{
    ulk_setup_authority_status_v1 status;
    memset(&status, 0, sizeof(status));
    status.struct_size = sizeof(status);
    CHECK(ulk_setup_authority_status_get_v1(&status) == ULK_STATUS_OK, 60);
    CHECK(ULK_API_VERSION_MAJOR == 1 && ULK_API_VERSION_MINOR == 2, 65);
    CHECK(status.allowed_operation_mask == 0x3fu, 61);
    CHECK(!status.launcher_can_mutate_setup, 62);
    CHECK(status.mutation_owner.size == strlen("universal-setup"), 63);
    CHECK(
        memcmp(status.mutation_owner.data, "universal-setup", status.mutation_owner.size) == 0,
        64);
    return 0;
}

int main(void)
{
    int status = test_apply_identity_binding();
    if (status != 0) {
        return status;
    }
    status = test_install_creation();
    if (status != 0) {
        return status;
    }
    status = test_repair_and_move_law();
    if (status != 0) {
        return status;
    }
    status = test_result_status_projection();
    if (status != 0) {
        return status;
    }
    status = test_uninstall_and_launch_staleness();
    if (status != 0) {
        return status;
    }
    return test_authority_status();
}
