// SPDX-FileCopyrightText: 2026 Jules C
// SPDX-License-Identifier: MIT

#include "ulk/ulk_session.h"

#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <io.h>
#include <process.h>
#else
#include <dirent.h>
#include <fcntl.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#endif

#define ULK_SESSION_DEFAULT_RECORDS 64u
#define ULK_SESSION_MAX_RECORDS 1024u
#define ULK_SESSION_MAX_FILE_BYTES 65536u
#define ULK_SESSION_MAX_ROOT_BYTES 32768u
#define ULK_SESSION_MAX_RUNNABLE_BYTES 4096u
#define ULK_SESSION_MAX_PROCESS_BYTES 1024u
#define ULK_SESSION_MAX_TIME_BYTES 64u
#define ULK_SESSION_MAX_REFERENCE_BYTES 128u

typedef struct ulk_session_owned_record {
    char session_id[129];
    char operation_id[129];
    char attempt_id[129];
    char runnable_reference[ULK_SESSION_MAX_RUNNABLE_BYTES + 1u];
    char process_identity[ULK_SESSION_MAX_PROCESS_BYTES + 1u];
    char started_at[ULK_SESSION_MAX_TIME_BYTES + 1u];
    char ended_at[ULK_SESSION_MAX_TIME_BYTES + 1u];
    char recovery_transaction[129];
    char recovery_inspect_command[129];
    char recovery_reference[ULK_SESSION_MAX_REFERENCE_BYTES + 1u];
    char relaunch_reference[ULK_SESSION_MAX_REFERENCE_BYTES + 1u];
    ulk_session_state_v1 state;
    ulk_operation_outcome_v1 outcome;
    int effects_may_have_occurred;
    int recovery_required;
    int exit_code_known;
    int64_t exit_code;
} ulk_session_owned_record;

typedef struct ulk_session_record_set {
    ulk_session_owned_record* values;
    size_t count;
    size_t capacity;
} ulk_session_record_set;

typedef struct ulk_text_builder {
    char* data;
    size_t size;
    size_t capacity;
} ulk_text_builder;

typedef enum ulk_disk_record_status {
    ULK_DISK_RECORD_OK = 0,
    ULK_DISK_RECORD_CORRUPT = 1,
    ULK_DISK_RECORD_INCOMPATIBLE = 2
} ulk_disk_record_status;

typedef struct ulk_session_lock {
#if defined(_WIN32)
    HANDLE handle;
#else
    int descriptor;
#endif
} ulk_session_lock;

static ulk_string_view ulk_static_view(const char* text)
{
    ulk_string_view result;
    result.data = text;
    result.size = text == 0 ? 0u : (ulk_size)strlen(text);
    return result;
}

static int ulk_fail(ulk_error_v1* error, int status, const char* message, const char* detail)
{
    if (error != 0 && error->struct_size >= (ulk_size)sizeof(*error)) {
        error->struct_size = sizeof(*error);
        error->code = status;
        error->message = ulk_static_view(message);
        error->detail = ulk_static_view(detail);
    }
    return status;
}

static void ulk_clear_error(ulk_error_v1* error)
{
    if (error != 0 && error->struct_size >= (ulk_size)sizeof(*error)) {
        error->struct_size = sizeof(*error);
        error->code = ULK_STATUS_OK;
        error->message = ulk_static_view(0);
        error->detail = ulk_static_view(0);
    }
}

static int ulk_view_valid(ulk_string_view value, ulk_size maximum, int allow_empty)
{
    if (value.size == 0u) {
        return allow_empty;
    }
    return value.data != 0 && value.size <= maximum;
}

static int ulk_identifier_valid(ulk_string_view value)
{
    ulk_size index;
    unsigned char first;
    if (!ulk_view_valid(value, 128u, 0)) return 0;
    first = (unsigned char)value.data[0];
    if (!((first >= 'A' && first <= 'Z') || (first >= 'a' && first <= 'z') ||
          (first >= '0' && first <= '9'))) return 0;
    for (index = 1u; index < value.size; ++index) {
        const unsigned char character = (unsigned char)value.data[index];
        if (!((character >= 'A' && character <= 'Z') ||
              (character >= 'a' && character <= 'z') ||
              (character >= '0' && character <= '9') ||
              character == '.' || character == '_' || character == ':' || character == '-')) return 0;
    }
    return 1;
}

static int ulk_views_equal(ulk_string_view left, ulk_string_view right)
{
    return left.size == right.size &&
        (left.size == 0u || memcmp(left.data, right.data, (size_t)left.size) == 0);
}

static int ulk_view_equals_text(ulk_string_view left, const char* right)
{
    const size_t length = strlen(right);
    return left.size == (ulk_size)length && memcmp(left.data, right, length) == 0;
}

static int ulk_copy_view(char* destination, size_t capacity, ulk_string_view source)
{
    if (source.size >= (ulk_size)capacity) return 0;
    if (source.size != 0u) memcpy(destination, source.data, (size_t)source.size);
    destination[source.size] = '\0';
    return 1;
}

static int ulk_builder_reserve(ulk_text_builder* builder, size_t extra)
{
    size_t required;
    char* replacement;
    if (extra > SIZE_MAX - builder->size - 1u) return 0;
    required = builder->size + extra + 1u;
    if (required <= builder->capacity) return 1;
    if (builder->capacity == 0u) builder->capacity = 256u;
    while (builder->capacity < required) {
        if (builder->capacity > SIZE_MAX / 2u) {
            builder->capacity = required;
            break;
        }
        builder->capacity *= 2u;
    }
    replacement = (char*)realloc(builder->data, builder->capacity);
    if (replacement == 0) return 0;
    builder->data = replacement;
    return 1;
}

static int ulk_builder_append_bytes(ulk_text_builder* builder, const char* text, size_t size)
{
    if (!ulk_builder_reserve(builder, size)) return 0;
    if (size != 0u) memcpy(builder->data + builder->size, text, size);
    builder->size += size;
    builder->data[builder->size] = '\0';
    return 1;
}

static int ulk_builder_append(ulk_text_builder* builder, const char* text)
{
    return ulk_builder_append_bytes(builder, text, strlen(text));
}

static int ulk_builder_append_integer(ulk_text_builder* builder, int64_t value)
{
    char number[64];
    const int count = snprintf(number, sizeof(number), "%" PRId64, value);
    return count > 0 && (size_t)count < sizeof(number) &&
        ulk_builder_append_bytes(builder, number, (size_t)count);
}

static int ulk_builder_append_unsigned(ulk_text_builder* builder, uint64_t value)
{
    char number[64];
    const int count = snprintf(number, sizeof(number), "%" PRIu64, value);
    return count > 0 && (size_t)count < sizeof(number) &&
        ulk_builder_append_bytes(builder, number, (size_t)count);
}

static void ulk_builder_release(ulk_text_builder* builder)
{
    free(builder->data);
    builder->data = 0;
    builder->size = 0u;
    builder->capacity = 0u;
}

