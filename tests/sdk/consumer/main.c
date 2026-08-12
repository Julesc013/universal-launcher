// SPDX-FileCopyrightText: 2026 Jules C
// SPDX-License-Identifier: MIT

#include "ulk/ulk_api.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
#include <direct.h>
#include <process.h>
#define ULK_SDK_PID _getpid
#else
#include <unistd.h>
#define ULK_SDK_PID getpid
#endif

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

static int prove_session_journal(void)
{
    char root[128];
    char path[256];
    char json[2048];
    ulk_size required = 0u;
    ulk_session_lookup_status_v1 lookup = ULK_SESSION_LOOKUP_NOT_FOUND;
    ulk_session_journal_v1 journal;
    ulk_session_record_v1 record;
    ulk_error_v1 error;
    int result = 0;

    (void)snprintf(root, sizeof(root), "ulk-sdk-session-%d", ULK_SDK_PID());
    memset(&journal, 0, sizeof(journal));
    memset(&record, 0, sizeof(record));
    memset(&error, 0, sizeof(error));
    journal.struct_size = sizeof(journal);
    journal.root = view(root);
    journal.maximum_records = 4u;
    record.struct_size = sizeof(record);
    record.session_id = view("sdk-session-1");
    record.identity.struct_size = sizeof(record.identity);
    record.identity.operation_id = view("sdk-operation-1");
    record.identity.attempt_id = view("sdk-attempt-1");
    record.runnable_reference = view("fixture://sdk/runnable");
    record.process_identity = view("sdk-fixture-process");
    record.state = ULK_SESSION_RUNNING;
    record.started_at = view("2026-08-12T00:00:00Z");
    error.struct_size = sizeof(error);
    if (ulk_session_journal_write_v1(&journal, &record, &error) == ULK_STATUS_OK &&
        ulk_session_journal_inspect_v1(&journal, record.session_id, &lookup,
            json, sizeof(json), &required, &error) == ULK_STATUS_OK &&
        lookup == ULK_SESSION_LOOKUP_FOUND &&
        strstr(json, "\"schema\":\"ulk.session_record.v1\"") != NULL) {
        result = 1;
    }
    (void)snprintf(path, sizeof(path), "%s/sessions/sdk-session-1.session", root);
    (void)remove(path);
    (void)snprintf(path, sizeof(path), "%s/.ulk-session.lock", root);
    (void)remove(path);
    (void)snprintf(path, sizeof(path), "%s/sessions", root);
#if defined(_WIN32)
    (void)_rmdir(path);
    (void)_rmdir(root);
#else
    (void)rmdir(path);
    (void)rmdir(root);
#endif
    return result;
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

    if (!prove_session_journal()) {
        return 15;
    }

    printf("{\"abi\":\"%u.%u\",\"composition\":\"valid\",\"owned_response\":\"valid\",\"session_journal\":\"valid\"}\n",
           (unsigned int)(ulk_abi_version_v1() >> 16),
           (unsigned int)(ulk_abi_version_v1() & 0xffffu));
    return 0;
}
