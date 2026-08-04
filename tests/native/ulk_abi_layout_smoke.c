// SPDX-FileCopyrightText: 2026 Jules C
// SPDX-License-Identifier: MIT

#include "ulk/ulk_api.h"

#include <stddef.h>

#define CHECK(condition, code) do { if (!(condition)) return (code); } while (0)
#define CHECK_LAYOUT(current, baseline, code) \
    CHECK(sizeof(current) == sizeof(baseline), code)
#define CHECK_OFFSET(current, baseline, member, code) \
    CHECK(offsetof(current, member) == offsetof(baseline, member), code)

typedef struct abi16_string_view {
    const char* data;
    ulk_size size;
} abi16_string_view;

typedef void* (*abi16_alloc_fn)(void* user, ulk_size size);
typedef void (*abi16_free_fn)(void* user, void* pointer);

typedef struct abi16_allocator {
    ulk_size struct_size;
    void* user;
    abi16_alloc_fn alloc;
    abi16_free_fn free;
} abi16_allocator;

typedef struct abi16_error {
    ulk_size struct_size;
    int code;
    abi16_string_view message;
    abi16_string_view detail;
} abi16_error;

typedef struct abi16_command_request {
    ulk_size struct_size;
    abi16_string_view command_name;
    abi16_string_view json_payload;
    ulk_bool dry_run;
} abi16_command_request;

typedef struct abi16_command_response {
    ulk_size struct_size;
    int status;
    abi16_string_view json_payload;
    abi16_error error;
} abi16_command_response;

typedef int (ULK_CALL *abi16_transport_execute_fn)(
    void* user_data,
    const abi16_command_request* request,
    abi16_command_response* response
);

typedef struct abi16_transport_adapter {
    ulk_size struct_size;
    ulk_transport_kind_v1 kind;
    abi16_string_view revision;
    abi16_transport_execute_fn execute;
    void* user_data;
} abi16_transport_adapter;

typedef struct abi16_client {
    ulk_size struct_size;
    abi16_transport_adapter transport;
} abi16_client;

typedef int (ULK_CALL *abi16_command_handler_fn)(
    void* user,
    const abi16_command_request* request,
    abi16_command_response* response
);

typedef struct abi16_command_descriptor {
    ulk_size struct_size;
    abi16_string_view command_name;
    abi16_string_view effects_json;
    void* user;
    abi16_command_handler_fn handler;
} abi16_command_descriptor;

typedef struct abi17_owned_response {
    ulk_size struct_size;
    ulk_command_response_v1 response;
    ulk_allocator_v1 allocator;
    void* storage;
    ulk_size storage_size;
} abi17_owned_response;

typedef struct abi17_owned_options {
    ulk_size struct_size;
    const ulk_allocator_v1* allocator;
    ulk_size maximum_total_bytes;
} abi17_owned_options;

typedef struct abi18_contract_set_identity {
    ulk_size struct_size;
    ulk_string_view contract_set_id;
    ulk_string_view version;
    ulk_string_view sha256;
} abi18_contract_set_identity;

typedef struct abi18_launch_capability {
    ulk_size struct_size;
    int kind;
    ulk_bool required;
} abi18_launch_capability;

typedef struct abi18_entrypoint_descriptor {
    ulk_size struct_size;
    ulk_string_view entrypoint_id;
    ulk_string_view relative_path;
    ulk_string_view artifact_set_id;
    const ulk_launch_capability_v1* capabilities;
    ulk_size capability_count;
} abi18_entrypoint_descriptor;

typedef struct abi18_product_descriptor {
    ulk_size struct_size;
    ulk_string_view product_id;
    ulk_string_view exact_version;
    ulk_string_view publisher_id;
    ulk_string_view composition_revision;
} abi18_product_descriptor;

typedef struct abi18_product_composition {
    ulk_size struct_size;
    const ulk_product_descriptor_v2* product;
    const ulk_entrypoint_descriptor_v1* entrypoints;
    ulk_size entrypoint_count;
    const ulk_contract_set_identity_v1* contract_set;
} abi18_product_composition;