static int ulk_builder_append_hex(ulk_text_builder* builder, const char* text)
{
    static const char digits[] = "0123456789abcdef";
    const unsigned char* cursor = (const unsigned char*)text;
    while (*cursor != 0u) {
        char encoded[2];
        encoded[0] = digits[*cursor >> 4u];
        encoded[1] = digits[*cursor & 15u];
        if (!ulk_builder_append_bytes(builder, encoded, 2u)) return 0;
        ++cursor;
    }
    return 1;
}

static int ulk_builder_append_json_string(ulk_text_builder* builder, const char* text)
{
    const unsigned char* cursor = (const unsigned char*)text;
    if (!ulk_builder_append(builder, "\"")) return 0;
    while (*cursor != 0u) {
        char escaped[7];
        size_t escaped_size = 0u;
        if (*cursor == '"' || *cursor == '\\') {
            escaped[0] = '\\';
            escaped[1] = (char)*cursor;
            escaped_size = 2u;
        } else if (*cursor == '\b') {
            memcpy(escaped, "\\b", 2u); escaped_size = 2u;
        } else if (*cursor == '\f') {
            memcpy(escaped, "\\f", 2u); escaped_size = 2u;
        } else if (*cursor == '\n') {
            memcpy(escaped, "\\n", 2u); escaped_size = 2u;
        } else if (*cursor == '\r') {
            memcpy(escaped, "\\r", 2u); escaped_size = 2u;
        } else if (*cursor == '\t') {
            memcpy(escaped, "\\t", 2u); escaped_size = 2u;
        } else if (*cursor < 0x20u) {
            (void)snprintf(escaped, sizeof(escaped), "\\u%04x", (unsigned int)*cursor);
            escaped_size = 6u;
        } else {
            escaped[0] = (char)*cursor;
            escaped_size = 1u;
        }
        if (!ulk_builder_append_bytes(builder, escaped, escaped_size)) return 0;
        ++cursor;
    }
    return ulk_builder_append(builder, "\"");
}

static uint32_t ulk_crc32(const unsigned char* data, size_t size)
{
    uint32_t crc = UINT32_C(0xffffffff);
    size_t index;
    for (index = 0u; index < size; ++index) {
        unsigned int bit;
        crc ^= data[index];
        for (bit = 0u; bit < 8u; ++bit) {
            const uint32_t mask = (uint32_t)-(int32_t)(crc & 1u);
            crc = (crc >> 1u) ^ (UINT32_C(0xedb88320) & mask);
        }
    }
    return ~crc;
}

static int ulk_hex_digit(unsigned char character)
{
    if (character >= '0' && character <= '9') return (int)(character - '0');
    if (character >= 'a' && character <= 'f') return (int)(character - 'a') + 10;
    return -1;
}

static int ulk_decode_hex(const char* encoded, char* output, size_t capacity)
{
    const size_t encoded_size = strlen(encoded);
    size_t index;
    if ((encoded_size & 1u) != 0u || encoded_size / 2u >= capacity) return 0;
    for (index = 0u; index < encoded_size; index += 2u) {
        const int high = ulk_hex_digit((unsigned char)encoded[index]);
        const int low = ulk_hex_digit((unsigned char)encoded[index + 1u]);
        if (high < 0 || low < 0) return 0;
        output[index / 2u] = (char)((high << 4) | low);
        if (output[index / 2u] == '\0') return 0;
    }
    output[encoded_size / 2u] = '\0';
    return 1;
}

static int ulk_parse_i64(const char* text, int64_t* output)
{
    char* end = 0;
    int64_t value;
    errno = 0;
    value = (int64_t)strtoll(text, &end, 10);
    if (errno != 0 || end == text || *end != '\0') return 0;
    *output = value;
    return 1;
}

static int ulk_parse_int(const char* text, int* output)
{
    int64_t value;
    if (!ulk_parse_i64(text, &value) || value < INT32_MIN || value > INT32_MAX) return 0;
    *output = (int)value;
    return 1;
}

static int ulk_join_path(const char* root, const char* leaf, char** output)
{
    const size_t root_size = strlen(root);
    const size_t leaf_size = strlen(leaf);
    const int separator = root_size != 0u && root[root_size - 1u] != '/' && root[root_size - 1u] != '\\';
    char* result;
    if (root_size > SIZE_MAX - leaf_size - 2u) return 0;
    result = (char*)malloc(root_size + leaf_size + (separator ? 2u : 1u));
    if (result == 0) return 0;
    memcpy(result, root, root_size);
    if (separator) result[root_size] = '/';
    memcpy(result + root_size + (separator ? 1u : 0u), leaf, leaf_size);
    result[root_size + leaf_size + (separator ? 1u : 0u)] = '\0';
    *output = result;
    return 1;
}

#if defined(_WIN32)
static wchar_t* ulk_wide_path(const char* path)
{
    int count;
    wchar_t* result;
    count = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, path, -1, 0, 0);
    if (count <= 0) return 0;
    result = (wchar_t*)malloc((size_t)count * sizeof(wchar_t));
    if (result == 0) return 0;
    if (MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, path, -1, result, count) != count) {
        free(result);
        return 0;
    }
    return result;
}

