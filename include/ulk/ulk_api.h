// SPDX-FileCopyrightText: 2026 Jules C
// SPDX-License-Identifier: MIT

#ifndef ULK_API_H
#define ULK_API_H

#include "ulk_allocator.h"
#include "ulk_artifact_set.h"
#include "ulk_client.h"
#include "ulk_command.h"
#include "ulk_operation.h"
#include "ulk_reference_model.h"
#include "ulk_registry.h"
#include "ulk_setup_handoff.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ulk_context ulk_context;

ULK_API int ULK_CALL ulk_context_create_v1(
    const ulk_allocator_v1* allocator,
    ulk_context** out_context
);

ULK_API int ULK_CALL ulk_command_execute_v1(
    ulk_context* context,
    const ulk_command_request_v1* request,
    ulk_command_response_v1* response
);

ULK_API int ULK_CALL ulk_command_register_v1(
    ulk_context* context,
    const ulk_command_descriptor_v1* descriptor
);

ULK_API int ULK_CALL ulk_command_register_v2(
    ulk_context* context,
    const ulk_command_descriptor_v2* descriptor
);

ULK_API int ULK_CALL ulk_command_unregister_v1(
    ulk_context* context,
    ulk_string_view command_name
);

ULK_API uint32_t ULK_CALL ulk_abi_version_v1(void);

ULK_API void ULK_CALL ulk_context_destroy_v1(ulk_context* context);

#ifdef __cplusplus
}
#endif

#endif