int main(void)
{
    CHECK_LAYOUT(ulk_string_view, abi16_string_view, 1);
    CHECK_OFFSET(ulk_string_view, abi16_string_view, data, 2);
    CHECK_OFFSET(ulk_string_view, abi16_string_view, size, 3);

    CHECK_LAYOUT(ulk_allocator_v1, abi16_allocator, 4);
    CHECK_OFFSET(ulk_allocator_v1, abi16_allocator, struct_size, 5);
    CHECK_OFFSET(ulk_allocator_v1, abi16_allocator, user, 6);
    CHECK_OFFSET(ulk_allocator_v1, abi16_allocator, alloc, 7);
    CHECK_OFFSET(ulk_allocator_v1, abi16_allocator, free, 8);

    CHECK_LAYOUT(ulk_error_v1, abi16_error, 9);
    CHECK_OFFSET(ulk_error_v1, abi16_error, struct_size, 10);
    CHECK_OFFSET(ulk_error_v1, abi16_error, code, 11);
    CHECK_OFFSET(ulk_error_v1, abi16_error, message, 12);
    CHECK_OFFSET(ulk_error_v1, abi16_error, detail, 13);

    CHECK_LAYOUT(ulk_command_request_v1, abi16_command_request, 14);
    CHECK_OFFSET(ulk_command_request_v1, abi16_command_request, struct_size, 15);
    CHECK_OFFSET(ulk_command_request_v1, abi16_command_request, command_name, 16);
    CHECK_OFFSET(ulk_command_request_v1, abi16_command_request, json_payload, 17);
    CHECK_OFFSET(ulk_command_request_v1, abi16_command_request, dry_run, 18);

    CHECK_LAYOUT(ulk_command_response_v1, abi16_command_response, 19);
    CHECK_OFFSET(ulk_command_response_v1, abi16_command_response, struct_size, 20);
    CHECK_OFFSET(ulk_command_response_v1, abi16_command_response, status, 21);
    CHECK_OFFSET(ulk_command_response_v1, abi16_command_response, json_payload, 22);
    CHECK_OFFSET(ulk_command_response_v1, abi16_command_response, error, 23);

    CHECK_LAYOUT(ulk_transport_adapter_v1, abi16_transport_adapter, 24);
    CHECK_OFFSET(ulk_transport_adapter_v1, abi16_transport_adapter, struct_size, 25);
    CHECK_OFFSET(ulk_transport_adapter_v1, abi16_transport_adapter, kind, 26);
    CHECK_OFFSET(ulk_transport_adapter_v1, abi16_transport_adapter, revision, 27);
    CHECK_OFFSET(ulk_transport_adapter_v1, abi16_transport_adapter, execute, 28);
    CHECK_OFFSET(ulk_transport_adapter_v1, abi16_transport_adapter, user_data, 29);

    CHECK_LAYOUT(ulk_client_v1, abi16_client, 30);
    CHECK_OFFSET(ulk_client_v1, abi16_client, struct_size, 31);
    CHECK_OFFSET(ulk_client_v1, abi16_client, transport, 32);

    CHECK_LAYOUT(ulk_command_descriptor_v1, abi16_command_descriptor, 33);
    CHECK_OFFSET(ulk_command_descriptor_v1, abi16_command_descriptor, struct_size, 34);
    CHECK_OFFSET(ulk_command_descriptor_v1, abi16_command_descriptor, command_name, 35);
    CHECK_OFFSET(ulk_command_descriptor_v1, abi16_command_descriptor, effects_json, 36);
    CHECK_OFFSET(ulk_command_descriptor_v1, abi16_command_descriptor, user, 37);
    CHECK_OFFSET(ulk_command_descriptor_v1, abi16_command_descriptor, handler, 38);

    CHECK_LAYOUT(ulk_owned_command_response_v1, abi17_owned_response, 39);
    CHECK_OFFSET(ulk_owned_command_response_v1, abi17_owned_response, storage, 40);
    CHECK_OFFSET(ulk_owned_command_response_v1, abi17_owned_response, storage_size, 41);
    CHECK_LAYOUT(ulk_owned_command_response_options_v1, abi17_owned_options, 42);
    CHECK_OFFSET(ulk_owned_command_response_options_v1, abi17_owned_options, struct_size, 43);
    CHECK_OFFSET(ulk_owned_command_response_options_v1, abi17_owned_options, allocator, 44);
    CHECK_OFFSET(ulk_owned_command_response_options_v1, abi17_owned_options, maximum_total_bytes, 45);

    CHECK_LAYOUT(ulk_contract_set_identity_v1, abi18_contract_set_identity, 46);
    CHECK_OFFSET(ulk_contract_set_identity_v1, abi18_contract_set_identity, contract_set_id, 47);
    CHECK_OFFSET(ulk_contract_set_identity_v1, abi18_contract_set_identity, sha256, 48);
    CHECK_LAYOUT(ulk_launch_capability_v1, abi18_launch_capability, 49);
    CHECK_OFFSET(ulk_launch_capability_v1, abi18_launch_capability, kind, 50);
    CHECK_OFFSET(ulk_launch_capability_v1, abi18_launch_capability, required, 51);
    CHECK_LAYOUT(ulk_entrypoint_descriptor_v1, abi18_entrypoint_descriptor, 52);
    CHECK_OFFSET(ulk_entrypoint_descriptor_v1, abi18_entrypoint_descriptor, relative_path, 53);
    CHECK_OFFSET(ulk_entrypoint_descriptor_v1, abi18_entrypoint_descriptor, capabilities, 54);
    CHECK_OFFSET(ulk_entrypoint_descriptor_v1, abi18_entrypoint_descriptor, capability_count, 55);
    CHECK_LAYOUT(ulk_product_descriptor_v2, abi18_product_descriptor, 56);
    CHECK_OFFSET(ulk_product_descriptor_v2, abi18_product_descriptor, exact_version, 57);
    CHECK_OFFSET(ulk_product_descriptor_v2, abi18_product_descriptor, composition_revision, 58);
    CHECK_LAYOUT(ulk_product_composition_v1, abi18_product_composition, 59);
    CHECK_OFFSET(ulk_product_composition_v1, abi18_product_composition, product, 60);
    CHECK_OFFSET(ulk_product_composition_v1, abi18_product_composition, entrypoint_count, 61);
    CHECK_OFFSET(ulk_product_composition_v1, abi18_product_composition, contract_set, 62);
    return 0;
}