static int ulk_path_is_reparse(const char* path)
{
    wchar_t* wide = ulk_wide_path(path);
    DWORD attributes;
    if (wide == 0) return -1;
    attributes = GetFileAttributesW(wide);
    free(wide);
    if (attributes == INVALID_FILE_ATTRIBUTES) return 0;
    return (attributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0u;
}

static int ulk_make_directory(const char* path)
{
    wchar_t* wide = ulk_wide_path(path);
    DWORD attributes;
    if (wide == 0) return 0;
    if (CreateDirectoryW(wide, 0) != 0) { free(wide); return 1; }
    if (GetLastError() != ERROR_ALREADY_EXISTS) { free(wide); return 0; }
    attributes = GetFileAttributesW(wide);
    free(wide);
    return attributes != INVALID_FILE_ATTRIBUTES &&
        (attributes & FILE_ATTRIBUTE_DIRECTORY) != 0u &&
        (attributes & FILE_ATTRIBUTE_REPARSE_POINT) == 0u;
}

static FILE* ulk_open_file(const char* path, const wchar_t* mode)
{
    wchar_t* wide = ulk_wide_path(path);
    FILE* result = 0;
    if (wide == 0) return 0;
    if (_wfopen_s(&result, wide, mode) != 0) result = 0;
    free(wide);
    return result;
}

static int ulk_replace_file(const char* source, const char* target)
{
    wchar_t* source_wide = ulk_wide_path(source);
    wchar_t* target_wide = ulk_wide_path(target);
    int result = 0;
    if (source_wide != 0 && target_wide != 0) {
        result = MoveFileExW(source_wide, target_wide,
            MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
    }
    free(source_wide);
    free(target_wide);
    return result;
}

static int ulk_remove_file(const char* path)
{
    wchar_t* wide = ulk_wide_path(path);
    int result = 0;
    if (wide != 0) result = DeleteFileW(wide) != 0;
    free(wide);
    return result;
}
#else
static int ulk_path_is_reparse(const char* path)
{
    struct stat status;
    if (lstat(path, &status) != 0) return 0;
    return S_ISLNK(status.st_mode) ? 1 : 0;
}

static int ulk_make_directory(const char* path)
{
    struct stat status;
    if (mkdir(path, 0700) == 0) return 1;
    return errno == EEXIST && lstat(path, &status) == 0 &&
        S_ISDIR(status.st_mode) && !S_ISLNK(status.st_mode);
}

static FILE* ulk_open_file(const char* path, const char* mode)
{
    if (strcmp(mode, "rb") == 0) {
        int descriptor;
        struct stat status;
#if defined(O_NOFOLLOW)
        descriptor = open(path, O_RDONLY | O_NOFOLLOW);
#else
        descriptor = open(path, O_RDONLY);
#endif
        if (descriptor < 0) return 0;
        if (fstat(descriptor, &status) != 0 || !S_ISREG(status.st_mode)) {
            close(descriptor);
            return 0;
        }
        return fdopen(descriptor, mode);
    }
    return fopen(path, mode);
}

static int ulk_replace_file(const char* source, const char* target)
{
    return rename(source, target) == 0;
}

static int ulk_remove_file(const char* path)
{
    return unlink(path) == 0;
}
#endif

static int ulk_prepare_root(const char* root, char** sessions, ulk_error_v1* error)
{
    if (ulk_path_is_reparse(root) != 0) {
        return ulk_fail(error, ULK_STATUS_ERROR,
            "Session journal root is a link or reparse point", "session_root_no_follow");
    }
    if (!ulk_make_directory(root)) {
        return ulk_fail(error, ULK_STATUS_ERROR,
            "Session journal root could not be created", "session_root_unavailable");
    }
    if (!ulk_join_path(root, "sessions", sessions)) {
        return ulk_fail(error, ULK_STATUS_ERROR,
            "Session journal path allocation failed", "session_path_allocation_failed");
    }
    if (ulk_path_is_reparse(*sessions) != 0 || !ulk_make_directory(*sessions)) {
        free(*sessions);
        *sessions = 0;
        return ulk_fail(error, ULK_STATUS_ERROR,
            "Session record directory is unavailable or unsafe", "session_directory_no_follow");
    }
    return ULK_STATUS_OK;
}

static int ulk_acquire_lock(const char* root, ulk_session_lock* lock, ulk_error_v1* error)
{
    char* path = 0;
    if (!ulk_join_path(root, ".ulk-session.lock", &path)) {
        return ulk_fail(error, ULK_STATUS_ERROR,
            "Session lock path allocation failed", "session_lock_allocation_failed");
    }
#if defined(_WIN32)
    {
        wchar_t* wide = ulk_wide_path(path);
        lock->handle = INVALID_HANDLE_VALUE;
        if (wide != 0) {
            lock->handle = CreateFileW(wide, GENERIC_READ | GENERIC_WRITE, 0, 0,
                OPEN_ALWAYS, FILE_ATTRIBUTE_HIDDEN, 0);
        }
        free(wide);
        free(path);
        if (lock->handle == INVALID_HANDLE_VALUE) {
            return ulk_fail(error, ULK_STATUS_ERROR,
                "Session journal is locked or unavailable", "session_lock_unavailable");
        }
    }
#else
    lock->descriptor = open(path, O_CREAT | O_RDWR, 0600);
    free(path);
    if (lock->descriptor < 0 || flock(lock->descriptor, LOCK_EX) != 0) {
        if (lock->descriptor >= 0) close(lock->descriptor);
        lock->descriptor = -1;
        return ulk_fail(error, ULK_STATUS_ERROR,
            "Session journal is locked or unavailable", "session_lock_unavailable");
    }
#endif
    return ULK_STATUS_OK;
}

static void ulk_release_lock(ulk_session_lock* lock)
{
#if defined(_WIN32)
    if (lock->handle != INVALID_HANDLE_VALUE) CloseHandle(lock->handle);
    lock->handle = INVALID_HANDLE_VALUE;
#else
    if (lock->descriptor >= 0) {
        (void)flock(lock->descriptor, LOCK_UN);
        close(lock->descriptor);
    }
    lock->descriptor = -1;
#endif
}

static int ulk_read_file(const char* path, char** data, size_t* size)
{
    FILE* file;
    long length;
    char* bytes;
    if (ulk_path_is_reparse(path) != 0) return 0;
#if defined(_WIN32)
    file = ulk_open_file(path, L"rb");
#else
    file = ulk_open_file(path, "rb");
#endif
    if (file == 0) return 0;
    if (fseek(file, 0, SEEK_END) != 0) { fclose(file); return 0; }
    length = ftell(file);
    if (length < 0 || (unsigned long)length > ULK_SESSION_MAX_FILE_BYTES ||
        fseek(file, 0, SEEK_SET) != 0) { fclose(file); return 0; }
    bytes = (char*)malloc((size_t)length + 1u);
    if (bytes == 0) { fclose(file); return 0; }
    if (length != 0 && fread(bytes, 1u, (size_t)length, file) != (size_t)length) {
        free(bytes); fclose(file); return 0;
    }
    if (fclose(file) != 0) { free(bytes); return 0; }
    bytes[length] = '\0';
    *data = bytes;
    *size = (size_t)length;
    return 1;
}

static int ulk_write_file_atomic(const char* target, const char* text, size_t size)
{
    char* temporary;
    FILE* file;
    int ok;
    const size_t target_size = strlen(target);
    temporary = (char*)malloc(target_size + 5u);
    if (temporary == 0) return 0;
    memcpy(temporary, target, target_size);
    memcpy(temporary + target_size, ".tmp", 5u);
#if defined(_WIN32)
    file = ulk_open_file(temporary, L"wb");
#else
    file = ulk_open_file(temporary, "wb");
#endif
    if (file == 0) { free(temporary); return 0; }
    ok = (size == 0u || fwrite(text, 1u, size, file) == size) && fflush(file) == 0;
    if (ok) {
#if defined(_WIN32)
        ok = _commit(_fileno(file)) == 0;
#else
        ok = fsync(fileno(file)) == 0;
#endif
    }
    if (fclose(file) != 0) ok = 0;
    if (ok) ok = ulk_replace_file(temporary, target);
    if (!ok) (void)ulk_remove_file(temporary);
    free(temporary);
    return ok;
}

static int ulk_record_from_public(
    const ulk_session_record_v1* source,
    ulk_session_owned_record* target)
{
    memset(target, 0, sizeof(*target));
    if (!ulk_copy_view(target->session_id, sizeof(target->session_id), source->session_id) ||
        !ulk_copy_view(target->operation_id, sizeof(target->operation_id), source->identity.operation_id) ||
        !ulk_copy_view(target->attempt_id, sizeof(target->attempt_id), source->identity.attempt_id) ||
        !ulk_copy_view(target->runnable_reference, sizeof(target->runnable_reference), source->runnable_reference) ||
        !ulk_copy_view(target->process_identity, sizeof(target->process_identity), source->process_identity) ||
        !ulk_copy_view(target->started_at, sizeof(target->started_at), source->started_at) ||
        !ulk_copy_view(target->ended_at, sizeof(target->ended_at), source->ended_at) ||
        !ulk_copy_view(target->recovery_reference, sizeof(target->recovery_reference), source->recovery_reference) ||
        !ulk_copy_view(target->relaunch_reference, sizeof(target->relaunch_reference), source->relaunch_reference)) return 0;
    target->state = source->state;
    target->exit_code_known = source->exit_code_known;
    target->exit_code = source->exit_code;
    if (source->terminal_result != 0) {
        target->outcome = source->terminal_result->outcome;
        target->effects_may_have_occurred = source->terminal_result->effects_may_have_occurred;
        target->recovery_required = source->terminal_result->recovery.required;
        if (!ulk_copy_view(target->recovery_transaction, sizeof(target->recovery_transaction),
                source->terminal_result->recovery.transaction_id) ||
            !ulk_copy_view(target->recovery_inspect_command, sizeof(target->recovery_inspect_command),
                source->terminal_result->recovery.inspect_command)) return 0;
    }
    return 1;
}

static int ulk_record_equal(
    const ulk_session_owned_record* left,
    const ulk_session_owned_record* right)
{
    return left->state == right->state && left->outcome == right->outcome &&
        left->effects_may_have_occurred == right->effects_may_have_occurred &&
        left->recovery_required == right->recovery_required &&
        left->exit_code_known == right->exit_code_known && left->exit_code == right->exit_code &&
        strcmp(left->session_id, right->session_id) == 0 &&
        strcmp(left->operation_id, right->operation_id) == 0 &&
        strcmp(left->attempt_id, right->attempt_id) == 0 &&
        strcmp(left->runnable_reference, right->runnable_reference) == 0 &&
        strcmp(left->process_identity, right->process_identity) == 0 &&
        strcmp(left->started_at, right->started_at) == 0 &&
        strcmp(left->ended_at, right->ended_at) == 0 &&
        strcmp(left->recovery_transaction, right->recovery_transaction) == 0 &&
        strcmp(left->recovery_inspect_command, right->recovery_inspect_command) == 0 &&
        strcmp(left->recovery_reference, right->recovery_reference) == 0 &&
        strcmp(left->relaunch_reference, right->relaunch_reference) == 0;
}

static int ulk_record_transition_valid(
    const ulk_session_owned_record* before,
    const ulk_session_owned_record* after)
{
    return before->state == ULK_SESSION_RUNNING && after->state == ULK_SESSION_TERMINAL &&
        strcmp(before->session_id, after->session_id) == 0 &&
        strcmp(before->operation_id, after->operation_id) == 0 &&
        strcmp(before->attempt_id, after->attempt_id) == 0 &&
        strcmp(before->runnable_reference, after->runnable_reference) == 0 &&
        strcmp(before->process_identity, after->process_identity) == 0 &&
        strcmp(before->started_at, after->started_at) == 0;
}

static int ulk_serialize_record(
    const ulk_session_owned_record* record,
    ulk_text_builder* output)
{
    uint32_t checksum;
    char checksum_text[16];
    const char* string_fields[] = {
        record->session_id, record->operation_id, record->attempt_id,
        record->runnable_reference, record->process_identity
    };
    const char* later_string_fields[] = {
        record->started_at, record->ended_at
    };
    const char* ending_string_fields[] = {
        record->recovery_transaction, record->recovery_inspect_command,
        record->recovery_reference, record->relaunch_reference
    };
    size_t index;
    if (!ulk_builder_append(output, "ULK_SESSION_RECORD_V1|")) return 0;
    for (index = 0u; index < sizeof(string_fields) / sizeof(string_fields[0]); ++index) {
        if (!ulk_builder_append_hex(output, string_fields[index]) || !ulk_builder_append(output, "|")) return 0;
    }
    if (!ulk_builder_append_integer(output, (int64_t)record->state) || !ulk_builder_append(output, "|")) return 0;
    for (index = 0u; index < sizeof(later_string_fields) / sizeof(later_string_fields[0]); ++index) {
        if (!ulk_builder_append_hex(output, later_string_fields[index]) || !ulk_builder_append(output, "|")) return 0;
    }
    if (!ulk_builder_append_integer(output, record->exit_code_known) || !ulk_builder_append(output, "|") ||
        !ulk_builder_append_integer(output, record->exit_code) || !ulk_builder_append(output, "|") ||
        !ulk_builder_append_integer(output, (int64_t)record->outcome) || !ulk_builder_append(output, "|") ||
        !ulk_builder_append_integer(output, record->effects_may_have_occurred) || !ulk_builder_append(output, "|") ||
        !ulk_builder_append_integer(output, record->recovery_required) || !ulk_builder_append(output, "|")) return 0;
    for (index = 0u; index < sizeof(ending_string_fields) / sizeof(ending_string_fields[0]); ++index) {
        if (!ulk_builder_append_hex(output, ending_string_fields[index]) || !ulk_builder_append(output, "|")) return 0;
    }
    checksum = ulk_crc32((const unsigned char*)output->data, output->size);
    (void)snprintf(checksum_text, sizeof(checksum_text), "%08" PRIx32 "\n", checksum);
    return ulk_builder_append(output, checksum_text);
}

static ulk_disk_record_status ulk_parse_record(
    char* text,
    size_t size,
    ulk_session_owned_record* record)
{
    char* fields[19];
    size_t field_count = 1u;
    size_t index;
    char* checksum_separator;
    char* checksum_end;
    unsigned long expected_checksum;
    int state;
    int outcome;
    if (size < 2u || text[size - 1u] != '\n') return ULK_DISK_RECORD_CORRUPT;
    text[size - 1u] = '\0';
    checksum_separator = strrchr(text, '|');
    if (checksum_separator == 0 || strlen(checksum_separator + 1u) != 8u) return ULK_DISK_RECORD_CORRUPT;
    errno = 0;
    expected_checksum = strtoul(checksum_separator + 1u, &checksum_end, 16);
    if (errno != 0 || *checksum_end != '\0' || expected_checksum > UINT32_MAX ||
        ulk_crc32((const unsigned char*)text, (size_t)(checksum_separator - text + 1)) !=
            (uint32_t)expected_checksum) return ULK_DISK_RECORD_CORRUPT;
    *checksum_separator = '\0';
    fields[0] = text;
    for (index = 0u; text[index] != '\0'; ++index) {
        if (text[index] == '|') {
            text[index] = '\0';
            if (field_count >= sizeof(fields) / sizeof(fields[0])) return ULK_DISK_RECORD_CORRUPT;
            fields[field_count++] = text + index + 1u;
        }
    }
    if (strcmp(fields[0], "ULK_SESSION_RECORD_V1") != 0) {
        return strncmp(fields[0], "ULK_SESSION_RECORD_", 19u) == 0
            ? ULK_DISK_RECORD_INCOMPATIBLE
            : ULK_DISK_RECORD_CORRUPT;
    }
    if (field_count != 18u) return ULK_DISK_RECORD_CORRUPT;
    memset(record, 0, sizeof(*record));
    if (!ulk_decode_hex(fields[1], record->session_id, sizeof(record->session_id)) ||
        !ulk_decode_hex(fields[2], record->operation_id, sizeof(record->operation_id)) ||
        !ulk_decode_hex(fields[3], record->attempt_id, sizeof(record->attempt_id)) ||
        !ulk_decode_hex(fields[4], record->runnable_reference, sizeof(record->runnable_reference)) ||
        !ulk_decode_hex(fields[5], record->process_identity, sizeof(record->process_identity)) ||
        !ulk_parse_int(fields[6], &state) ||
        !ulk_decode_hex(fields[7], record->started_at, sizeof(record->started_at)) ||
        !ulk_decode_hex(fields[8], record->ended_at, sizeof(record->ended_at)) ||
        !ulk_parse_int(fields[9], &record->exit_code_known) ||
        !ulk_parse_i64(fields[10], &record->exit_code) ||
        !ulk_parse_int(fields[11], &outcome) ||
        !ulk_parse_int(fields[12], &record->effects_may_have_occurred) ||
        !ulk_parse_int(fields[13], &record->recovery_required) ||
        !ulk_decode_hex(fields[14], record->recovery_transaction, sizeof(record->recovery_transaction)) ||
        !ulk_decode_hex(fields[15], record->recovery_inspect_command, sizeof(record->recovery_inspect_command)) ||
        !ulk_decode_hex(fields[16], record->recovery_reference, sizeof(record->recovery_reference)) ||
        !ulk_decode_hex(fields[17], record->relaunch_reference, sizeof(record->relaunch_reference))) {
        return ULK_DISK_RECORD_CORRUPT;
    }
    record->state = (ulk_session_state_v1)state;
    record->outcome = (ulk_operation_outcome_v1)outcome;
    return ULK_DISK_RECORD_OK;
}

static int ulk_record_to_json(
    const ulk_session_owned_record* record,
    ulk_text_builder* output)
{
    const ulk_string_view outcome_name = record->state == ULK_SESSION_TERMINAL
        ? ulk_operation_outcome_name_v1(record->outcome)
        : ulk_static_view(0);
    if (!ulk_builder_append(output, "{\"schema\":\"ulk.session_record.v1\",\"session_id\":" ) ||
        !ulk_builder_append_json_string(output, record->session_id) ||
        !ulk_builder_append(output, ",\"operation_id\":") ||
        !ulk_builder_append_json_string(output, record->operation_id) ||
        !ulk_builder_append(output, ",\"attempt_id\":") ||
        !ulk_builder_append_json_string(output, record->attempt_id) ||
        !ulk_builder_append(output, ",\"runnable_reference\":") ||
        !ulk_builder_append_json_string(output, record->runnable_reference) ||
        !ulk_builder_append(output, ",\"process_identity\":")) return 0;
    if (record->process_identity[0] == '\0') {
        if (!ulk_builder_append(output, "null")) return 0;
    } else if (!ulk_builder_append_json_string(output, record->process_identity)) return 0;
    if (!ulk_builder_append(output, ",\"state\":\"") ||
        !ulk_builder_append(output, record->state == ULK_SESSION_RUNNING ? "running" : "terminal") ||
        !ulk_builder_append(output, "\",\"started_at\":") ||
        !ulk_builder_append_json_string(output, record->started_at) ||
        !ulk_builder_append(output, ",\"ended_at\":")) return 0;
    if (record->ended_at[0] == '\0') {
        if (!ulk_builder_append(output, "null")) return 0;
    } else if (!ulk_builder_append_json_string(output, record->ended_at)) return 0;
    if (!ulk_builder_append(output, ",\"exit_code\":")) return 0;
    if (!record->exit_code_known) {
        if (!ulk_builder_append(output, "null")) return 0;
    } else if (!ulk_builder_append_integer(output, record->exit_code)) return 0;
    if (!ulk_builder_append(output, ",\"terminal_result\":")) return 0;
    if (record->state == ULK_SESSION_RUNNING) {
        if (!ulk_builder_append(output, "null")) return 0;
    } else {
        if (!ulk_builder_append(output, "{\"schema\":\"ulk.operation_outcome.v1\",\"operation_id\":" ) ||
            !ulk_builder_append_json_string(output, record->operation_id) ||
            !ulk_builder_append(output, ",\"attempt_id\":") ||
            !ulk_builder_append_json_string(output, record->attempt_id) ||
            !ulk_builder_append(output, ",\"outcome\":") ||
            !ulk_builder_append_json_string(output, outcome_name.data) ||
            !ulk_builder_append(output, ",\"effects_may_have_occurred\":") ||
            !ulk_builder_append(output, record->effects_may_have_occurred ? "true" : "false") ||
            !ulk_builder_append(output, ",\"recovery\":{\"required\":") ||
            !ulk_builder_append(output, record->recovery_required ? "true" : "false") ||
            !ulk_builder_append(output, ",\"transaction_id\":") ||
            !ulk_builder_append_json_string(output, record->recovery_transaction) ||
            !ulk_builder_append(output, ",\"inspect_command\":") ||
            !ulk_builder_append_json_string(output, record->recovery_inspect_command) ||
            !ulk_builder_append(output, "}}")) return 0;
    }
    if (!ulk_builder_append(output, ",\"recovery_reference\":")) return 0;
    if (record->recovery_reference[0] == '\0') {
        if (!ulk_builder_append(output, "null")) return 0;
    } else if (!ulk_builder_append_json_string(output, record->recovery_reference)) return 0;
    if (!ulk_builder_append(output, ",\"relaunch_reference\":")) return 0;
    if (record->relaunch_reference[0] == '\0') {
        if (!ulk_builder_append(output, "null")) return 0;
    } else if (!ulk_builder_append_json_string(output, record->relaunch_reference)) return 0;
    return ulk_builder_append(output, "}");
}

static int ulk_append_record(
    ulk_session_record_set* set,
    const ulk_session_owned_record* record)
{
    ulk_session_owned_record* replacement;
    if (set->count == set->capacity) {
        const size_t capacity = set->capacity == 0u ? 16u : set->capacity * 2u;
        if (capacity > ULK_SESSION_MAX_RECORDS) return 0;
        replacement = (ulk_session_owned_record*)realloc(set->values, capacity * sizeof(*replacement));
        if (replacement == 0) return 0;
        set->values = replacement;
        set->capacity = capacity;
    }
    set->values[set->count++] = *record;
    return 1;
}

static int ulk_record_compare_descending(const void* left_value, const void* right_value)
{
    const ulk_session_owned_record* left = (const ulk_session_owned_record*)left_value;
    const ulk_session_owned_record* right = (const ulk_session_owned_record*)right_value;
    const int time_order = strcmp(right->started_at, left->started_at);
    return time_order != 0 ? time_order : strcmp(right->session_id, left->session_id);
}

static int ulk_load_record_path(
    const char* path,
    ulk_session_owned_record* record,
    ulk_disk_record_status* disk_status)
{
    char* text = 0;
    size_t size = 0u;
    if (!ulk_read_file(path, &text, &size)) {
        *disk_status = ULK_DISK_RECORD_CORRUPT;
        return 0;
    }
    *disk_status = ulk_parse_record(text, size, record);
    free(text);
    return *disk_status == ULK_DISK_RECORD_OK;
}

static int ulk_has_session_suffix(const char* name)
{
    const size_t size = strlen(name);
    return size > 8u && strcmp(name + size - 8u, ".session") == 0;
}

static int ulk_scan_records(
    const char* sessions,
    ulk_session_record_set* set,
    ulk_session_lookup_status_v1* failure_status)
{
#if defined(_WIN32)
    char* pattern = 0;
    wchar_t* wide_pattern;
    WIN32_FIND_DATAW entry;
    HANDLE search;
    if (!ulk_join_path(sessions, "*.session", &pattern)) return 0;
    wide_pattern = ulk_wide_path(pattern);
    free(pattern);
    if (wide_pattern == 0) return 0;
    search = FindFirstFileW(wide_pattern, &entry);
    free(wide_pattern);
    if (search == INVALID_HANDLE_VALUE) {
        return GetLastError() == ERROR_FILE_NOT_FOUND;
    }
    do {
        char name[300];
        char* path = 0;
        ulk_session_owned_record record;
        ulk_disk_record_status disk_status;
        if ((entry.dwFileAttributes & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_POINT)) != 0u) continue;
        if (WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, entry.cFileName, -1,
                name, (int)sizeof(name), 0, 0) <= 0 || !ulk_has_session_suffix(name) ||
            !ulk_join_path(sessions, name, &path)) {
            FindClose(search); return 0;
        }
        if (!ulk_load_record_path(path, &record, &disk_status)) {
            free(path); FindClose(search);
            *failure_status = disk_status == ULK_DISK_RECORD_INCOMPATIBLE
                ? ULK_SESSION_LOOKUP_INCOMPATIBLE : ULK_SESSION_LOOKUP_CORRUPT;
            return 0;
        }
        free(path);
        if (!ulk_append_record(set, &record)) { FindClose(search); return 0; }
    } while (FindNextFileW(search, &entry) != 0);
    FindClose(search);
#else
    DIR* directory = opendir(sessions);
    struct dirent* entry;
    if (directory == 0) return 0;
    while ((entry = readdir(directory)) != 0) {
        char* path = 0;
        ulk_session_owned_record record;
        ulk_disk_record_status disk_status;
        if (!ulk_has_session_suffix(entry->d_name)) continue;
        if (!ulk_join_path(sessions, entry->d_name, &path)) { closedir(directory); return 0; }
        if (!ulk_load_record_path(path, &record, &disk_status)) {
            free(path); closedir(directory);
            *failure_status = disk_status == ULK_DISK_RECORD_INCOMPATIBLE
                ? ULK_SESSION_LOOKUP_INCOMPATIBLE : ULK_SESSION_LOOKUP_CORRUPT;
            return 0;
        }
        free(path);
        if (!ulk_append_record(set, &record)) { closedir(directory); return 0; }
    }
    closedir(directory);
#endif
    qsort(set->values, set->count, sizeof(*set->values), ulk_record_compare_descending);
    return 1;
}

