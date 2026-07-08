#ifndef ULU_UI_MODEL_H
#define ULU_UI_MODEL_H

#include "ulu_abi.h"

typedef struct ulu_ui_model_v1 {
    ulu_size struct_size;
    ulu_string_view json_payload;
} ulu_ui_model_v1;

#endif
