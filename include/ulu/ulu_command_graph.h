// SPDX-FileCopyrightText: 2026 Jules C
// SPDX-License-Identifier: MIT

#ifndef ULU_COMMAND_GRAPH_H
#define ULU_COMMAND_GRAPH_H

#include "ulu_callbacks.h"

typedef struct ulu_command_graph_options_v1 {
    ulu_size struct_size;
    const ulu_callbacks_v1* callbacks;
    ulu_bool dry_run_default;
} ulu_command_graph_options_v1;

#endif
