// SPDX-FileCopyrightText: 2026 Jules C
// SPDX-License-Identifier: MIT

#include "ulk/ulk_api.h"
#include "ulu/ulu_api.h"

int main()
{
    return ULK_API_VERSION_MAJOR == 1 && ULK_API_VERSION_MINOR == 8 ? 0 : 1;
}
