// SPDX-FileCopyrightText: 2026 Jules C
// SPDX-License-Identifier: MIT

#include "ulk/ulk_api.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
#include <direct.h>
#include <process.h>
#define ULK_TEST_PID _getpid
#else
#include <unistd.h>
#define ULK_TEST_PID getpid
#endif

#define CHECK(condition, code) do { if (!(condition)) return (code); } while (0)

static ulk_string_view view(const char* text)
{
    ulk_string_view result;
    result.data = text;
    result.size = text == 0 ? 0u : (ulk_size)strlen(text);
    return result;
}

static int view_is(ulk_string_view actual, const char* expected)
{
    const size_t size = strlen(expected);
    return actual.size == (ulk_size)size &&
        (size == 0u || memcmp(actual.data, expected, size) == 0);
}

static void initialize_error(ulk_error_v1* error)
{
    memset(error, 0, sizeof(*error));
    error->struct_size = sizeof(*error);
}

static void initialize_running(
    ulk_session_record_v1* record,
    const char* session_id,
    const char* operation_id,
    const char* attempt_id,
    const char* runnable,
    const char* started_at)
{
    memset(record, 0, sizeof(*record));
    record->struct_size = sizeof(*record);
    record->session_id = view(session_id);
    record->identity.struct_size = sizeof(record->identity);
    record->identity.operation_id = view(operation_id);
    record->identity.attempt_id = view(attempt_id);
    record->runnable_reference = view(runnable);
    record->process_identity = view("fixture-process-42");
    record->state = ULK_SESSION_RUNNING;
    record->started_at = view(started_at);
    record->ended_at = view(0);
    record->recovery_reference = view(0);
    record->relaunch_reference = view(0);
}

static void initialize_terminal_result(
    ulk_operation_result_v1* result,
    const ulk_session_record_v1* record,
    ulk_operation_outcome_v1 outcome)
{
    memset(result, 0, sizeof(*result));
    result->struct_size = sizeof(*result);
    result->identity = record->identity;
    result->outcome = outcome;
    result->recovery.struct_size = sizeof(result->recovery);
    if (outcome == ULK_OPERATION_COMPLETED ||
        outcome == ULK_OPERATION_CANCELLATION_REQUESTED_BUT_COMPLETED) {
        result->effects_may_have_occurred = 1;
    } else if (outcome == ULK_OPERATION_RECOVERY_REQUIRED ||
        outcome == ULK_OPERATION_OUTCOME_UNKNOWN) {
        result->effects_may_have_occurred = 1;
        result->recovery.required = 1;
        result->recovery.transaction_id = view("transaction-1");
        result->recovery.inspect_command = view("session.recovery.inspect");
    }
}

static void make_terminal(
    ulk_session_record_v1* record,
    ulk_operation_result_v1* result,
    ulk_operation_outcome_v1 outcome,
    const char* ended_at)
{
    initialize_terminal_result(result, record, outcome);
    record->state = ULK_SESSION_TERMINAL;
    record->ended_at = view(ended_at);
    record->exit_code_known = outcome == ULK_OPERATION_COMPLETED;
    record->exit_code = outcome == ULK_OPERATION_COMPLETED ? 0 : 0;
    record->terminal_result = result;
    if (result->recovery.required) {
        record->recovery_reference = view("recovery/session-1");
    }
    record->relaunch_reference = view("relaunch/runnable-1");
}

static int render_inspect(
    const ulk_session_journal_v1* journal,
    const char* session_id,
    char* output,
    size_t output_size)
{
    ulk_session_lookup_status_v1 lookup = ULK_SESSION_LOOKUP_NOT_FOUND;
    ulk_size required = 0u;
    ulk_error_v1 error;
    initialize_error(&error);
    if (ulk_session_journal_inspect_v1(journal, view(session_id), &lookup,
            0, 0u, &required, &error) != ULK_STATUS_OK ||
        lookup != ULK_SESSION_LOOKUP_FOUND || required > (ulk_size)output_size) return 0;
    if (ulk_session_journal_inspect_v1(journal, view(session_id), &lookup,
            output, (ulk_size)output_size, &required, &error) != ULK_STATUS_OK) return 0;
    return strstr(output, "\"schema\":\"ulk.session_record.v1\"") != 0;
}