static int ulk_copy_json(
    const ulk_text_builder* output,
    char* json,
    ulk_size json_capacity,
    ulk_size* required_capacity,
    ulk_error_v1* error)
{
    const ulk_size required = (ulk_size)output->size + 1u;
    if (required_capacity == 0) {
        return ulk_fail(error, ULK_STATUS_INVALID_ARGUMENT,
            "required_capacity is required", "session_output_contract_invalid");
    }
    *required_capacity = required;
    if (json == 0 && json_capacity == 0u) return ULK_STATUS_OK;
    if (json == 0 || json_capacity < required) {
        return ulk_fail(error, ULK_STATUS_ERROR,
            "Session JSON output buffer is too small", "session_output_buffer_too_small");
    }
    memcpy(json, output->data, output->size + 1u);
    return ULK_STATUS_OK;
}

static int ulk_validate_owned_record(const ulk_session_owned_record* record)
{
    ulk_session_record_v1 public_record;
    ulk_operation_result_v1 terminal;
    memset(&public_record, 0, sizeof(public_record));
    memset(&terminal, 0, sizeof(terminal));
    public_record.struct_size = sizeof(public_record);
    public_record.session_id = ulk_static_view(record->session_id);
    public_record.identity.struct_size = sizeof(public_record.identity);
    public_record.identity.operation_id = ulk_static_view(record->operation_id);
    public_record.identity.attempt_id = ulk_static_view(record->attempt_id);
    public_record.runnable_reference = ulk_static_view(record->runnable_reference);
    public_record.process_identity = ulk_static_view(record->process_identity);
    public_record.state = record->state;
    public_record.started_at = ulk_static_view(record->started_at);
    public_record.ended_at = ulk_static_view(record->ended_at);
    public_record.exit_code_known = record->exit_code_known;
    public_record.exit_code = record->exit_code;
    public_record.recovery_reference = ulk_static_view(record->recovery_reference);
    public_record.relaunch_reference = ulk_static_view(record->relaunch_reference);
    if (record->state == ULK_SESSION_TERMINAL) {
        terminal.struct_size = sizeof(terminal);
        terminal.identity = public_record.identity;
        terminal.outcome = record->outcome;
        terminal.effects_may_have_occurred = record->effects_may_have_occurred;
        terminal.recovery.struct_size = sizeof(terminal.recovery);
        terminal.recovery.required = record->recovery_required;
        terminal.recovery.transaction_id = ulk_static_view(record->recovery_transaction);
        terminal.recovery.inspect_command = ulk_static_view(record->recovery_inspect_command);
        public_record.terminal_result = &terminal;
    }
    return ulk_session_record_validate_v1(&public_record) == ULK_STATUS_OK;
}

