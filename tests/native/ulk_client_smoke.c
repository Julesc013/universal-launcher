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
    if (ulk_client_initialize_v1(&client, &adapter) != ULK_STATUS_OK) return 1;

    request.struct_size = sizeof(request);
    request.command_name = view("diagnostics.run");
    request.json_payload = view("{}");
    request.dry_run = 1;
    response.struct_size = sizeof(response);
    if (ulk_client_execute_v1(&client, &request, &response) != ULK_STATUS_OK) return 2;
    if (state.calls != 1 || state.request != &request) return 3;
    if (response.status != ULK_STATUS_OK) return 4;

    request.command_name = view(0);
    if (ulk_client_execute_v1(&client, &request, &response) != ULK_STATUS_INVALID_ARGUMENT) return 5;
    if (state.calls != 1) return 6;
    request.command_name = view("diagnostics.run");
    adapter.kind = (ulk_transport_kind_v1)99;
    if (ulk_client_initialize_v1(&client, &adapter) != ULK_STATUS_INVALID_ARGUMENT) return 7;

    name = ulk_transport_kind_name_v1(ULK_TRANSPORT_DIRECT);
    if (name.size != 6u || memcmp(name.data, "direct", 6u) != 0) return 8;
    name = ulk_transport_kind_name_v1(ULK_TRANSPORT_PROCESS);
    if (name.size != 7u || memcmp(name.data, "process", 7u) != 0) return 9;
    name = ulk_transport_kind_name_v1(ULK_TRANSPORT_DAEMON);
    if (name.size != 6u || memcmp(name.data, "daemon", 6u) != 0) return 10;
    return 0;
}
