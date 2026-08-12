// SPDX-FileCopyrightText: 2026 Jules C
// SPDX-License-Identifier: MIT

#ifndef ULK_SESSION_H
#define ULK_SESSION_H

#include "ulk_error.h"
#include "ulk_operation.h"
#include "ulk_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum ulk_session_state_v1 {
    ULK_SESSION_RUNNING = 1,
    ULK_SESSION_TERMINAL = 2
} ulk_session_state_v1;

typedef enum ulk_session_lookup_status_v1 {
    ULK_SESSION_LOOKUP_FOUND = 1,
    ULK_SESSION_LOOKUP_NOT_FOUND = 2,
    ULK_SESSION_LOOKUP_CORRUPT = 3,
    ULK_SESSION_LOOKUP_INCOMPATIBLE = 4
} ulk_session_lookup_status_v1;

typedef struct ulk_session_journal_v1 {
    ulk_size struct_size;
    ulk_string_view root;
    ulk_size maximum_records;
} ulk_session_journal_v1;

typedef struct ulk_session_record_v1 {
    ulk_size struct_size;
    ulk_string_view session_id;
    ulk_operation_identity_v1 identity;
    ulk_string_view runnable_reference;
    ulk_string_view process_identity;
    ulk_session_state_v1 state;
    ulk_string_view started_at;
    ulk_string_view ended_at;
    ulk_bool exit_code_known;
    int64_t exit_code;
    const ulk_operation_result_v1* terminal_result;
    ulk_string_view recovery_reference;
    ulk_string_view relaunch_reference;
} ulk_session_record_v1;

ULK_API int ULK_CALL ulk_session_journal_validate_v1(
    const ulk_session_journal_v1* journal
);

ULK_API int ULK_CALL ulk_session_record_validate_v1(
    const ulk_session_record_v1* record
);

ULK_API int ULK_CALL ulk_session_journal_write_v1(
    const ulk_session_journal_v1* journal,
    const ulk_session_record_v1* record,
    ulk_error_v1* error
);

ULK_API int ULK_CALL ulk_session_journal_inspect_v1(
    const ulk_session_journal_v1* journal,
    ulk_string_view session_id,
    ulk_session_lookup_status_v1* lookup_status,
    char* json,
    ulk_size json_capacity,
    ulk_size* required_capacity,
    ulk_error_v1* error
);

ULK_API int ULK_CALL ulk_session_journal_last_run_v1(
    const ulk_session_journal_v1* journal,
    ulk_string_view runnable_reference,
    ulk_session_lookup_status_v1* lookup_status,
    char* json,
    ulk_size json_capacity,
    ulk_size* required_capacity,
    ulk_error_v1* error
);

ULK_API int ULK_CALL ulk_session_journal_list_v1(
    const ulk_session_journal_v1* journal,
    ulk_size limit,
    char* json,
    ulk_size json_capacity,
    ulk_size* required_capacity,
    ulk_error_v1* error
);

/*
 * The journal root and every input view are borrowed for the duration of the
 * call. JSON output uses a two-call caller-buffer law: required_capacity
 * includes the trailing NUL. A null/zero buffer is a successful size probe.
 * The store is product-neutral and caller-rooted. It records no process by
 * itself and grants no launch, setup, network, signing, or publication power.
 * This ABI is experimental in the 1.9 development train.
 */

#ifdef __cplusplus
}
#endif

#endif
