// SPDX-FileCopyrightText: 2026 Jules C
// SPDX-License-Identifier: MIT

#include "ulk/ulk_reference_model.h"

#include <stddef.h>
#include <string.h>

static int ulk_reference_view_valid(ulk_string_view value, int allow_empty)
{
    ulk_size index;
    if (value.size == 0u) {
        return allow_empty;
    }
    if (value.data == 0) {
        return 0;
    }
    for (index = 0u; index < value.size; ++index) {
        if (value.data[index] == '\0') {
            return 0;
        }
    }
    return 1;
}

static int ulk_reference_identifier_valid(ulk_string_view value, int allow_empty)
{
    ulk_size index;
    if (value.size == 0u) {
        return allow_empty;
    }
    if (value.data == 0 || value.size > 128u) {
        return 0;
    }
    for (index = 0u; index < value.size; ++index) {
        unsigned char current = (unsigned char)value.data[index];
        int valid =
            (current >= (unsigned char)'a' && current <= (unsigned char)'z') ||
            (current >= (unsigned char)'A' && current <= (unsigned char)'Z') ||
            (current >= (unsigned char)'0' && current <= (unsigned char)'9') ||
            current == (unsigned char)'.' ||
            current == (unsigned char)'_' ||
            current == (unsigned char)'-';
        if (!valid) {
            return 0;
        }
    }
    return 1;
}

static int ulk_reference_view_equal(ulk_string_view left, ulk_string_view right)
{
    return
        left.size == right.size &&
        (left.size == 0u || memcmp(left.data, right.data, (size_t)left.size) == 0);
}

static ulk_string_view ulk_reference_status_code(ulk_reference_issue_v1 issue)
{
    const char* code = "valid";
    ulk_string_view result;
    switch (issue) {
        case ULK_REFERENCE_ISSUE_INVALID_GRAPH: code = "invalid_graph"; break;
        case ULK_REFERENCE_ISSUE_INVALID_PRODUCT: code = "invalid_product"; break;
        case ULK_REFERENCE_ISSUE_INVALID_INSTALL_REFERENCE: code = "invalid_install_reference"; break;
        case ULK_REFERENCE_ISSUE_INVALID_INSTANCE: code = "invalid_instance"; break;
        case ULK_REFERENCE_ISSUE_INVALID_PROFILE: code = "invalid_profile"; break;
        case ULK_REFERENCE_ISSUE_INVALID_ARTIFACT_SET: code = "invalid_artifact_set"; break;
        case ULK_REFERENCE_ISSUE_INVALID_LAUNCH_PLAN: code = "invalid_launch_plan"; break;
        case ULK_REFERENCE_ISSUE_PRODUCT_MISMATCH: code = "product_mismatch"; break;
        case ULK_REFERENCE_ISSUE_INSTALL_MISMATCH: code = "install_mismatch"; break;
        case ULK_REFERENCE_ISSUE_INSTANCE_MISMATCH: code = "instance_mismatch"; break;
        case ULK_REFERENCE_ISSUE_PROFILE_MISMATCH: code = "profile_mismatch"; break;
        case ULK_REFERENCE_ISSUE_ARTIFACT_SET_MISMATCH: code = "artifact_set_mismatch"; break;
        case ULK_REFERENCE_ISSUE_NONE: break;
        default: code = "invalid_graph"; break;
    }
    result.data = code;
    result.size = (ulk_size)strlen(code);
    return result;
}

static int ulk_reference_fail(
    ulk_reference_validation_v1* validation,
    ulk_reference_issue_v1 issue
)
{
    if (validation != 0 &&
        validation->struct_size >= (ulk_size)sizeof(*validation)) {
        validation->valid = 0;
        validation->issue = issue;
        validation->launch_plan_status = ULK_LAUNCH_PLAN_STALE;
        validation->status_code = ulk_reference_status_code(issue);
    }
    return ULK_STATUS_INVALID_ARGUMENT;
}

int ULK_CALL ulk_product_ref_validate_v1(const ulk_product_ref_v1* product)
{
    if (
        product == 0 ||
        product->struct_size < (ulk_size)sizeof(*product) ||
        !ulk_reference_identifier_valid(product->product_id, 0) ||
        !ulk_reference_identifier_valid(product->binding_id, 0)
    ) {
        return ULK_STATUS_INVALID_ARGUMENT;
    }
    return ULK_STATUS_OK;
}

