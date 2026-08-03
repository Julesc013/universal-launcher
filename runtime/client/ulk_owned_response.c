// SPDX-FileCopyrightText: 2026 Jules C
// SPDX-License-Identifier: MIT

#include "ulk/ulk_owned_response.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static void* ULK_CALL ulk_owned_default_alloc(void* user, ulk_size size)
{
    (void)user;
    return malloc((size_t)size);
}

static void ULK_CALL ulk_owned_default_free(void* user, void* pointer)
{
    (void)user;
    free(pointer);
}

static int ulk_owned_view_is_valid(ulk_string_view value)
{
    return value.size == 0u || value.data != 0;
}

static int ulk_owned_status_is_valid(int status)
{
    return
        status == ULK_STATUS_OK ||
        status == ULK_STATUS_ERROR ||
        status == ULK_STATUS_INVALID_ARGUMENT ||
        status == ULK_STATUS_UNSUPPORTED_VERSION;
}

static int ulk_owned_allocator_select(
    const ulk_allocator_v1* requested,
    ulk_allocator_v1* effective
)
{
    if (effective == 0) {
        return 0;
    }

    memset(effective, 0, sizeof(*effective));
    effective->struct_size = sizeof(*effective);
    effective->alloc = ulk_owned_default_alloc;
    effective->free = ulk_owned_default_free;

    if (requested == 0) {
        return 1;
    }
    if (
        requested->struct_size < (ulk_size)sizeof(*requested) ||
        ((requested->alloc == 0) != (requested->free == 0))
    ) {
        return 0;
    }
    if (requested->alloc != 0) {
        *effective = *requested;
    }
    return 1;
}

static int ulk_owned_size_add(
    ulk_size left,
    ulk_size right,
    ulk_size* result
)
{
    if (result == 0 || left > (ulk_size)-1 - right) {
        return 0;
    }
    *result = left + right;
    return 1;
}

static void ulk_owned_response_reset(ulk_owned_command_response_v1* response)
{
    if (response == 0) {
        return;
    }
    memset(response, 0, sizeof(*response));
    response->struct_size = sizeof(*response);
    response->response.struct_size = sizeof(response->response);
    response->response.error.struct_size = sizeof(response->response.error);
}

int ULK_CALL ulk_command_response_validate_v1(
    const ulk_command_response_v1* response
)
{
    if (
        response == 0 ||
        response->struct_size < (ulk_size)sizeof(*response) ||
        !ulk_owned_status_is_valid(response->status) ||
        !ulk_owned_view_is_valid(response->json_payload) ||
        response->error.struct_size < (ulk_size)sizeof(response->error) ||
        !ulk_owned_view_is_valid(response->error.message) ||
        !ulk_owned_view_is_valid(response->error.detail)
    ) {
        return ULK_STATUS_INVALID_ARGUMENT;
    }
    return ULK_STATUS_OK;
}

int ULK_CALL ulk_command_response_copy_owned_v1(
    const ulk_command_response_v1* source,
    const ulk_allocator_v1* allocator,
    ulk_owned_command_response_v1* destination
)
{
    ulk_allocator_v1 effective_allocator;
    ulk_command_response_v1 source_value;
    ulk_size storage_size = 0u;
    unsigned char* storage = 0;
    unsigned char* cursor;

    if (
        destination == 0 ||
        destination->struct_size < (ulk_size)sizeof(*destination) ||
        destination->storage != 0
    ) {
        return ULK_STATUS_INVALID_ARGUMENT;
    }

    if (ulk_command_response_validate_v1(source) != ULK_STATUS_OK) {
        ulk_owned_response_reset(destination);
        return ULK_STATUS_INVALID_ARGUMENT;
    }
    source_value = *source;

    if (!ulk_owned_allocator_select(allocator, &effective_allocator)) {
        ulk_owned_response_reset(destination);
        return ULK_STATUS_INVALID_ARGUMENT;
    }
    if (
        !ulk_owned_size_add(
            source_value.json_payload.size,
            source_value.error.message.size,
            &storage_size) ||
        !ulk_owned_size_add(
            storage_size,
            source_value.error.detail.size,
            &storage_size) ||
        storage_size > ULK_OWNED_COMMAND_RESPONSE_BYTE_BUDGET_V1 ||
        storage_size > (ulk_size)SIZE_MAX
    ) {
        ulk_owned_response_reset(destination);
        return ULK_STATUS_INVALID_ARGUMENT;
    }

    if (storage_size != 0u) {
        storage = (unsigned char*)effective_allocator.alloc(
            effective_allocator.user,
            storage_size);
        if (storage == 0) {
            ulk_owned_response_reset(destination);
            return ULK_STATUS_ERROR;
        }
    }

    cursor = storage;
    if (source_value.json_payload.size != 0u) {
        memcpy(
            cursor,
            source_value.json_payload.data,
            (size_t)source_value.json_payload.size);
        cursor += source_value.json_payload.size;
    }
    if (source_value.error.message.size != 0u) {
        memcpy(
            cursor,
            source_value.error.message.data,
            (size_t)source_value.error.message.size);
        cursor += source_value.error.message.size;
    }
    if (source_value.error.detail.size != 0u) {
        memcpy(
            cursor,
            source_value.error.detail.data,
            (size_t)source_value.error.detail.size);
    }

    ulk_owned_response_reset(destination);
    destination->allocator = effective_allocator;
    destination->storage = storage;
    destination->storage_size = storage_size;
    destination->response.status = source_value.status;
    destination->response.error.code = source_value.error.code;

    cursor = storage;
    if (source_value.json_payload.size != 0u) {
        destination->response.json_payload.data = (const char*)cursor;
        destination->response.json_payload.size = source_value.json_payload.size;
        cursor += source_value.json_payload.size;
    }
    if (source_value.error.message.size != 0u) {
        destination->response.error.message.data = (const char*)cursor;
        destination->response.error.message.size = source_value.error.message.size;
        cursor += source_value.error.message.size;
    }
    if (source_value.error.detail.size != 0u) {
        destination->response.error.detail.data = (const char*)cursor;
        destination->response.error.detail.size = source_value.error.detail.size;
    }
    return ULK_STATUS_OK;
}

void ULK_CALL ulk_owned_command_response_release_v1(
    ulk_owned_command_response_v1* response
)
{
    if (
        response == 0 ||
        response->struct_size < (ulk_size)sizeof(*response)
    ) {
        return;
    }
    if (response->storage != 0 && response->allocator.free != 0) {
        response->allocator.free(response->allocator.user, response->storage);
    }
    ulk_owned_response_reset(response);
}