static void cleanup_root(const char* root, const char* const* session_ids, size_t count)
{
    char path[1024];
    size_t index;
    for (index = 0u; index < count; ++index) {
        (void)snprintf(path, sizeof(path), "%s/sessions/%s.session", root, session_ids[index]);
        (void)remove(path);
        (void)snprintf(path, sizeof(path), "%s/sessions/%s.session.tmp", root, session_ids[index]);
        (void)remove(path);
    }
    (void)snprintf(path, sizeof(path), "%s/.ulk-session.lock", root);
    (void)remove(path);
    (void)snprintf(path, sizeof(path), "%s/sessions", root);
#if defined(_WIN32)
    (void)_rmdir(path);
    (void)_rmdir(root);
#else
    (void)rmdir(path);
    (void)rmdir(root);
#endif
}

static unsigned long crc32_bytes(const unsigned char* bytes, size_t size)
{
    unsigned long crc = 0xfffffffful;
    size_t index;
    for (index = 0u; index < size; ++index) {
        unsigned int bit;
        crc ^= bytes[index];
        for (bit = 0u; bit < 8u; ++bit) {
            crc = (crc >> 1u) ^ (0xedb88320ul & (0ul - (crc & 1ul)));
        }
    }
    return crc ^ 0xfffffffful;
}

static int write_bytes(const char* path, const char* bytes, size_t size)
{
    FILE* stream = 0;
#if defined(_WIN32)
    if (fopen_s(&stream, path, "wb") != 0) stream = 0;
#else
    stream = fopen(path, "wb");
#endif
    if (stream == 0) return 0;
    if (size != 0u && fwrite(bytes, 1u, size, stream) != size) {
        fclose(stream);
        return 0;
    }
    return fclose(stream) == 0;
}

static int convert_record_to_future(const char* path)
{
    char bytes[65536];
    char* checksum;
    FILE* stream = 0;
    size_t size;
    unsigned long crc;
#if defined(_WIN32)
    if (fopen_s(&stream, path, "rb") != 0) stream = 0;
#else
    stream = fopen(path, "rb");
#endif
    if (stream == 0) return 0;
    size = fread(bytes, 1u, sizeof(bytes) - 1u, stream);
    if (ferror(stream) || fclose(stream) != 0 || size < 32u) return 0;
    bytes[size] = '\0';
    if (strncmp(bytes, "ULK_SESSION_RECORD_V1|", 22u) != 0) return 0;
    bytes[20] = '2';
    checksum = strrchr(bytes, '|');
    if (checksum == 0 || (size_t)(checksum - bytes) + 10u != size) return 0;
    crc = crc32_bytes((const unsigned char*)bytes, (size_t)(checksum - bytes + 1));
    (void)snprintf(checksum + 1, 10u, "%08lx\n", crc);
    return write_bytes(path, bytes, size);
}

