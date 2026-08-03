// SPDX-FileCopyrightText: 2026 Jules C
// SPDX-License-Identifier: MIT

#ifndef ULK_CLIENT_API_1_6_H
#define ULK_CLIENT_API_1_6_H

#include <stdint.h>

#define ULK_API_VERSION_MAJOR 1
#define ULK_API_VERSION_MINOR 6

typedef uint64_t ulk_size;
typedef int ulk_bool;

#if defined(_WIN32)
#define ULK_CALL __cdecl
#if defined(ULK_USE_SHARED)
#define ULK_API __declspec(dllimport)
#else
#define ULK_API
#endif
#else
#define ULK_CALL
#define ULK_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ulk_string_view {
    const char* data;
    ulk_size size;
} ulk_string_view;

typedef enum ulk_status {
    ULK_STATUS_OK = 0,
    ULK_STATUS_ERROR = 1,
    ULK_STATUS_INVALID_ARGUMENT = 2,
    ULK_STATUS_UNSUPPORTED_VERSION = 3
} ulk_status;

typedef struct ulk_error_v1 {
    ulk_size struct_size;
    int code;
    ulk_string_view message;
    ulk_string_view detail;
} ulk_error_v1;

typedef struct ulk_command_request_v1 {
    ulk_size struct_size;
    ulk_string_view command_name;
    ulk_string_view json_payload;
    ulk_bool dry_run;
} ulk_command_request_v1;

typedef struct ulk_command_response_v1 {
    ulk_size struct_size;
    int status;
    ulk_string_view json_payload;
    ulk_error_v1 error;
} ulk_command_response_v1;

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

ULK_API uint32_t ULK_CALL ulk_abi_version_v1(void);

#ifdef __cplusplus
}
#endif

#endif
