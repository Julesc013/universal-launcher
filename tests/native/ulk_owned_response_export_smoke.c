// SPDX-FileCopyrightText: 2026 Jules C
// SPDX-License-Identifier: MIT

#include "ulk/ulk_api.h"

#include <string.h>

#define CHECK(condition, code) do { if (!(condition)) return (code); } while (0)

int main(void)
{
    ulk_command_response_v1 source;
    ulk_owned_command_response_options_v1 options;
    ulk_owned_command_response_v1 owned;

    memset(&source, 0, sizeof(source));
    memset(&options, 0, sizeof(options));
    memset(&owned, 0, sizeof(owned));
    source.struct_size = sizeof(source);
    source.status = ULK_STATUS_OK;
    source.error.struct_size = sizeof(source.error);
    options.struct_size = sizeof(options);
    options.maximum_total_bytes = 0u;
    owned.struct_size = sizeof(owned);

    CHECK(ulk_command_response_validate_v1(&source) == ULK_STATUS_OK, 1);
    CHECK(
        ulk_command_response_copy_owned_v1(&source, 0, &owned) ==
            ULK_STATUS_OK,
        2);
    ulk_owned_command_response_release_v1(&owned);
    CHECK(
        ulk_command_response_copy_owned_with_options_v1(
            &source,
            &options,
            &owned) == ULK_STATUS_OK,
        3);
    ulk_owned_command_response_release_v1(&owned);
    return 0;
}
