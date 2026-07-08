#ifndef ULK_ARTIFACT_SET_H
#define ULK_ARTIFACT_SET_H

#include "ulk_types.h"

typedef struct ulk_artifact_set_ref_v1 {
    ulk_size struct_size;
    ulk_string_view product_id;
    ulk_string_view artifact_set_id;
    ulk_string_view lock_json;
} ulk_artifact_set_ref_v1;

#endif
