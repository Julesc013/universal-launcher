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

int main(void)
{
    ulk_launch_capability_v1 capability;
    ulk_entrypoint_descriptor_v1 entrypoint;
    ulk_product_descriptor_v2 product;
    ulk_contract_set_identity_v1 contract_set;
    ulk_product_composition_v1 composition;

    memset(&capability, 0, sizeof(capability));
    memset(&entrypoint, 0, sizeof(entrypoint));
    memset(&product, 0, sizeof(product));
    memset(&contract_set, 0, sizeof(contract_set));
    memset(&composition, 0, sizeof(composition));

    capability.struct_size = sizeof(capability);
    capability.kind = ULK_LAUNCH_CAPABILITY_SINGLE_PROCESS;
    capability.required = 1;

    entrypoint.struct_size = sizeof(entrypoint);
    entrypoint.entrypoint_id = view("main");
    entrypoint.relative_path = view("bin/fixture");
    entrypoint.artifact_set_id = view("core");
    entrypoint.capabilities = &capability;
    entrypoint.capability_count = 1u;

    product.struct_size = sizeof(product);
    product.product_id = view("org.example.fixture");
    product.exact_version = view("1.0.0");
    product.publisher_id = view("publisher.example");
    product.composition_revision = view("composition.1");

    contract_set.struct_size = sizeof(contract_set);
    contract_set.contract_set_id = view("org.example.fixture.contracts");
    contract_set.version = view("1");
    contract_set.sha256 = view(
        "5555555555555555555555555555555555555555555555555555555555555555");

    composition.struct_size = sizeof(composition);
    composition.product = &product;
    composition.entrypoints = &entrypoint;
    composition.entrypoint_count = 1u;
    composition.contract_set = &contract_set;

    if (ulk_product_composition_validate_v1(&composition) != ULK_STATUS_OK) {
        return 1;
    }
    entrypoint.relative_path = view("../escape");
    if (ulk_product_composition_validate_v1(&composition) == ULK_STATUS_OK) {
        return 2;
    }
    return 0;
}