int ULK_CALL ulk_session_journal_validate_v1(const ulk_session_journal_v1* journal)
{
    return journal != 0 && journal->struct_size >= (ulk_size)sizeof(*journal) &&
        ulk_view_valid(journal->root, ULK_SESSION_MAX_ROOT_BYTES, 0) &&
        journal->maximum_records <= ULK_SESSION_MAX_RECORDS
        ? ULK_STATUS_OK : ULK_STATUS_INVALID_ARGUMENT;
}

int ULK_CALL ulk_session_record_validate_v1(const ulk_session_record_v1* record)
{
    if (record == 0 || record->struct_size < (ulk_size)sizeof(*record) ||
        !ulk_identifier_valid(record->session_id) ||
        ulk_operation_identity_validate_v1(&record->identity) != ULK_STATUS_OK ||
        !ulk_view_valid(record->runnable_reference, ULK_SESSION_MAX_RUNNABLE_BYTES, 0) ||
        !ulk_view_valid(record->process_identity, ULK_SESSION_MAX_PROCESS_BYTES, 1) ||
        !ulk_view_valid(record->started_at, ULK_SESSION_MAX_TIME_BYTES, 0) ||
        !ulk_view_valid(record->ended_at, ULK_SESSION_MAX_TIME_BYTES, 1) ||
        !ulk_view_valid(record->recovery_reference, ULK_SESSION_MAX_REFERENCE_BYTES, 1) ||
        !ulk_view_valid(record->relaunch_reference, ULK_SESSION_MAX_REFERENCE_BYTES, 1) ||
        (record->exit_code_known != 0 && record->exit_code_known != 1)) {
        return ULK_STATUS_INVALID_ARGUMENT;
    }
    if (record->state == ULK_SESSION_RUNNING) {
        if (record->terminal_result != 0 || record->ended_at.size != 0u ||
            record->exit_code_known != 0 || record->recovery_reference.size != 0u ||
            record->relaunch_reference.size != 0u) {
            return ULK_STATUS_INVALID_ARGUMENT;
        }
        return ULK_STATUS_OK;
    }
    if (record->state != ULK_SESSION_TERMINAL || record->terminal_result == 0 ||
        record->ended_at.size == 0u ||
        ulk_operation_result_validate_v1(record->terminal_result) != ULK_STATUS_OK ||
        !ulk_views_equal(record->identity.operation_id,
            record->terminal_result->identity.operation_id) ||
        !ulk_views_equal(record->identity.attempt_id,
            record->terminal_result->identity.attempt_id)) {
        return ULK_STATUS_INVALID_ARGUMENT;
    }
    if (record->terminal_result->recovery.required &&
        record->recovery_reference.size == 0u) return ULK_STATUS_INVALID_ARGUMENT;
    return ULK_STATUS_OK;
}

