// SPDX-FileCopyrightText: 2026 Jules C
// SPDX-License-Identifier: MIT

#include "ulk_client_api_1_6.h"

#include <string.h>

#define CHECK(condition, code) do { if (!(condition)) return (code); } while (0)

typedef struct compat_fixture {
    int calls;
} compat_fixture;

static ulk_string_view view(const char* text)
{
    ulk_string_view value;
    value.data = text;
    value.size = text == 0 ? 0u : (ulk_size)strlen(text);
    return value;
}

static int ULK_CALL execute(
    void* user_data,
    const ulk_command_request_v1* request,
    ulk_command_response_v1* response
)
{
    compat_fixture* fixture = (compat_fixture*)user_data;
    fixture->calls += 1;
    CHECK(request->command_name.size == 15u, 20);
    response->status = ULK_STATUS_OK;
    response->json_payload = view("{\"compat\":true}");
    response->error.struct_size = sizeof(response->error);
    return ULK_STATUS_OK;
}

int main(void)
{
    const uint32_t runtime_version = ulk_abi_version_v1();
    compat_fixture fixture;
    ulk_transport_adapter_v1 adapter;
    ulk_client_v1 client;
    ulk_command_request_v1 request;
    ulk_command_response_v1 response;
    ulk_string_view kind;

    memset(&fixture, 0, sizeof(fixture));
    memset(&adapter, 0, sizeof(adapter));
    memset(&client, 0, sizeof(client));
    memset(&request, 0, sizeof(request));
    memset(&response, 0, sizeof(response));

    CHECK(ULK_API_VERSION_MAJOR == 1 && ULK_API_VERSION_MINOR == 6, 1);
    CHECK((runtime_version >> 16) == 1u, 2);
    CHECK((runtime_version & 0xffffu) >= 6u, 3);

    adapter.struct_size = sizeof(adapter);
    adapter.kind = ULK_TRANSPORT_DIRECT;
    adapter.revision = view("compat.abi.1.6");
    adapter.execute = execute;
    adapter.user_data = &fixture;
    client.struct_size = sizeof(client);
    CHECK(ulk_client_initialize_v1(&client, &adapter) == ULK_STATUS_OK, 4);

    request.struct_size = sizeof(request);
    request.command_name = view("diagnostics.run");
    request.json_payload = view("{}");
    request.dry_run = 1;
    response.struct_size = sizeof(response);
    CHECK(
        ulk_client_execute_v1(&client, &request, &response) == ULK_STATUS_OK,
        5);
    CHECK(fixture.calls == 1, 6);
    CHECK(response.status == ULK_STATUS_OK, 7);
    CHECK(
        response.json_payload.size == 15u &&
        memcmp(response.json_payload.data, "{\"compat\":true}", 15u) == 0,
        8);

    kind = ulk_transport_kind_name_v1(ULK_TRANSPORT_DIRECT);
    CHECK(kind.size == 6u && memcmp(kind.data, "direct", 6u) == 0, 9);
    return 0;
}
