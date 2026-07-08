#ifndef ULU_API_H
#define ULU_API_H

#include "ulu_command_graph.h"
#include "ulu_platform_iface.h"
#include "ulu_report.h"
#include "ulu_ui_model.h"

#ifdef __cplusplus
extern "C" {
#endif

int ulu_describe_command_graph_v1(
    const ulu_command_graph_options_v1* options,
    ulu_report_v1* out_report
);

#ifdef __cplusplus
}
#endif

#endif
