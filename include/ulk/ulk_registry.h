#ifndef ULK_REGISTRY_H
#define ULK_REGISTRY_H

#include "ulk_command.h"

typedef int (ULK_CALL *ulk_command_handler_v1)(
    void* user,
    const ulk_command_request_v1* request,
    ulk_command_response_v1* response);

typedef struct ulk_command_descriptor_v1 {
    ulk_size struct_size;
    ulk_string_view command_name;
    ulk_string_view effects_json;
    void* user;
    ulk_command_handler_v1 handler;
} ulk_command_descriptor_v1;

typedef struct ulk_command_descriptor_v2 {
    ulk_size struct_size;
    ulk_string_view command_name;
    ulk_string_view effects_json;
    ulk_string_view request_schema;
    ulk_string_view response_schema;
    ulk_string_view result_schema;
    ulk_string_view refusal_schema;
    ulk_string_view dry_run_behavior;
    ulk_string_view availability;
    ulk_string_view owner;
    ulk_string_view binding;
    void* user;
    ulk_command_handler_v1 handler;
} ulk_command_descriptor_v2;

#endif
