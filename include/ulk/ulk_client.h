// SPDX-FileCopyrightText: 2026 Jules C
// SPDX-License-Identifier: MIT

#ifndef ULK_CLIENT_H
#define ULK_CLIENT_H

#include "ulk_command.h"
#include "ulk_error.h"
#include "ulk_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum ulk_transport_kind_v1 {
    ULK_TRANSPORT_DIRECT = 1,
    ULK_TRANSPORT_PROCESS = 2,
    ULK_TRANSPORT_DAEMON = 3
} ulk_transport_kind_v1;

typedef int (ULK_CALL *ulk_transport_execute_fn_v1)(
    void* user_data,
    const ulk_command_request_v1* request,
    ulk_command_response_v1* response
);

typedef struct ulk_transport_adapter_v1 {
    ulk_size struct_size;
    ulk_transport_kind_v1 kind;
    ulk_string_view revision;
    ulk_transport_execute_fn_v1 execute;
    void* user_data;
} ulk_transport_adapter_v1;

typedef struct ulk_client_v1 {
    ulk_size struct_size;
    ulk_transport_adapter_v1 transport;
} ulk_client_v1;

ULK_API int ULK_CALL ulk_client_initialize_v1(
    ulk_client_v1* client,
    const ulk_transport_adapter_v1* transport
);

ULK_API int ULK_CALL ulk_client_execute_v1(
    ulk_client_v1* client,
    const ulk_command_request_v1* request,
    ulk_command_response_v1* response
);

ULK_API ulk_string_view ULK_CALL ulk_transport_kind_name_v1(
    ulk_transport_kind_v1 kind
);

#ifdef __cplusplus
}
#endif

#endif