static int prove_failure_modes(int process_id)
{
    char root[256];
    char path[512];
    char json[2048];
    ulk_size required = 0u;
    ulk_session_journal_v1 journal;
    ulk_session_record_v1 record;
    ulk_error_v1 error;
    const char* id[] = {"failure-session", "orphan"};

    (void)snprintf(root, sizeof(root), "ulk-session-corrupt-%d", process_id);
    memset(&journal, 0, sizeof(journal));
    journal.struct_size = sizeof(journal);
    journal.root = view(root);
    initialize_running(&record, id[0], "failure-operation-1", "failure-attempt-1",
        "fixture://failure/corrupt", "2026-08-12T01:00:00Z");
    initialize_error(&error);
    if (ulk_session_journal_write_v1(&journal, &record, &error) != ULK_STATUS_OK) return 0;
    (void)snprintf(path, sizeof(path), "%s/sessions/%s.session", root, id[0]);
    if (!write_bytes(path, "corrupt\n", 8u) ||
        ulk_session_journal_list_v1(&journal, 1u, json, sizeof(json),
            &required, &error) != ULK_STATUS_ERROR ||
        !view_is(error.detail, "session_record_corrupt")) return 0;
    cleanup_root(root, id, 1u);

    (void)snprintf(root, sizeof(root), "ulk-session-future-%d", process_id);
    journal.root = view(root);
    record.identity.operation_id = view("failure-operation-2");
    record.identity.attempt_id = view("failure-attempt-2");
    record.runnable_reference = view("fixture://failure/future");
    if (ulk_session_journal_write_v1(&journal, &record, &error) != ULK_STATUS_OK) return 0;
    (void)snprintf(path, sizeof(path), "%s/sessions/%s.session", root, id[0]);
    if (!convert_record_to_future(path) ||
        ulk_session_journal_list_v1(&journal, 1u, json, sizeof(json),
            &required, &error) != ULK_STATUS_ERROR ||
        !view_is(error.detail, "session_record_incompatible")) return 0;
    cleanup_root(root, id, 1u);

    (void)snprintf(root, sizeof(root), "ulk-session-interrupted-%d", process_id);
    journal.root = view(root);
    record.identity.operation_id = view("failure-operation-3");
    record.identity.attempt_id = view("failure-attempt-3");
    record.runnable_reference = view("fixture://failure/interrupted");
    if (ulk_session_journal_write_v1(&journal, &record, &error) != ULK_STATUS_OK) return 0;
    (void)snprintf(path, sizeof(path), "%s/sessions/orphan.session.tmp", root);
    if (!write_bytes(path, "interrupted", 11u) ||
        ulk_session_journal_list_v1(&journal, 1u, json, sizeof(json),
            &required, &error) != ULK_STATUS_OK || strstr(json, id[0]) == 0) return 0;
    cleanup_root(root, id, 2u);
    return 1;
}

