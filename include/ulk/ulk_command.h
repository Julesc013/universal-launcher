// SPDX-FileCopyrightText: 2026 Jules C
// SPDX-License-Identifier: MIT

#ifndef ULK_COMMAND_H
#define ULK_COMMAND_H

#include "ulk_error.h"
#include "ulk_types.h"

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

/*
 * Response string views are borrowed; their lifetime is defined by the
 * producing API or callback. Built-in context responses remain valid until
 * the next command call on that context or until the context is destroyed.
 * Registered-handler and transport views retain their producer's declared
 * lifetime. Callers set response.struct_size before every call. No disposal is
 * required. The additive owned-response ABI can validate and immediately copy
 * valid views when a caller needs an independent lifetime.
 */

#endif
