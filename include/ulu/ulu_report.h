#ifndef ULU_REPORT_H
#define ULU_REPORT_H

#include "ulu_abi.h"

typedef struct ulu_report_v1 {
    ulu_size struct_size;
    int status;
    ulu_string_view json_payload;
} ulu_report_v1;

#endif