int main(void)
{
    char root[256];
    char json[8192];
    ulk_size required = 0u;
    ulk_session_lookup_status_v1 lookup = ULK_SESSION_LOOKUP_NOT_FOUND;
    ulk_session_journal_v1 journal;
    ulk_session_record_v1 first;
    ulk_session_record_v1 second;
    ulk_session_record_v1 third;
    ulk_operation_result_v1 first_result;
    ulk_operation_result_v1 second_result;
    ulk_operation_result_v1 third_result;
    ulk_error_v1 error;
    const char* ids[] = {"session-1", "session-2", "session-3"};

    (void)snprintf(root, sizeof(root), "ulk-session-\xE6\xB5\x8B\xE8\xAF\x95-%d", ULK_TEST_PID());
    memset(&journal, 0, sizeof(journal));
    journal.struct_size = sizeof(journal);
    journal.root = view(root);
    journal.maximum_records = 2u;
    initialize_error(&error);

    CHECK(ULK_API_VERSION_MAJOR == 1 && ULK_API_VERSION_MINOR == 9, 1);
    CHECK(ulk_session_journal_validate_v1(&journal) == ULK_STATUS_OK, 2);
    initialize_running(&first, ids[0], "operation-1", "attempt-1",
        "fixture://runnable/alpha", "2026-08-12T00:00:01Z");
    CHECK(ulk_session_record_validate_v1(&first) == ULK_STATUS_OK, 3);
    CHECK(ulk_session_journal_write_v1(&journal, &first, &error) == ULK_STATUS_OK, 4);
    CHECK(ulk_session_journal_write_v1(&journal, &first, &error) == ULK_STATUS_OK, 5);
    CHECK(render_inspect(&journal, ids[0], json, sizeof(json)), 6);
    CHECK(strstr(json, "\"state\":\"running\"") != 0, 7);

    first.process_identity = view("different-process");
    CHECK(ulk_session_journal_write_v1(&journal, &first, &error) == ULK_STATUS_ERROR, 8);
    CHECK(view_is(error.detail, "session_idempotency_conflict"), 9);
    first.process_identity = view("fixture-process-42");
    make_terminal(&first, &first_result, ULK_OPERATION_COMPLETED,
        "2026-08-12T00:00:02Z");
    CHECK(ulk_session_record_validate_v1(&first) == ULK_STATUS_OK, 10);
    CHECK(ulk_session_journal_write_v1(&journal, &first, &error) == ULK_STATUS_OK, 11);
    CHECK(ulk_session_journal_write_v1(&journal, &first, &error) == ULK_STATUS_OK, 12);
    CHECK(render_inspect(&journal, ids[0], json, sizeof(json)), 13);
    CHECK(strstr(json, "\"outcome\":\"completed\"") != 0, 14);
    first.exit_code = 17;
    CHECK(ulk_session_journal_write_v1(&journal, &first, &error) == ULK_STATUS_ERROR, 15);
    CHECK(view_is(error.detail, "session_terminal_immutable"), 16);

    initialize_running(&second, ids[1], "operation-2", "attempt-2",
        "fixture://runnable/alpha", "2026-08-12T00:00:03Z");
    make_terminal(&second, &second_result, ULK_OPERATION_OUTCOME_UNKNOWN,
        "2026-08-12T00:00:04Z");
    CHECK(ulk_session_record_validate_v1(&second) == ULK_STATUS_OK, 17);
    CHECK(ulk_session_journal_write_v1(&journal, &second, &error) == ULK_STATUS_OK, 18);
    CHECK(ulk_session_journal_last_run_v1(&journal, view("fixture://runnable/alpha"),
        &lookup, 0, 0u, &required, &error) == ULK_STATUS_OK, 19);
    CHECK(lookup == ULK_SESSION_LOOKUP_FOUND && required <= sizeof(json), 20);
    CHECK(ulk_session_journal_last_run_v1(&journal, view("fixture://runnable/alpha"),
        &lookup, json, sizeof(json), &required, &error) == ULK_STATUS_OK, 21);
    CHECK(strstr(json, "\"outcome\":\"outcome_unknown\"") != 0, 22);
    CHECK(strstr(json, "\"recovery_reference\":\"recovery/session-1\"") != 0, 23);

    initialize_running(&third, ids[2], "operation-3", "attempt-3",
        "fixture://runnable/beta", "2026-08-12T00:00:05Z");
    make_terminal(&third, &third_result, ULK_OPERATION_RECOVERY_REQUIRED,
        "2026-08-12T00:00:06Z");
    CHECK(ulk_session_journal_write_v1(&journal, &third, &error) == ULK_STATUS_OK, 24);
    CHECK(ulk_session_journal_inspect_v1(&journal, view(ids[0]), &lookup,
        0, 0u, &required, &error) == ULK_STATUS_OK, 25);
    CHECK(lookup == ULK_SESSION_LOOKUP_NOT_FOUND, 26);
    CHECK(ulk_session_journal_list_v1(&journal, 2u, json, sizeof(json),
        &required, &error) == ULK_STATUS_OK, 27);
    CHECK(strstr(json, "\"schema\":\"ulk.session_list.v1\"") != 0, 28);
    CHECK(strstr(json, "session-2") != 0 && strstr(json, "session-3") != 0, 29);
    CHECK(ulk_session_journal_list_v1(&journal, 2u, json, 8u,
        &required, &error) == ULK_STATUS_ERROR, 30);
    CHECK(view_is(error.detail, "session_output_buffer_too_small"), 31);

    CHECK(prove_failure_modes(ULK_TEST_PID()), 32);

    cleanup_root(root, ids, sizeof(ids) / sizeof(ids[0]));
    return 0;
}
