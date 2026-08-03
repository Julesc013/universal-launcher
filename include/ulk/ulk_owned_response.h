// SPDX-FileCopyrightText: 2026 Jules C
// SPDX-License-Identifier: MIT

#ifndef ULK_OWNED_RESPONSE_H
#define ULK_OWNED_RESPONSE_H

#include "ulk_allocator.h"
#include "ulk_command.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ULK_OWNED_COMMAND_RESPONSE_BYTE_BUDGET_V1 \
    ((ulk_size)(1024u * 1024u))

#define ULK_OWNED_COMMAND_RESPONSE_DEFAULT_MAXIMUM_TOTAL_BYTES_V1 \
    ULK_OWNED_COMMAND_RESPONSE_BYTE_BUDGET_V1

typedef struct ulk_owned_command_response_v1 {
    ulk_size struct_size;
    ulk_command_response_v1 response;
    ulk_allocator_v1 allocator;
    void* storage;
    ulk_size storage_size;
} ulk_owned_command_response_v1;

typedef struct ulk_owned_command_response_options_v1 {
    ulk_size struct_size;
    const ulk_allocator_v1* allocator;
    ulk_size maximum_total_bytes;
} ulk_owned_command_response_options_v1;

ULK_API int ULK_CALL ulk_command_response_validate_v1(
    const ulk_command_response_v1* response
);

ULK_API int ULK_CALL ulk_command_response_copy_owned_v1(
    const ulk_command_response_v1* source,
    const ulk_allocator_v1* allocator,
    ulk_owned_command_response_v1* destination
);

ULK_API int ULK_CALL ulk_command_response_copy_owned_with_options_v1(
    const ulk_command_response_v1* source,
    const ulk_owned_command_response_options_v1* options,
    ulk_owned_command_response_v1* destination
);

ULK_API void ULK_CALL ulk_owned_command_response_release_v1(
    ulk_owned_command_response_v1* response
);

/*
 * Copying preserves the exact byte lengths of the payload, error message, and
 * error detail in one allocator-owned block. The source may be released or
 * invalidated as soon as copying succeeds.
 *
 * Callers zero-initialize destination and set destination.struct_size before
 * copying. A destination that already owns storage must be released first.
 * Release is idempotent and does not require the source context or transport
 * to remain alive. An owned response must not be copied by value because its
 * storage has exactly one owner.
 *
 * ulk_command_response_copy_owned_v1 is the convenience entry point and keeps
 * its 1 MiB aggregate byte limit. The options entry point accepts a null
 * options pointer for the same defaults. A non-null options object must set
 * struct_size. Its null allocator selects the library default, while
 * maximum_total_bytes selects the 1 MiB default when zero and otherwise is the
 * caller's exact aggregate limit. A nonzero options limit must also fit the
 * current platform's addressable size. The options and allocator structures
 * are borrowed only for the call. Custom allocator callback code and user
 * state must remain valid until release because the callback values are copied
 * into the owner.
 */

#ifdef __cplusplus
}
#endif

#endif
