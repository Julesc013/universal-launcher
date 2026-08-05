// SPDX-FileCopyrightText: 2026 Jules C
// SPDX-License-Identifier: MIT

#include "ulk/ulk_api.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static ulk_string_view view(const char* value)
{
    ulk_string_view result;
    result.data = value;
    result.size = value == NULL ? 0u : (ulk_size)strlen(value);
    return result;
}
static int fixture_contains(const char* path, const char* expected)
{
    FILE* stream;
    long length;
    char* content;
    int found;
    stream = fopen(path, "rb");
    if (stream == NULL || fseek(stream, 0, SEEK_END) != 0) {
        if (stream != NULL) {
            fclose(stream);
        }
        return 0;
    }
    length = ftell(stream);
    if (length < 0 || fseek(stream, 0, SEEK_SET) != 0) {
        fclose(stream);
        return 0;
    }
    content = (char*)malloc((size_t)length + 1u);
    if (content == NULL) {
        fclose(stream);
        return 0;
    }
    if (fread(content, 1u, (size_t)length, stream) != (size_t)length) {
        free(content);
        fclose(stream);
        return 0;
    }
    content[length] = '\0';
    found = strstr(content, expected) != NULL;
    free(content);
    fclose(stream);
    return found;
}

int main(int argc, char** argv)
{
    const char payload[] = "{\"result\":\"fixture-qualified\"}";
    ulk_launch_capability_v1 capability;
    ulk_entrypoint_descriptor_v1 entrypoint;
    ulk_product_descriptor_v2 product;
    ulk_contract_set_identity_v1 contract_set;
    ulk_product_composition_v1 composition;
    ulk_command_response_v1 response;
    ulk_owned_command_response_v1 owned;

    if (argc != 2 || !fixture_contains(argv[1], "org.example.fixture") ||
        !fixture_contains(argv[1], "single_process")) {
        return 10;
    }

    memset(&capability, 0, sizeof(capability));
    memset(&entrypoint, 0, sizeof(entrypoint));
    memset(&product, 0, sizeof(product));
    memset(&contract_set, 0, sizeof(contract_set));
    memset(&composition, 0, sizeof(composition));
    memset(&response, 0, sizeof(response));
    memset(&owned, 0, sizeof(owned));

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
        return 11;
    }

    response.struct_size = sizeof(response);
    response.status = ULK_STATUS_OK;
    response.json_payload = view(payload);
    response.error.struct_size = sizeof(response.error);
    owned.struct_size = sizeof(owned);
    if (ulk_command_response_copy_owned_v1(&response, NULL, &owned) != ULK_STATUS_OK) {
        return 12;
    }
    if (owned.response.json_payload.size != response.json_payload.size ||
        memcmp(owned.response.json_payload.data, payload, sizeof(payload) - 1u) != 0) {
        ulk_owned_command_response_release_v1(&owned);
        return 13;
    }
    ulk_owned_command_response_release_v1(&owned);
    if (owned.storage != NULL) {
        return 14;
    }

    printf("{\"abi\":\"%u.%u\",\"composition\":\"valid\",\"owned_response\":\"valid\"}\n",
           (unsigned int)(ulk_abi_version_v1() >> 16),
           (unsigned int)(ulk_abi_version_v1() & 0xffffu));
    return 0;
}
