#include "ulk/ulk_setup_handoff.h"

#include "ulk/ulk_error.h"

#include <string.h>

#define ULK_SETUP_TEXT_LIMIT 4096u
#define ULK_SETUP_OPERATION_MASK_ALL 0x3fu

static ulk_string_view ulk_setup_view(const char* value)
{
    ulk_string_view result;
    result.data = value;
    result.size = (ulk_size)strlen(value);
    return result;
}

static int ulk_setup_text_valid(ulk_string_view value, int allow_empty)
{
    ulk_size index;
    if (value.data == 0) {
        return allow_empty && value.size == 0;
    }
    if ((!allow_empty && value.size == 0) || value.size > ULK_SETUP_TEXT_LIMIT) {
        return 0;
    }
    for (index = 0; index < value.size; ++index) {
        if (value.data[index] == '\0') {
            return 0;
        }
    }
    return 1;
}

static int ulk_setup_text_equal(ulk_string_view left, ulk_string_view right)
{
    return left.size == right.size &&
        (left.size == 0 ||
         (left.data != 0 && right.data != 0 &&
          memcmp(left.data, right.data, (size_t)left.size) == 0));
}

static int ulk_setup_operation_valid(ulk_setup_operation_v1 operation)
{
    return operation >= ULK_SETUP_OPERATION_INSTALL &&
        operation <= ULK_SETUP_OPERATION_RECOVERY;
}

static int ulk_setup_plan_valid(const ulk_setup_plan_reference_v1* plan)
{
    if (plan == 0 || plan->struct_size < (ulk_size)sizeof(*plan) ||
        !ulk_setup_operation_valid(plan->operation) || !plan->reviewed ||
        !ulk_setup_text_valid(plan->plan_id, 0) ||
        !ulk_setup_text_valid(plan->plan_digest, 0) ||
        !ulk_setup_text_valid(plan->input_identity_digest, 0) ||
        !ulk_setup_text_valid(plan->product_id, 0) ||
        !ulk_setup_text_valid(plan->setup_provider, 0) ||
        !ulk_setup_text_valid(plan->provider_revision, 0)) {
        return 0;
    }
    if (plan->operation == ULK_SETUP_OPERATION_INSTALL) {
        return ulk_setup_text_valid(plan->install_id, 1) && plan->install_id.size == 0;
    }
    return ulk_setup_text_valid(plan->install_id, 0);
}

static int ulk_setup_reference_valid(const ulk_install_reference_v2* reference)
{
    if (reference == 0 || reference->struct_size < (ulk_size)sizeof(*reference) ||
        reference->ownership < ULK_INSTALL_OWNERSHIP_MANAGED ||
        reference->ownership > ULK_INSTALL_OWNERSHIP_FOREIGN_OWNED ||
        reference->lifecycle_status < ULK_INSTALL_LIFECYCLE_ACTIVE ||
        reference->lifecycle_status > ULK_INSTALL_LIFECYCLE_UNINSTALLED ||
        !ulk_setup_text_valid(reference->install_id, 0) ||
        !ulk_setup_text_valid(reference->product_id, 0) ||
        !ulk_setup_text_valid(reference->exact_product_version, 0) ||
        !ulk_setup_text_valid(reference->entrypoint, 0) ||
        !ulk_setup_text_valid(reference->capabilities_json, 0) ||
        !ulk_setup_text_valid(reference->last_verification_identity, 0) ||
        !ulk_setup_text_valid(reference->state_revision, 0)) {
        return 0;
    }
    if (reference->ownership == ULK_INSTALL_OWNERSHIP_MANAGED) {
        return ulk_setup_text_valid(reference->setup_state_ref, 0);
    }
    return ulk_setup_text_valid(reference->setup_state_ref, 1);
}

static int ulk_setup_state_valid(const ulk_installed_state_reference_v1* state)
{
    return state != 0 && state->struct_size >= (ulk_size)sizeof(*state) &&
        state->lifecycle_status >= ULK_INSTALL_LIFECYCLE_ACTIVE &&
        state->lifecycle_status <= ULK_INSTALL_LIFECYCLE_UNINSTALLED &&
        ulk_setup_text_valid(state->setup_state_ref, 0) &&
        ulk_setup_text_valid(state->install_id, 0) &&
        ulk_setup_text_valid(state->product_id, 0) &&
        ulk_setup_text_valid(state->exact_product_version, 0) &&
        ulk_setup_text_valid(state->entrypoint, 0) &&
        ulk_setup_text_valid(state->capabilities_json, 0) &&
        ulk_setup_text_valid(state->last_verification_identity, 0) &&
        ulk_setup_text_valid(state->state_revision, 0);
}

static void ulk_setup_copy_state(
    const ulk_installed_state_reference_v1* state,
    ulk_install_reference_v2* reference
)
{
    reference->struct_size = sizeof(*reference);
    reference->install_id = state->install_id;
    reference->product_id = state->product_id;
    reference->ownership = ULK_INSTALL_OWNERSHIP_MANAGED;
    reference->setup_state_ref = state->setup_state_ref;
    reference->exact_product_version = state->exact_product_version;
    reference->entrypoint = state->entrypoint;
    reference->capabilities_json = state->capabilities_json;
    reference->lifecycle_status = state->lifecycle_status;
    reference->last_verification_identity = state->last_verification_identity;
    reference->state_revision = state->state_revision;
}