int ULK_CALL ulk_install_reference_validate_v2(
    const ulk_install_reference_v2* install_reference
)
{
    int ownership_valid;
    int lifecycle_valid;
    if (install_reference == 0 ||
        install_reference->struct_size < (ulk_size)sizeof(*install_reference)) {
        return ULK_STATUS_INVALID_ARGUMENT;
    }
    ownership_valid =
        install_reference->ownership == ULK_INSTALL_OWNERSHIP_MANAGED ||
        install_reference->ownership == ULK_INSTALL_OWNERSHIP_IMPORTED ||
        install_reference->ownership == ULK_INSTALL_OWNERSHIP_FOREIGN_OWNED;
    lifecycle_valid =
        install_reference->lifecycle_status == ULK_INSTALL_LIFECYCLE_ACTIVE ||
        install_reference->lifecycle_status == ULK_INSTALL_LIFECYCLE_VERIFICATION_FAILED ||
        install_reference->lifecycle_status == ULK_INSTALL_LIFECYCLE_RECOVERY_REQUIRED ||
        install_reference->lifecycle_status == ULK_INSTALL_LIFECYCLE_RETIRED ||
        install_reference->lifecycle_status == ULK_INSTALL_LIFECYCLE_UNINSTALLED;
    if (
        !ownership_valid ||
        !lifecycle_valid ||
        !ulk_reference_identifier_valid(install_reference->install_id, 0) ||
        !ulk_reference_identifier_valid(install_reference->product_id, 0) ||
        !ulk_reference_view_valid(install_reference->setup_state_ref, 1) ||
        !ulk_reference_view_valid(install_reference->exact_product_version, 0) ||
        !ulk_reference_view_valid(install_reference->entrypoint, 0) ||
        !ulk_reference_view_valid(install_reference->capabilities_json, 0) ||
        !ulk_reference_view_valid(install_reference->last_verification_identity, 0) ||
        !ulk_reference_view_valid(install_reference->state_revision, 0) ||
        (install_reference->ownership == ULK_INSTALL_OWNERSHIP_MANAGED &&
         install_reference->setup_state_ref.size == 0u)
    ) {
        return ULK_STATUS_INVALID_ARGUMENT;
    }
    return ULK_STATUS_OK;
}

int ULK_CALL ulk_instance_ref_validate_v2(const ulk_instance_ref_v2* instance)
{
    if (
        instance == 0 ||
        instance->struct_size < (ulk_size)sizeof(*instance) ||
        !ulk_reference_identifier_valid(instance->instance_id, 0) ||
        !ulk_reference_identifier_valid(instance->product_id, 0) ||
        !ulk_reference_identifier_valid(instance->install_id, 0) ||
        !ulk_reference_identifier_valid(instance->profile_id, 1) ||
        !ulk_reference_identifier_valid(instance->artifact_set_id, 1) ||
        !ulk_reference_view_valid(instance->binding_revision, 0)
    ) {
        return ULK_STATUS_INVALID_ARGUMENT;
    }
    return ULK_STATUS_OK;
}

int ULK_CALL ulk_profile_ref_validate_v2(const ulk_profile_ref_v2* profile)
{
    if (
        profile == 0 ||
        profile->struct_size < (ulk_size)sizeof(*profile) ||
        !ulk_reference_identifier_valid(profile->profile_id, 0) ||
        !ulk_reference_identifier_valid(profile->product_id, 0) ||
        !ulk_reference_view_valid(profile->revision, 0)
    ) {
        return ULK_STATUS_INVALID_ARGUMENT;
    }
    return ULK_STATUS_OK;
}

int ULK_CALL ulk_artifact_set_ref_validate_v1(
    const ulk_artifact_set_ref_v1* artifact_set
)
{
    if (
        artifact_set == 0 ||
        artifact_set->struct_size < (ulk_size)sizeof(*artifact_set) ||
        !ulk_reference_identifier_valid(artifact_set->product_id, 0) ||
        !ulk_reference_identifier_valid(artifact_set->artifact_set_id, 0) ||
        !ulk_reference_view_valid(artifact_set->lock_json, 0)
    ) {
        return ULK_STATUS_INVALID_ARGUMENT;
    }
    return ULK_STATUS_OK;
}

int ULK_CALL ulk_launch_plan_ref_validate_v2(
    const ulk_launch_plan_ref_v2* launch_plan
)
{
    if (
        launch_plan == 0 ||
        launch_plan->struct_size < (ulk_size)sizeof(*launch_plan) ||
        !ulk_reference_identifier_valid(launch_plan->plan_id, 0) ||
        !ulk_reference_identifier_valid(launch_plan->product_id, 0) ||
        !ulk_reference_identifier_valid(launch_plan->instance_id, 0) ||
        !ulk_reference_identifier_valid(launch_plan->install_id, 0) ||
        !ulk_reference_identifier_valid(launch_plan->profile_id, 1) ||
        !ulk_reference_identifier_valid(launch_plan->artifact_set_id, 1) ||
        !ulk_reference_view_valid(launch_plan->install_state_revision, 0) ||
        !ulk_reference_view_valid(launch_plan->instance_binding_revision, 0) ||
        !ulk_reference_view_valid(launch_plan->composition_digest, 0)
    ) {
        return ULK_STATUS_INVALID_ARGUMENT;
    }
    return ULK_STATUS_OK;
}

