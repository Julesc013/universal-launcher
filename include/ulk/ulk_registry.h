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

#endif
