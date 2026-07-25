// SPDX-FileCopyrightText: 2026 Jules C
// SPDX-License-Identifier: MIT

#include "ulk/ulk_api.h"

#include <string.h>

static ulk_string_view view(const char* value)
{
    ulk_string_view result;
    result.data = value;
    result.size = value == 0 ? 0u : (ulk_size)strlen(value);
    return result;
}

static int equals(ulk_string_view value, const char* expected)
{
    size_t expected_size = strlen(expected);
    return
        value.data != 0 &&
        value.size == (ulk_size)expected_size &&
        memcmp(value.data, expected, expected_size) == 0;
}

int main(void)
{
    ulk_product_ref_v1 product;
    ulk_install_reference_v2 install_reference;
    ulk_instance_ref_v2 instance;
    ulk_profile_ref_v2 profile;
    ulk_artifact_set_ref_v1 artifact_set;
    ulk_launch_plan_ref_v2 launch_plan;
    ulk_reference_graph_v1 graph;
    ulk_reference_validation_v1 validation;

    memset(&product, 0, sizeof(product));
    memset(&install_reference, 0, sizeof(install_reference));
    memset(&instance, 0, sizeof(instance));
    memset(&profile, 0, sizeof(profile));
    memset(&artifact_set, 0, sizeof(artifact_set));
    memset(&launch_plan, 0, sizeof(launch_plan));
    memset(&graph, 0, sizeof(graph));
    memset(&validation, 0, sizeof(validation));

    product.struct_size = sizeof(product);
    product.product_id = view("example.product");
    product.binding_id = view("example.binding");

    install_reference.struct_size = sizeof(install_reference);
    install_reference.install_id = view("install.main");
    install_reference.product_id = view("example.product");
    install_reference.ownership = ULK_INSTALL_OWNERSHIP_MANAGED;
    install_reference.setup_state_ref = view("setup-state-1");
    install_reference.exact_product_version = view("1.2.3");
    install_reference.entrypoint = view("bin/example");
    install_reference.capabilities_json = view("[]");
    install_reference.lifecycle_status = ULK_INSTALL_LIFECYCLE_ACTIVE;
    install_reference.last_verification_identity = view("verification-1");
    install_reference.state_revision = view("install-revision-1");

    instance.struct_size = sizeof(instance);
    instance.instance_id = view("instance.main");
    instance.product_id = view("example.product");
    instance.install_id = view("install.main");
    instance.profile_id = view("profile.main");
    instance.artifact_set_id = view("artifacts.main");
    instance.binding_revision = view("binding-revision-1");

    profile.struct_size = sizeof(profile);
    profile.profile_id = view("profile.main");
    profile.product_id = view("example.product");
    profile.revision = view("profile-revision-1");

    artifact_set.struct_size = sizeof(artifact_set);
    artifact_set.product_id = view("example.product");
    artifact_set.artifact_set_id = view("artifacts.main");
    artifact_set.lock_json = view("{\"artifacts\":[]}");

    launch_plan.struct_size = sizeof(launch_plan);
    launch_plan.plan_id = view("plan.main");
    launch_plan.product_id = view("example.product");
    launch_plan.instance_id = view("instance.main");
    launch_plan.install_id = view("install.main");
    launch_plan.profile_id = view("profile.main");
    launch_plan.artifact_set_id = view("artifacts.main");
    launch_plan.install_state_revision = view("install-revision-1");
    launch_plan.instance_binding_revision = view("binding-revision-1");
    launch_plan.composition_digest = view("digest-1");

    graph.struct_size = sizeof(graph);
    graph.product = &product;
    graph.install_reference = &install_reference;
    graph.instance = &instance;
    graph.profile = &profile;
    graph.artifact_set = &artifact_set;
    graph.launch_plan = &launch_plan;

    validation.struct_size = sizeof(validation);
    if (ulk_reference_graph_validate_v1(&graph, &validation) != ULK_STATUS_OK) return 1;
    if (!validation.valid || validation.issue != ULK_REFERENCE_ISSUE_NONE) return 2;
    if (validation.launch_plan_status != ULK_LAUNCH_PLAN_FRESH) return 3;
    if (!equals(validation.status_code, "valid")) return 4;

    launch_plan.install_state_revision = view("install-revision-old");
    if (ulk_reference_graph_validate_v1(&graph, &validation) != ULK_STATUS_OK) return 5;
    if (!validation.valid || validation.launch_plan_status != ULK_LAUNCH_PLAN_STALE) return 6;
    if (!equals(validation.status_code, "stale")) return 7;
    launch_plan.install_state_revision = view("install-revision-1");

    launch_plan.product_id = view("other.product");
    if (ulk_reference_graph_validate_v1(&graph, &validation) != ULK_STATUS_INVALID_ARGUMENT) return 8;
    if (validation.valid || validation.issue != ULK_REFERENCE_ISSUE_PRODUCT_MISMATCH) return 9;
    if (!equals(validation.status_code, "product_mismatch")) return 10;
    launch_plan.product_id = view("example.product");

    instance.profile_id = view("profile.other");
    if (ulk_reference_graph_validate_v1(&graph, &validation) != ULK_STATUS_INVALID_ARGUMENT) return 11;
    if (validation.issue != ULK_REFERENCE_ISSUE_PROFILE_MISMATCH) return 12;
    instance.profile_id = view("profile.main");

    graph.profile = 0;
    if (ulk_reference_graph_validate_v1(&graph, &validation) != ULK_STATUS_INVALID_ARGUMENT) return 13;
    if (validation.issue != ULK_REFERENCE_ISSUE_PROFILE_MISMATCH) return 14;
    graph.profile = &profile;

    product.product_id = view("invalid/product");
    if (ulk_product_ref_validate_v1(&product) != ULK_STATUS_INVALID_ARGUMENT) return 15;
    product.product_id = view("example.product");

    if (ulk_reference_graph_validate_v1(0, &validation) != ULK_STATUS_INVALID_ARGUMENT) return 16;
    if (validation.issue != ULK_REFERENCE_ISSUE_INVALID_GRAPH) return 17;
    return 0;
}