int ULK_CALL ulk_session_journal_write_v1(
    const ulk_session_journal_v1* journal,
    const ulk_session_record_v1* record,
    ulk_error_v1* error)
{
    char root[ULK_SESSION_MAX_ROOT_BYTES + 1u];
    char* sessions = 0;
    char file_name[160];
    char* target = 0;
    ulk_session_lock lock;
    ulk_session_owned_record requested;
    ulk_session_record_set set;
    ulk_session_lookup_status_v1 failure_status = ULK_SESSION_LOOKUP_CORRUPT;
    ulk_text_builder serialized;
    size_t index;
    size_t maximum;
    int result = ULK_STATUS_ERROR;
    memset(&set, 0, sizeof(set));
    memset(&serialized, 0, sizeof(serialized));
    if (ulk_session_journal_validate_v1(journal) != ULK_STATUS_OK ||
        ulk_session_record_validate_v1(record) != ULK_STATUS_OK ||
        !ulk_copy_view(root, sizeof(root), journal->root) ||
        !ulk_record_from_public(record, &requested)) {
        return ulk_fail(error, ULK_STATUS_INVALID_ARGUMENT,
            "Session journal write input is invalid", "session_write_invalid");
    }
    if (ulk_prepare_root(root, &sessions, error) != ULK_STATUS_OK) return ULK_STATUS_ERROR;
    if (ulk_acquire_lock(root, &lock, error) != ULK_STATUS_OK) { free(sessions); return ULK_STATUS_ERROR; }
    if (!ulk_scan_records(sessions, &set, &failure_status)) {
        ulk_fail(error, ULK_STATUS_ERROR,
            failure_status == ULK_SESSION_LOOKUP_INCOMPATIBLE
                ? "Session journal contains a future record schema"
                : "Session journal contains a corrupt record",
            failure_status == ULK_SESSION_LOOKUP_INCOMPATIBLE
                ? "session_record_incompatible" : "session_record_corrupt");
        goto cleanup;
    }
    for (index = 0u; index < set.count; ++index) {
        const ulk_session_owned_record* existing = &set.values[index];
        if (strcmp(existing->operation_id, requested.operation_id) == 0 &&
            strcmp(existing->attempt_id, requested.attempt_id) == 0 &&
            strcmp(existing->session_id, requested.session_id) != 0) {
            ulk_fail(error, ULK_STATUS_ERROR,
                "Operation and attempt identity already belongs to another session",
                "session_operation_identity_conflict");
            goto cleanup;
        }
        if (strcmp(existing->session_id, requested.session_id) == 0) {
            if (ulk_record_equal(existing, &requested)) { result = ULK_STATUS_OK; goto cleanup; }
            if (!ulk_record_transition_valid(existing, &requested)) {
                ulk_fail(error, ULK_STATUS_ERROR,
                    "Session identity already has different content",
                    existing->state == ULK_SESSION_TERMINAL
                        ? "session_terminal_immutable" : "session_idempotency_conflict");
                goto cleanup;
            }
        }
    }
    if (!ulk_serialize_record(&requested, &serialized)) {
        ulk_fail(error, ULK_STATUS_ERROR,
            "Session record serialization failed", "session_serialization_failed");
        goto cleanup;
    }
    if (snprintf(file_name, sizeof(file_name), "%s.session", requested.session_id) <= 0 ||
        !ulk_join_path(sessions, file_name, &target) ||
        !ulk_write_file_atomic(target, serialized.data, serialized.size)) {
        ulk_fail(error, ULK_STATUS_ERROR,
            "Session record could not be committed atomically", "session_atomic_write_failed");
        goto cleanup;
    }
    maximum = journal->maximum_records == 0u
        ? ULK_SESSION_DEFAULT_RECORDS : (size_t)journal->maximum_records;
    free(set.values); memset(&set, 0, sizeof(set));
    if (!ulk_scan_records(sessions, &set, &failure_status)) {
        ulk_fail(error, ULK_STATUS_ERROR,
            "Committed session journal could not be re-read", "session_commit_verification_failed");
        goto cleanup;
    }
    for (index = maximum; index < set.count; ++index) {
        if (snprintf(file_name, sizeof(file_name), "%s.session", set.values[index].session_id) <= 0) {
            ulk_fail(error, ULK_STATUS_ERROR,
                "Session retention path could not be formed", "session_retention_failed");
            goto cleanup;
        }
        free(target); target = 0;
        if (!ulk_join_path(sessions, file_name, &target) || !ulk_remove_file(target)) {
            ulk_fail(error, ULK_STATUS_ERROR,
                "Session retention could not remove the oldest record", "session_retention_failed");
            goto cleanup;
        }
    }
    result = ULK_STATUS_OK;
cleanup:
    if (result == ULK_STATUS_OK) ulk_clear_error(error);
    free(target);
    free(set.values);
    ulk_builder_release(&serialized);
    ulk_release_lock(&lock);
    free(sessions);
    return result;
}

