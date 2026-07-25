// SPDX-FileCopyrightText: 2026 Jules C
// SPDX-License-Identifier: MIT

#include "ulk/ulk_client.h"

#include <string.h>

static int ulk_view_is_valid(ulk_string_view value)
{
    return value.size == 0u || value.data != 0;
}

static int ulk_transport_kind_is_valid(ulk_transport_kind_v1 kind)
{
    return
        kind == ULK_TRANSPORT_DIRECT ||
        kind == ULK_TRANSPORT_PROCESS ||
        kind == ULK_TRANSPORT_DAEMON;
}

int ULK_CALL ulk_client_initialize_v1(
    ulk_client_v1* client,
    const ulk_transport_adapter_v1* transport
)
{
    if (
        client == 0 ||
        client->struct_size < (ulk_size)sizeof(*client) ||
        transport == 0 ||
        transport->struct_size < (ulk_size)sizeof(*transport) ||
        !ulk_transport_kind_is_valid(transport->kind) ||
        !ulk_view_is_valid(transport->revision) ||
        transport->revision.size == 0u ||
        transport->execute == 0
    ) {
        return ULK_STATUS_INVALID_ARGUMENT;
    }
    client->transport = *transport;
    return ULK_STATUS_OK;
}

int ULK_CALL ulk_client_execute_v1(
    ulk_client_v1* client,
    const ulk_command_request_v1* request,
    ulk_command_response_v1* response
)
{
    if (
        client == 0 ||
        client->struct_size < (ulk_size)sizeof(*client) ||
        client->transport.struct_size < (ulk_size)sizeof(client->transport) ||
        !ulk_transport_kind_is_valid(client->transport.kind) ||
        client->transport.execute == 0 ||
        request == 0 ||
        request->struct_size < (ulk_size)sizeof(*request) ||
        !ulk_view_is_valid(request->command_name) ||
        request->command_name.size == 0u ||
        !ulk_view_is_valid(request->json_payload) ||
        response == 0 ||
        response->struct_size < (ulk_size)sizeof(*response)
    ) {
        return ULK_STATUS_INVALID_ARGUMENT;
    }
    return client->transport.execute(
        client->transport.user_data,
        request,
        response);
}

ulk_string_view ULK_CALL ulk_transport_kind_name_v1(ulk_transport_kind_v1 kind)
{
    const char* name = "unknown";
    ulk_string_view result;
    if (kind == ULK_TRANSPORT_DIRECT) {
        name = "direct";
    } else if (kind == ULK_TRANSPORT_PROCESS) {
        name = "process";
    } else if (kind == ULK_TRANSPORT_DAEMON) {
        name = "daemon";
    }
    result.data = name;
    result.size = (ulk_size)strlen(name);
    return result;
}