int ULK_CALL ulk_reference_graph_validate_v1(
    const ulk_reference_graph_v1* graph,
    ulk_reference_validation_v1* validation
)
{
    int stale;
    if (
        validation == 0 ||
        validation->struct_size < (ulk_size)sizeof(*validation)
    ) {
        return ULK_STATUS_INVALID_ARGUMENT;
    }
    validation->valid = 0;
    validation->issue = ULK_REFERENCE_ISSUE_INVALID_GRAPH;
    validation->launch_plan_status = ULK_LAUNCH_PLAN_STALE;
    validation->status_code = ulk_reference_status_code(ULK_REFERENCE_ISSUE_INVALID_GRAPH);
    if (
        graph == 0 ||
        graph->struct_size < (ulk_size)sizeof(*graph) ||
        graph->product == 0 ||
        graph->install_reference == 0 ||
        graph->instance == 0 ||
        graph->launch_plan == 0
    ) {
        return ULK_STATUS_INVALID_ARGUMENT;
    }
    if (ulk_product_ref_validate_v1(graph->product) != ULK_STATUS_OK) {
        return ulk_reference_fail(validation, ULK_REFERENCE_ISSUE_INVALID_PRODUCT);
    }
    if (ulk_install_reference_validate_v2(graph->install_reference) != ULK_STATUS_OK) {
        return ulk_reference_fail(validation, ULK_REFERENCE_ISSUE_INVALID_INSTALL_REFERENCE);
    }
    if (ulk_instance_ref_validate_v2(graph->instance) != ULK_STATUS_OK) {
        return ulk_reference_fail(validation, ULK_REFERENCE_ISSUE_INVALID_INSTANCE);
    }
    if (graph->profile != 0 &&
        ulk_profile_ref_validate_v2(graph->profile) != ULK_STATUS_OK) {
        return ulk_reference_fail(validation, ULK_REFERENCE_ISSUE_INVALID_PROFILE);
    }
    if (graph->artifact_set != 0 &&
        ulk_artifact_set_ref_validate_v1(graph->artifact_set) != ULK_STATUS_OK) {
        return ulk_reference_fail(validation, ULK_REFERENCE_ISSUE_INVALID_ARTIFACT_SET);
    }
    if (ulk_launch_plan_ref_validate_v2(graph->launch_plan) != ULK_STATUS_OK) {
        return ulk_reference_fail(validation, ULK_REFERENCE_ISSUE_INVALID_LAUNCH_PLAN);
    }
    if (
        !ulk_reference_view_equal(graph->product->product_id, graph->install_reference->product_id) ||
        !ulk_reference_view_equal(graph->product->product_id, graph->instance->product_id) ||
        !ulk_reference_view_equal(graph->product->product_id, graph->launch_plan->product_id) ||
        (graph->profile != 0 &&
         !ulk_reference_view_equal(graph->product->product_id, graph->profile->product_id)) ||
        (graph->artifact_set != 0 &&
         !ulk_reference_view_equal(graph->product->product_id, graph->artifact_set->product_id))
    ) {
        return ulk_reference_fail(validation, ULK_REFERENCE_ISSUE_PRODUCT_MISMATCH);
    }
    if (
        !ulk_reference_view_equal(graph->install_reference->install_id, graph->instance->install_id) ||
        !ulk_reference_view_equal(graph->install_reference->install_id, graph->launch_plan->install_id)
    ) {
        return ulk_reference_fail(validation, ULK_REFERENCE_ISSUE_INSTALL_MISMATCH);
    }
    if (!ulk_reference_view_equal(graph->instance->instance_id, graph->launch_plan->instance_id)) {
        return ulk_reference_fail(validation, ULK_REFERENCE_ISSUE_INSTANCE_MISMATCH);
    }
    if (
        (graph->profile == 0 && graph->instance->profile_id.size != 0u) ||
        (graph->profile == 0 && graph->launch_plan->profile_id.size != 0u) ||
        (graph->profile != 0 &&
         (!ulk_reference_view_equal(graph->profile->profile_id, graph->instance->profile_id) ||
          !ulk_reference_view_equal(graph->profile->profile_id, graph->launch_plan->profile_id)))
    ) {
        return ulk_reference_fail(validation, ULK_REFERENCE_ISSUE_PROFILE_MISMATCH);
    }
    if (
        (graph->artifact_set == 0 && graph->instance->artifact_set_id.size != 0u) ||
        (graph->artifact_set == 0 && graph->launch_plan->artifact_set_id.size != 0u) ||
        (graph->artifact_set != 0 &&
         (!ulk_reference_view_equal(graph->artifact_set->artifact_set_id, graph->instance->artifact_set_id) ||
          !ulk_reference_view_equal(graph->artifact_set->artifact_set_id, graph->launch_plan->artifact_set_id)))
    ) {
        return ulk_reference_fail(validation, ULK_REFERENCE_ISSUE_ARTIFACT_SET_MISMATCH);
    }
    stale =
        !ulk_reference_view_equal(
            graph->install_reference->state_revision,
            graph->launch_plan->install_state_revision) ||
        !ulk_reference_view_equal(
            graph->instance->binding_revision,
            graph->launch_plan->instance_binding_revision);
    validation->valid = 1;
    validation->issue = ULK_REFERENCE_ISSUE_NONE;
    validation->launch_plan_status = stale
        ? ULK_LAUNCH_PLAN_STALE
        : ULK_LAUNCH_PLAN_FRESH;
    validation->status_code = stale
        ? (ulk_string_view){"stale", 5u}
        : ulk_reference_status_code(ULK_REFERENCE_ISSUE_NONE);
    return ULK_STATUS_OK;
}