static int ulk_lookup_common(
    const ulk_session_journal_v1* journal,
    ulk_string_view key,
    int by_runnable,
    ulk_session_lookup_status_v1* lookup_status,
    char* json,
    ulk_size json_capacity,
    ulk_size* required_capacity,
    ulk_error_v1* error)
{
    char root[ULK_SESSION_MAX_ROOT_BYTES + 1u];
    char key_text[ULK_SESSION_MAX_RUNNABLE_BYTES + 1u];
    char* sessions = 0;
    ulk_session_lock lock;
    ulk_session_record_set set;
    ulk_session_lookup_status_v1 failure_status = ULK_SESSION_LOOKUP_CORRUPT;
    ulk_text_builder output;
    size_t index;
    int result = ULK_STATUS_ERROR;
    memset(&set, 0, sizeof(set));
    memset(&output, 0, sizeof(output));
    if (lookup_status == 0 || required_capacity == 0 ||
        ulk_session_journal_validate_v1(journal) != ULK_STATUS_OK ||
        !(by_runnable ? ulk_view_valid(key, ULK_SESSION_MAX_RUNNABLE_BYTES, 0)
                      : ulk_identifier_valid(key)) ||
        !ulk_copy_view(root, sizeof(root), journal->root) ||
        !ulk_copy_view(key_text, sizeof(key_text), key)) {
        return ulk_fail(error, ULK_STATUS_INVALID_ARGUMENT,
            "Session lookup input is invalid", "session_lookup_invalid");
    }
    *required_capacity = 0u;
    *lookup_status = ULK_SESSION_LOOKUP_NOT_FOUND;
    if (ulk_prepare_root(root, &sessions, error) != ULK_STATUS_OK) return ULK_STATUS_ERROR;
    if (ulk_acquire_lock(root, &lock, error) != ULK_STATUS_OK) { free(sessions); return ULK_STATUS_ERROR; }
    if (!ulk_scan_records(sessions, &set, &failure_status)) {
        *lookup_status = failure_status;
        ulk_fail(error, ULK_STATUS_ERROR,
            failure_status == ULK_SESSION_LOOKUP_INCOMPATIBLE
                ? "Session journal contains a future record schema"
                : "Session journal contains a corrupt record",
            failure_status == ULK_SESSION_LOOKUP_INCOMPATIBLE
                ? "session_record_incompatible" : "session_record_corrupt");
        goto cleanup;
    }
    for (index = 0u; index < set.count; ++index) {
        const char* candidate = by_runnable
            ? set.values[index].runnable_reference : set.values[index].session_id;
        if (strcmp(candidate, key_text) != 0) continue;
        if (!ulk_validate_owned_record(&set.values[index]) ||
            !ulk_record_to_json(&set.values[index], &output)) {
            *lookup_status = ULK_SESSION_LOOKUP_CORRUPT;
            ulk_fail(error, ULK_STATUS_ERROR,
                "Session record could not be rendered", "session_record_corrupt");
            goto cleanup;
        }
        *lookup_status = ULK_SESSION_LOOKUP_FOUND;
        result = ulk_copy_json(&output, json, json_capacity, required_capacity, error);
        goto cleanup;
    }
    result = ULK_STATUS_OK;
cleanup:
    if (result == ULK_STATUS_OK) ulk_clear_error(error);
    ulk_builder_release(&output);
    free(set.values);
    ulk_release_lock(&lock);
    free(sessions);
    return result;
}

