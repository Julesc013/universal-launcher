// SPDX-FileCopyrightText: 2026 Jules C
// SPDX-License-Identifier: MIT

#include "ulk/ulk_api.h"

#include <string.h>

typedef struct fixture {
    int calls;
    const ulk_command_request_v1* request;
} fixture;

static ulk_string_view view(const char* text)
{
    ulk_string_view result;
    result.data = text;
    result.size = text == 0 ? 0u : (ulk_size)strlen(text);
    return result;
}

static int ULK_CALL execute(
    void* user_data,
    const ulk_command_request_v1* request,
    ulk_command_response_v1* response
)
{
    fixture* state = (fixture*)user_data;
    state->calls += 1;
    state->request = request;
    response->status = ULK_STATUS_OK;
    response->json_payload = view("{\"status\":\"ok\"}");
    response->error.struct_size = sizeof(response->error);
    return ULK_STATUS_OK;
}

int main(void)
{
    fixture state;
    ulk_transport_adapter_v1 adapter;
    ulk_client_v1 client;
    ulk_command_request_v1 request;
    ulk_command_response_v1 response;
    ulk_string_view name;

    memset(&state, 0, sizeof(state));
    memset(&adapter, 0, sizeof(adapter));
    memset(&client, 0, sizeof(client));
    memset(&request, 0, sizeof(request));
    memset(&response, 0, sizeof(response));
    adapter.struct_size = sizeof(adapter);
    adapter.kind = ULK_TRANSPORT_DIRECT;
    adapter.revision = view("fixture.direct.v1");
    adapter.execute = execute;
    adapter.user_data = &state;
    client.struct_size = sizeof(client);

    if (ulk_client_initialize_v1(0, &adapter) != ULK_STATUS_INVALID_ARGUMENT) return 1;
    if (ulk_client_initialize_v1(&client, 0) != ULK_STATUS_INVALID_ARGUMENT) return 2;
    client.struct_size = sizeof(client) - 1u;
    if (ulk_client_initialize_v1(&client, &adapter) != ULK_STATUS_INVALID_ARGUMENT) return 3;
    client.struct_size = sizeof(client);
    adapter.struct_size = sizeof(adapter) - 1u;
    if (ulk_client_initialize_v1(&client, &adapter) != ULK_STATUS_INVALID_ARGUMENT) return 4;
    adapter.struct_size = sizeof(adapter);
    adapter.kind = (ulk_transport_kind_v1)99;
    if (ulk_client_initialize_v1(&client, &adapter) != ULK_STATUS_INVALID_ARGUMENT) return 5;
    adapter.kind = ULK_TRANSPORT_DIRECT;
    adapter.revision = view(0);
    if (ulk_client_initialize_v1(&client, &adapter) != ULK_STATUS_INVALID_ARGUMENT) return 6;
    adapter.revision.data = 0;
    adapter.revision.size = 1u;
    if (ulk_client_initialize_v1(&client, &adapter) != ULK_STATUS_INVALID_ARGUMENT) return 7;
    adapter.revision = view("fixture.direct.v1");
    adapter.execute = 0;
    if (ulk_client_initialize_v1(&client, &adapter) != ULK_STATUS_INVALID_ARGUMENT) return 8;
    adapter.execute = execute;
    if (ulk_client_initialize_v1(&client, &adapter) != ULK_STATUS_OK) return 1;
    if (
        client.transport.kind != ULK_TRANSPORT_DIRECT ||
        client.transport.revision.data != adapter.revision.data ||
        client.transport.execute != execute ||
        client.transport.user_data != &state
    ) {
        return 9;
    }
    memset(&adapter, 0, sizeof(adapter));

    request.struct_size = sizeof(request);
    request.command_name = view("diagnostics.run");
    request.json_payload = view("{}");
    request.dry_run = 1;
    response.struct_size = sizeof(response);
    if (ulk_client_execute_v1(&client, &request, &response) != ULK_STATUS_OK) return 10;
    if (state.calls != 1 || state.request != &request) return 11;
    if (response.status != ULK_STATUS_OK) return 12;
    if (ulk_command_response_validate_v1(&response) != ULK_STATUS_OK) return 30;

    request.command_name = view(0);
    if (ulk_client_execute_v1(&client, &request, &response) != ULK_STATUS_INVALID_ARGUMENT) return 13;
    if (state.calls != 1) return 14;
    request.command_name = view("diagnostics.run");
    request.struct_size = sizeof(request) - 1u;
    if (ulk_client_execute_v1(&client, &request, &response) != ULK_STATUS_INVALID_ARGUMENT) return 15;
    request.struct_size = sizeof(request);
    request.command_name.data = 0;
    request.command_name.size = 1u;
    if (ulk_client_execute_v1(&client, &request, &response) != ULK_STATUS_INVALID_ARGUMENT) return 16;
    request.command_name = view("diagnostics.run");
    request.json_payload.data = 0;
    request.json_payload.size = 1u;
    if (ulk_client_execute_v1(&client, &request, &response) != ULK_STATUS_INVALID_ARGUMENT) return 17;
    request.json_payload = view("{}");
    response.struct_size = sizeof(response) - 1u;
    if (ulk_client_execute_v1(&client, &request, &response) != ULK_STATUS_INVALID_ARGUMENT) return 18;
    response.struct_size = sizeof(response);
    if (ulk_client_execute_v1(0, &request, &response) != ULK_STATUS_INVALID_ARGUMENT) return 19;
    if (ulk_client_execute_v1(&client, 0, &response) != ULK_STATUS_INVALID_ARGUMENT) return 20;
    if (ulk_client_execute_v1(&client, &request, 0) != ULK_STATUS_INVALID_ARGUMENT) return 21;
    client.transport.struct_size = sizeof(client.transport) - 1u;
    if (ulk_client_execute_v1(&client, &request, &response) != ULK_STATUS_INVALID_ARGUMENT) return 22;
    client.transport.struct_size = sizeof(client.transport);
    client.transport.kind = (ulk_transport_kind_v1)99;
    if (ulk_client_execute_v1(&client, &request, &response) != ULK_STATUS_INVALID_ARGUMENT) return 23;
    client.transport.kind = ULK_TRANSPORT_DIRECT;
    client.transport.execute = 0;
    if (ulk_client_execute_v1(&client, &request, &response) != ULK_STATUS_INVALID_ARGUMENT) return 24;
    client.transport.execute = execute;
    if (state.calls != 1) return 25;

    name = ulk_transport_kind_name_v1(ULK_TRANSPORT_DIRECT);
    if (name.size != 6u || memcmp(name.data, "direct", 6u) != 0) return 26;
    name = ulk_transport_kind_name_v1(ULK_TRANSPORT_PROCESS);
    if (name.size != 7u || memcmp(name.data, "process", 7u) != 0) return 27;
    name = ulk_transport_kind_name_v1(ULK_TRANSPORT_DAEMON);
    if (name.size != 6u || memcmp(name.data, "daemon", 6u) != 0) return 28;
    name = ulk_transport_kind_name_v1((ulk_transport_kind_v1)99);
    if (name.size != 7u || memcmp(name.data, "unknown", 7u) != 0) return 29;
    return 0;
}