static int ulk_setup_result_identity_matches(
    const ulk_setup_plan_reference_v1* plan,
    const ulk_setup_result_v1* result
)
{
    return result != 0 && result->struct_size >= (ulk_size)sizeof(*result) &&
        result->operation == plan->operation &&
        result->status >= ULK_SETUP_RESULT_COMPLETED &&
        result->status <= ULK_SETUP_RESULT_RECOVERY_REQUIRED &&
        ulk_setup_text_valid(result->result_id, 0) &&
        ulk_setup_text_valid(result->audit_identity, result->status != ULK_SETUP_RESULT_COMPLETED) &&
        ulk_setup_text_equal(result->plan_id, plan->plan_id) &&
        ulk_setup_text_equal(result->plan_digest, plan->plan_digest) &&
        ulk_setup_text_equal(result->product_id, plan->product_id) &&
        ulk_setup_text_equal(result->setup_provider, plan->setup_provider) &&
        ulk_setup_text_equal(result->provider_revision, plan->provider_revision);
}

static void ulk_setup_copy_current(
    const ulk_install_reference_v2* current,
    ulk_install_reference_refresh_v1* refresh
)
{
    refresh->has_install_reference = current != 0;
    if (current != 0) {
        refresh->install_reference = *current;
        refresh->install_reference.struct_size = sizeof(refresh->install_reference);
    }
}

int ulk_setup_apply_request_validate_v1(
    const ulk_setup_apply_request_v1* request,
    const ulk_setup_plan_reference_v1* reviewed_plan
)
{
    if (request == 0 || request->struct_size < (ulk_size)sizeof(*request) ||
        !ulk_setup_plan_valid(reviewed_plan) ||
        !ulk_setup_operation_valid(request->operation) ||
        !ulk_setup_text_valid(request->plan_id, 0) ||
        !ulk_setup_text_valid(request->plan_digest, 0) ||
        !ulk_setup_text_valid(request->input_identity_digest, 0) ||
        !ulk_setup_text_valid(request->product_id, 0) ||
        !ulk_setup_text_valid(request->setup_provider, 0) ||
        !ulk_setup_text_valid(request->provider_revision, 0) ||
        !ulk_setup_text_valid(request->install_id, request->operation == ULK_SETUP_OPERATION_INSTALL)) {
        return ULK_STATUS_INVALID_ARGUMENT;
    }
    if (request->operation != reviewed_plan->operation ||
        !ulk_setup_text_equal(request->plan_id, reviewed_plan->plan_id) ||
        !ulk_setup_text_equal(request->plan_digest, reviewed_plan->plan_digest) ||
        !ulk_setup_text_equal(request->input_identity_digest, reviewed_plan->input_identity_digest) ||
        !ulk_setup_text_equal(request->product_id, reviewed_plan->product_id) ||
        !ulk_setup_text_equal(request->install_id, reviewed_plan->install_id) ||
        !ulk_setup_text_equal(request->setup_provider, reviewed_plan->setup_provider) ||
        !ulk_setup_text_equal(request->provider_revision, reviewed_plan->provider_revision)) {
        return ULK_STATUS_ERROR;
    }
    return ULK_STATUS_OK;
}