int ULK_CALL ulk_session_journal_inspect_v1(
    const ulk_session_journal_v1* journal,
    ulk_string_view session_id,
    ulk_session_lookup_status_v1* lookup_status,
    char* json,
    ulk_size json_capacity,
    ulk_size* required_capacity,
    ulk_error_v1* error)
{
    return ulk_lookup_common(journal, session_id, 0, lookup_status,
        json, json_capacity, required_capacity, error);
}

int ULK_CALL ulk_session_journal_last_run_v1(
    const ulk_session_journal_v1* journal,
    ulk_string_view runnable_reference,
    ulk_session_lookup_status_v1* lookup_status,
    char* json,
    ulk_size json_capacity,
    ulk_size* required_capacity,
    ulk_error_v1* error)
{
    return ulk_lookup_common(journal, runnable_reference, 1, lookup_status,
        json, json_capacity, required_capacity, error);
}

int ULK_CALL ulk_session_journal_list_v1(
    const ulk_session_journal_v1* journal,
    ulk_size limit,
    char* json,
    ulk_size json_capacity,
    ulk_size* required_capacity,
    ulk_error_v1* error)
{
    char root[ULK_SESSION_MAX_ROOT_BYTES + 1u];
    char* sessions = 0;
    ulk_session_lock lock;
    ulk_session_record_set set;
    ulk_session_lookup_status_v1 failure_status = ULK_SESSION_LOOKUP_CORRUPT;
    ulk_text_builder output;
    size_t index;
    size_t bounded_limit;
    int result = ULK_STATUS_ERROR;
    memset(&set, 0, sizeof(set));
    memset(&output, 0, sizeof(output));
    if (required_capacity == 0 || ulk_session_journal_validate_v1(journal) != ULK_STATUS_OK ||
        limit > ULK_SESSION_MAX_RECORDS || !ulk_copy_view(root, sizeof(root), journal->root)) {
        return ulk_fail(error, ULK_STATUS_INVALID_ARGUMENT,
            "Session list input is invalid", "session_list_invalid");
    }
    if (ulk_prepare_root(root, &sessions, error) != ULK_STATUS_OK) return ULK_STATUS_ERROR;
    if (ulk_acquire_lock(root, &lock, error) != ULK_STATUS_OK) { free(sessions); return ULK_STATUS_ERROR; }
    if (!ulk_scan_records(sessions, &set, &failure_status)) {
        ulk_fail(error, ULK_STATUS_ERROR,
            failure_status == ULK_SESSION_LOOKUP_INCOMPATIBLE
                ? "Session journal contains a future record schema"
                : "Session journal contains a corrupt record",
            failure_status == ULK_SESSION_LOOKUP_INCOMPATIBLE
                ? "session_record_incompatible" : "session_record_corrupt");
        goto cleanup;
    }
    bounded_limit = limit == 0u ? set.count : (size_t)limit;
    if (bounded_limit > set.count) bounded_limit = set.count;
    if (!ulk_builder_append(&output, "{\"schema\":\"ulk.session_list.v1\",\"sessions\":[")) goto allocation_failure;
    for (index = 0u; index < bounded_limit; ++index) {
        if (index != 0u && !ulk_builder_append(&output, ",")) goto allocation_failure;
        if (!ulk_validate_owned_record(&set.values[index]) ||
            !ulk_record_to_json(&set.values[index], &output)) goto allocation_failure;
    }
    if (!ulk_builder_append(&output, "]}")) goto allocation_failure;
    result = ulk_copy_json(&output, json, json_capacity, required_capacity, error);
    goto cleanup;
allocation_failure:
    ulk_fail(error, ULK_STATUS_ERROR,
        "Session list could not be rendered", "session_list_render_failed");
cleanup:
    if (result == ULK_STATUS_OK) ulk_clear_error(error);
    ulk_builder_release(&output);
    free(set.values);
    ulk_release_lock(&lock);
    free(sessions);
    return result;
}
