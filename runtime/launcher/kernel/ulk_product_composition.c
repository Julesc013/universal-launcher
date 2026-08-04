// SPDX-FileCopyrightText: 2026 Jules C
// SPDX-License-Identifier: MIT

#include "ulk/ulk_product_composition.h"

#include "ulk/ulk_result.h"

#include <stddef.h>

static int ulk_composition_view_valid(ulk_string_view value, int allow_empty)
{
    ulk_size index;
    if (value.size == 0u) {
        return allow_empty;
    }
    if (value.data == 0) {
        return 0;
    }
    for (index = 0u; index < value.size; ++index) {
        if (value.data[index] == '\0') {
            return 0;
        }
    }
    return 1;
}

static int ulk_composition_identifier_valid(ulk_string_view value, int allow_empty)
{
    ulk_size index;
    if (value.size == 0u) {
        return allow_empty;
    }
    if (value.data == 0 || value.size > 128u) {
        return 0;
    }
    for (index = 0u; index < value.size; ++index) {
        unsigned char current = (unsigned char)value.data[index];
        int valid =
            (current >= (unsigned char)'a' && current <= (unsigned char)'z') ||
            (current >= (unsigned char)'A' && current <= (unsigned char)'Z') ||
            (current >= (unsigned char)'0' && current <= (unsigned char)'9') ||
            current == (unsigned char)'.' ||
            current == (unsigned char)'_' ||
            current == (unsigned char)'-';
        if (!valid) {
            return 0;
        }
    }
    return 1;
}

static int ulk_composition_sha256_valid(ulk_string_view value)
{
    ulk_size index;
    if (value.data == 0 || value.size != 64u) {
        return 0;
    }
    for (index = 0u; index < value.size; ++index) {
        unsigned char current = (unsigned char)value.data[index];
        if (!((current >= (unsigned char)'0' && current <= (unsigned char)'9') ||
              (current >= (unsigned char)'a' && current <= (unsigned char)'f'))) {
            return 0;
        }
    }
    return 1;
}

static int ulk_composition_relative_path_valid(ulk_string_view value)
{
    ulk_size index;
    if (!ulk_composition_view_valid(value, 0) ||
        value.data[0] == '/' || value.data[0] == '\\') {
        return 0;
    }
    for (index = 0u; index < value.size; ++index) {
        if (value.data[index] == '\\') {
            return 0;
        }
        if (value.data[index] == '.' && index + 1u < value.size &&
            value.data[index + 1u] == '.' &&
            (index == 0u || value.data[index - 1u] == '/') &&
            (index + 2u == value.size || value.data[index + 2u] == '/')) {
            return 0;
        }
    }
    return 1;
}

int ULK_CALL ulk_contract_set_identity_validate_v1(
    const ulk_contract_set_identity_v1* identity
)
{
    if (identity == 0 ||
        identity->struct_size < (ulk_size)sizeof(*identity) ||
        !ulk_composition_identifier_valid(identity->contract_set_id, 0) ||
        !ulk_composition_view_valid(identity->version, 0) ||
        !ulk_composition_sha256_valid(identity->sha256)) {
        return ULK_STATUS_INVALID_ARGUMENT;
    }
    return ULK_STATUS_OK;
}

int ULK_CALL ulk_launch_capability_validate_v1(
    const ulk_launch_capability_v1* capability
)
{
    if (capability == 0 ||
        capability->struct_size < (ulk_size)sizeof(*capability) ||
        capability->kind < ULK_LAUNCH_CAPABILITY_SINGLE_PROCESS ||
        capability->kind > ULK_LAUNCH_CAPABILITY_SERVER ||
        (capability->required != 0 && capability->required != 1)) {
        return ULK_STATUS_INVALID_ARGUMENT;
    }
    return ULK_STATUS_OK;
}

int ULK_CALL ulk_entrypoint_descriptor_validate_v1(
    const ulk_entrypoint_descriptor_v1* entrypoint
)
{
    ulk_size index;
    if (entrypoint == 0 ||
        entrypoint->struct_size < (ulk_size)sizeof(*entrypoint) ||
        !ulk_composition_identifier_valid(entrypoint->entrypoint_id, 0) ||
        !ulk_composition_relative_path_valid(entrypoint->relative_path) ||
        !ulk_composition_identifier_valid(entrypoint->artifact_set_id, 1) ||
        entrypoint->capability_count == 0u ||
        entrypoint->capabilities == 0) {
        return ULK_STATUS_INVALID_ARGUMENT;
    }
    for (index = 0u; index < entrypoint->capability_count; ++index) {
        if (ulk_launch_capability_validate_v1(&entrypoint->capabilities[index]) !=
            ULK_STATUS_OK) {
            return ULK_STATUS_INVALID_ARGUMENT;
        }
    }
    return ULK_STATUS_OK;
}

int ULK_CALL ulk_product_descriptor_validate_v2(
    const ulk_product_descriptor_v2* product
)
{
    if (product == 0 ||
        product->struct_size < (ulk_size)sizeof(*product) ||
        !ulk_composition_identifier_valid(product->product_id, 0) ||
        !ulk_composition_view_valid(product->exact_version, 0) ||
        !ulk_composition_identifier_valid(product->publisher_id, 0) ||
        !ulk_composition_view_valid(product->composition_revision, 0)) {
        return ULK_STATUS_INVALID_ARGUMENT;
    }
    return ULK_STATUS_OK;
}

int ULK_CALL ulk_product_composition_validate_v1(
    const ulk_product_composition_v1* composition
)
{
    ulk_size index;
    if (composition == 0 ||
        composition->struct_size < (ulk_size)sizeof(*composition) ||
        composition->product == 0 ||
        composition->entrypoints == 0 ||
        composition->entrypoint_count == 0u ||
        composition->contract_set == 0 ||
        ulk_product_descriptor_validate_v2(composition->product) != ULK_STATUS_OK ||
        ulk_contract_set_identity_validate_v1(composition->contract_set) != ULK_STATUS_OK) {
        return ULK_STATUS_INVALID_ARGUMENT;
    }
    for (index = 0u; index < composition->entrypoint_count; ++index) {
        if (ulk_entrypoint_descriptor_validate_v1(&composition->entrypoints[index]) !=
            ULK_STATUS_OK) {
            return ULK_STATUS_INVALID_ARGUMENT;
        }
    }
    return ULK_STATUS_OK;
}