int ulk_setup_result_project_v1(
    const ulk_setup_plan_reference_v1* reviewed_plan,
    const ulk_install_reference_v2* current_reference,
    const ulk_setup_result_v1* result,
    ulk_install_reference_refresh_v1* refresh
)
{
    const ulk_installed_state_reference_v1* state;
    if (refresh == 0 || refresh->struct_size < (ulk_size)sizeof(*refresh) ||
        !ulk_setup_plan_valid(reviewed_plan) ||
        !ulk_setup_result_identity_matches(reviewed_plan, result) ||
        (current_reference != 0 && !ulk_setup_reference_valid(current_reference))) {
        return ULK_STATUS_INVALID_ARGUMENT;
    }
    if (reviewed_plan->operation == ULK_SETUP_OPERATION_INSTALL) {
        if (current_reference != 0) {
            return ULK_STATUS_ERROR;
        }
    } else if (current_reference == 0 ||
               current_reference->ownership != ULK_INSTALL_OWNERSHIP_MANAGED ||
               !ulk_setup_text_equal(current_reference->install_id, reviewed_plan->install_id) ||
               !ulk_setup_text_equal(current_reference->product_id, reviewed_plan->product_id)) {
        return ULK_STATUS_ERROR;
    }

    memset(refresh, 0, sizeof(*refresh));
    refresh->struct_size = sizeof(*refresh);
    refresh->transition = ULK_INSTALL_REFRESH_UNCHANGED;
    refresh->dependent_instance_status = current_reference == 0
        ? ULK_DEPENDENT_INSTANCE_INSTALL_UNAVAILABLE
        : ULK_DEPENDENT_INSTANCE_READY;
    refresh->launch_plan_status = ULK_LAUNCH_PLAN_FRESH;
    ulk_setup_copy_current(current_reference, refresh);

    if (result->status == ULK_SETUP_RESULT_RECOVERY_REQUIRED) {
        if (current_reference != 0) {
            refresh->transition = ULK_INSTALL_REFRESH_REFRESHED;
            refresh->install_reference.lifecycle_status = ULK_INSTALL_LIFECYCLE_RECOVERY_REQUIRED;
            refresh->dependent_instance_status = ULK_DEPENDENT_INSTANCE_RECOVERY_REQUIRED;
            refresh->launch_plan_status = ULK_LAUNCH_PLAN_STALE;
        }
        return ULK_STATUS_OK;
    }
    if (result->status != ULK_SETUP_RESULT_COMPLETED) {
        return ULK_STATUS_OK;
    }

    if (reviewed_plan->operation == ULK_SETUP_OPERATION_UNINSTALL) {
        if (result->installed_state != 0) {
            return ULK_STATUS_ERROR;
        }
        refresh->transition = ULK_INSTALL_REFRESH_ARCHIVED;
        refresh->install_reference.lifecycle_status = ULK_INSTALL_LIFECYCLE_UNINSTALLED;
        refresh->dependent_instance_status = ULK_DEPENDENT_INSTANCE_INSTALL_UNAVAILABLE;
        refresh->launch_plan_status = ULK_LAUNCH_PLAN_STALE;
        return ULK_STATUS_OK;
    }

    state = result->installed_state;
    if (!ulk_setup_state_valid(state) ||
        state->lifecycle_status != ULK_INSTALL_LIFECYCLE_ACTIVE ||
        !ulk_setup_text_equal(state->product_id, reviewed_plan->product_id)) {
        return ULK_STATUS_ERROR;
    }
    if (reviewed_plan->operation != ULK_SETUP_OPERATION_INSTALL &&
        (!ulk_setup_text_equal(state->install_id, current_reference->install_id) ||
         !ulk_setup_text_equal(state->exact_product_version, current_reference->exact_product_version))) {
        return ULK_STATUS_ERROR;
    }

    refresh->has_install_reference = 1;
    refresh->transition = reviewed_plan->operation == ULK_SETUP_OPERATION_INSTALL
        ? ULK_INSTALL_REFRESH_CREATED
        : ULK_INSTALL_REFRESH_REFRESHED;
    refresh->dependent_instance_status = reviewed_plan->operation == ULK_SETUP_OPERATION_INSTALL
        ? ULK_DEPENDENT_INSTANCE_READY
        : ULK_DEPENDENT_INSTANCE_INSTALL_CHANGED;
    refresh->launch_plan_status = current_reference != 0 &&
        ulk_setup_text_equal(state->state_revision, current_reference->state_revision) &&
        ulk_setup_text_equal(
            state->last_verification_identity,
            current_reference->last_verification_identity)
        ? ULK_LAUNCH_PLAN_FRESH
        : ULK_LAUNCH_PLAN_STALE;
    ulk_setup_copy_state(state, &refresh->install_reference);
    return ULK_STATUS_OK;
}

int ulk_launch_plan_install_status_v1(
    const ulk_launch_plan_install_binding_v1* binding,
    const ulk_install_reference_v2* current_reference,
    ulk_launch_plan_status_v1* status
)
{
    if (binding == 0 || binding->struct_size < (ulk_size)sizeof(*binding) ||
        status == 0 || !ulk_setup_reference_valid(current_reference) ||
        !ulk_setup_text_valid(binding->install_id, 0) ||
        !ulk_setup_text_valid(binding->state_revision, 0) ||
        !ulk_setup_text_valid(binding->verification_identity, 0)) {
        return ULK_STATUS_INVALID_ARGUMENT;
    }
    *status = current_reference->lifecycle_status == ULK_INSTALL_LIFECYCLE_ACTIVE &&
        ulk_setup_text_equal(binding->install_id, current_reference->install_id) &&
        ulk_setup_text_equal(binding->state_revision, current_reference->state_revision) &&
        ulk_setup_text_equal(
            binding->verification_identity,
            current_reference->last_verification_identity)
        ? ULK_LAUNCH_PLAN_FRESH
        : ULK_LAUNCH_PLAN_STALE;
    return ULK_STATUS_OK;
}

int ulk_setup_authority_status_get_v1(ulk_setup_authority_status_v1* status)
{
    if (status == 0 || status->struct_size < (ulk_size)sizeof(*status)) {
        return ULK_STATUS_INVALID_ARGUMENT;
    }
    status->struct_size = sizeof(*status);
    status->mutation_owner = ulk_setup_view("universal-setup");
    status->launcher_role = ulk_setup_view("request_validate_reference");
    status->allowed_operation_mask = ULK_SETUP_OPERATION_MASK_ALL;
    status->launcher_can_mutate_setup = 0;
    return ULK_STATUS_OK;
}
