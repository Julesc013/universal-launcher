#include "ulk/ulk_api.h"

#include <stdlib.h>
#include <string.h>

#define ULK_MAX_DESCRIPTOR_TEXT 4096
#define ULK_INITIAL_REGISTRY_CAPACITY 8
#define ULK_REGISTRY_STORAGE_BUDGET_BYTES (64u * 1024u)

typedef struct ulk_command_metadata {
    const char* command_name;
    const char* effects_json;
    const char* request_schema;
    const char* response_schema;
    const char* result_schema;
    const char* refusal_schema;
    const char* dry_run_behavior;
    const char* availability;
    const char* owner;
    const char* binding;
    const char* handler_status;
    int builtin_kind;
} ulk_command_metadata;

typedef struct ulk_registered_command {
    char* command_name;
    ulk_size command_name_size;
    char* effects_json;
    char* request_schema;
    char* response_schema;
    char* result_schema;
    char* refusal_schema;
    char* dry_run_behavior;
    char* availability;
    char* owner;
    char* binding;
    ulk_command_handler_v1 handler;
    void* user;
} ulk_registered_command;

struct ulk_context {
    ulk_allocator_v1 allocator;
    int has_allocator;
    ulk_size registered_count;
    ulk_size registered_capacity;
    ulk_registered_command* registered;
    char* command_graph_json;
};

typedef struct ulk_static_response {
    int status;
    const char* payload;
    const char* error_message;
} ulk_static_response;

typedef struct ulk_json_buffer {
    char* data;
    ulk_size length;
    ulk_size capacity;
} ulk_json_buffer;

enum {
    ULK_BUILTIN_NONE = 0,
    ULK_BUILTIN_COMMAND_GRAPH_INSPECT,
    ULK_BUILTIN_PRODUCT_INSPECT,
    ULK_BUILTIN_INSTALL_REFS_LIST,
    ULK_BUILTIN_INSTALL_REFS_INSPECT,
    ULK_BUILTIN_INSTANCE_CREATE,
    ULK_BUILTIN_INSTANCE_LIST,
    ULK_BUILTIN_PROFILES_LIST,
    ULK_BUILTIN_ACCOUNT_REFS_LIST,
    ULK_BUILTIN_ARTIFACT_SETS_LIST,
    ULK_BUILTIN_LAUNCH_PLAN_BUILD,
    ULK_BUILTIN_DIAGNOSTICS_RUN
};

static const ulk_command_metadata ULK_BUILTIN_COMMANDS[] = {
    {
        "command_graph.inspect", "[\"none\"]", "ulk.command_request.v1",
        "ulk.command_response.v1", "ulk.result.v1", "ulk.refusal.v1",
        "read_only", "available", "universal-launcher", "builtin", "builtin",
        ULK_BUILTIN_COMMAND_GRAPH_INSPECT
    },
    {
        "product.inspect", "[\"none\"]", "ulk.command_request.v1",
        "ulk.command_response.v1", "ulk.result.v1", "ulk.refusal.v1",
        "read_only", "available", "universal-launcher", "builtin", "builtin",
        ULK_BUILTIN_PRODUCT_INSPECT
    },
    {
        "install_refs.list", "[\"workspace_read\"]", "ulk.command_request.v1",
        "ulk.command_response.v1", "ulk.result.v1", "ulk.refusal.v1",
        "read_only", "available", "universal-launcher", "builtin", "builtin",
        ULK_BUILTIN_INSTALL_REFS_LIST
    },
    {
        "install_refs.inspect", "[\"workspace_read\"]", "ulk.command_request.v1",
        "ulk.command_response.v1", "ulk.result.v1", "ulk.refusal.v1",
        "read_only", "available", "universal-launcher", "builtin", "builtin",
        ULK_BUILTIN_INSTALL_REFS_INSPECT
    },
    {
        "instance.create", "[\"workspace_write\"]", "ulk.command_request.v1",
        "ulk.command_response.v1", "ulk.result.v1", "ulk.refusal.v1",
        "preview_or_refusal", "available", "universal-launcher", "builtin", "builtin",
        ULK_BUILTIN_INSTANCE_CREATE
    },
    {
        "instance.list", "[\"workspace_read\"]", "ulk.command_request.v1",
        "ulk.command_response.v1", "ulk.result.v1", "ulk.refusal.v1",
        "read_only", "available", "universal-launcher", "builtin", "builtin",
        ULK_BUILTIN_INSTANCE_LIST
    },
    {
        "profiles.list", "[\"workspace_read\"]", "ulk.command_request.v1",
        "ulk.command_response.v1", "ulk.result.v1", "ulk.refusal.v1",
        "read_only", "available", "universal-launcher", "builtin", "builtin",
        ULK_BUILTIN_PROFILES_LIST
    },
    {
        "account_refs.list", "[\"workspace_read\"]", "ulk.command_request.v1",
        "ulk.command_response.v1", "ulk.result.v1", "ulk.refusal.v1",
        "read_only", "available", "universal-launcher", "builtin", "builtin",
        ULK_BUILTIN_ACCOUNT_REFS_LIST
    },
    {
        "artifact_sets.list", "[\"workspace_read\"]", "ulk.command_request.v1",
        "ulk.command_response.v1", "ulk.result.v1", "ulk.refusal.v1",
        "read_only", "available", "universal-launcher", "builtin", "builtin",
        ULK_BUILTIN_ARTIFACT_SETS_LIST
    },
    {
        "launch_plan.build", "[\"workspace_read\"]", "ulk.command_request.v1",
        "ulk.command_response.v1", "ulk.result.v1", "ulk.refusal.v1",
        "preview_only", "available", "universal-launcher", "builtin", "builtin",
        ULK_BUILTIN_LAUNCH_PLAN_BUILD
    },
    {
        "diagnostics.run", "[\"workspace_read\"]", "ulk.command_request.v1",
        "ulk.command_response.v1", "ulk.result.v1", "ulk.refusal.v1",
        "read_only", "available", "universal-launcher", "builtin", "builtin",
        ULK_BUILTIN_DIAGNOSTICS_RUN
    }
};

#define ULK_BUILTIN_COMMAND_COUNT \
    ((ulk_size)(sizeof(ULK_BUILTIN_COMMANDS) / sizeof(ULK_BUILTIN_COMMANDS[0])))

static void* ulk_default_alloc(void* user, ulk_size size)
{
    (void)user;
    return malloc((size_t)size);
}

static void ulk_default_free(void* user, void* ptr)
{
    (void)user;
    free(ptr);
}

static ulk_string_view ulk_view_from_cstr(const char* text)
{
    ulk_string_view view;
    view.data = text;
    view.size = text == 0 ? 0 : (ulk_size)strlen(text);
    return view;
}

static int ulk_string_equals(ulk_string_view value, const char* expected)
{
    ulk_size expected_size;

    if (value.data == 0 || expected == 0) {
        return 0;
    }

    expected_size = (ulk_size)strlen(expected);
    return value.size == expected_size &&
        memcmp(value.data, expected, (size_t)expected_size) == 0;
}

static int ulk_view_is_text(ulk_string_view value)
{
    ulk_size index;
    if (value.data == 0 || value.size == 0 || value.size > ULK_MAX_DESCRIPTOR_TEXT) {
        return 0;
    }
    for (index = 0; index < value.size; ++index) {
        if (value.data[index] == '\0') {
            return 0;
        }
    }
    return 1;
}

static int ulk_is_canonical_command_id(ulk_string_view value)
{
    ulk_size index;
    int has_dot = 0;
    int has_segment_character = 0;

    if (!ulk_view_is_text(value) ||
        ulk_string_equals(value, "instances.list") ||
        ulk_string_equals(value, "diagnostics.report")) {
        return 0;
    }

    for (index = 0; index < value.size; ++index) {
        char current = value.data[index];
        if (current == '.') {
            if (!has_segment_character) {
                return 0;
            }
            has_dot = 1;
            has_segment_character = 0;
        } else if ((current >= 'a' && current <= 'z') ||
                   (current >= '0' && current <= '9') ||
                   current == '_') {
            has_segment_character = 1;
        } else {
            return 0;
        }
    }

    return has_dot && has_segment_character;
}

static void ulk_skip_json_space(ulk_string_view value, ulk_size* index)
{
    while (*index < value.size) {
        char current = value.data[*index];
        if (current != ' ' && current != '\t' && current != '\r' && current != '\n') {
            break;
        }
        ++(*index);
    }
}

static int ulk_effects_json_is_valid(ulk_string_view value)
{
    ulk_size index = 0;
    int effect_count = 0;

    if (!ulk_view_is_text(value)) {
        return 0;
    }
    ulk_skip_json_space(value, &index);
    if (index >= value.size || value.data[index++] != '[') {
        return 0;
    }

    for (;;) {
        int token_size = 0;
        ulk_skip_json_space(value, &index);
        if (index >= value.size || value.data[index++] != '"') {
            return 0;
        }
        while (index < value.size && value.data[index] != '"') {
            char current = value.data[index++];
            if (!((current >= 'a' && current <= 'z') ||
                  (current >= '0' && current <= '9') ||
                  current == '_' || current == '.' || current == '-')) {
                return 0;
            }
            ++token_size;
        }
        if (token_size == 0 || index >= value.size || value.data[index++] != '"') {
            return 0;
        }
        ++effect_count;
        ulk_skip_json_space(value, &index);
        if (index >= value.size) {
            return 0;
        }
        if (value.data[index] == ']') {
            ++index;
            break;
        }
        if (value.data[index++] != ',') {
            return 0;
        }
    }

    ulk_skip_json_space(value, &index);
    return effect_count > 0 && index == value.size;
}

static void ulk_set_response(
    ulk_command_response_v1* response,
    int status,
    const char* payload,
    const char* error_message
)
{
    response->status = 0;
    response->json_payload.data = 0;
    response->json_payload.size = 0;
    memset(&response->error, 0, sizeof(response->error));
    response->struct_size = sizeof(*response);
    response->status = status;

    if (payload != 0) {
        response->json_payload.data = payload;
        response->json_payload.size = (ulk_size)strlen(payload);
    }

    response->error.struct_size = sizeof(response->error);
    response->error.code = status;
    if (error_message != 0) {
        response->error.message.data = error_message;
        response->error.message.size = (ulk_size)strlen(error_message);
    }
}

static ulk_registered_command* ulk_find_registered(
    ulk_context* context,
    ulk_string_view command_name
)
{
    ulk_size index;
    if (context == 0) {
        return 0;
    }
    for (index = 0; index < context->registered_count; ++index) {
        ulk_registered_command* entry = &context->registered[index];
        if (entry->command_name_size == command_name.size &&
            command_name.data != 0 &&
            memcmp(entry->command_name, command_name.data, (size_t)command_name.size) == 0) {
            return entry;
        }
    }
    return 0;
}

static const ulk_command_metadata* ulk_find_builtin(ulk_string_view command_name)
{
    ulk_size index;
    for (index = 0; index < ULK_BUILTIN_COMMAND_COUNT; ++index) {
        if (ulk_string_equals(command_name, ULK_BUILTIN_COMMANDS[index].command_name)) {
            return &ULK_BUILTIN_COMMANDS[index];
        }
    }
    return 0;
}

static void ulk_invalidate_graph(ulk_context* context)
{
    if (context->command_graph_json != 0) {
        context->allocator.free(context->allocator.user, context->command_graph_json);
        context->command_graph_json = 0;
    }
}

static char* ulk_copy_view(ulk_context* context, ulk_string_view value)
{
    char* copy;
    if (!ulk_view_is_text(value)) {
        return 0;
    }
    copy = (char*)context->allocator.alloc(context->allocator.user, value.size + 1);
    if (copy == 0) {
        return 0;
    }
    memcpy(copy, value.data, (size_t)value.size);
    copy[value.size] = '\0';
    return copy;
}

static void ulk_clear_registered(ulk_context* context, ulk_registered_command* entry)
{
    char** fields[] = {
        &entry->command_name,
        &entry->effects_json,
        &entry->request_schema,
        &entry->response_schema,
        &entry->result_schema,
        &entry->refusal_schema,
        &entry->dry_run_behavior,
        &entry->availability,
        &entry->owner,
        &entry->binding
    };
    ulk_size index;

    for (index = 0; index < (ulk_size)(sizeof(fields) / sizeof(fields[0])); ++index) {
        if (*fields[index] != 0) {
            context->allocator.free(context->allocator.user, *fields[index]);
        }
    }
    memset(entry, 0, sizeof(*entry));
}

static ulk_size ulk_registry_capacity_limit(void)
{
    return (ulk_size)(ULK_REGISTRY_STORAGE_BUDGET_BYTES / sizeof(ulk_registered_command));
}

static int ulk_ensure_registry_capacity(ulk_context* context, ulk_size required_count)
{
    ulk_size capacity_limit;
    ulk_size new_capacity;
    ulk_size allocation_size;
    ulk_registered_command* replacement;

    if (context == 0) {
        return ULK_STATUS_INVALID_ARGUMENT;
    }
    if (required_count <= context->registered_capacity) {
        return ULK_STATUS_OK;
    }

    capacity_limit = ulk_registry_capacity_limit();
    if (required_count > capacity_limit) {
        return ULK_STATUS_INVALID_ARGUMENT;
    }

    new_capacity = context->registered_capacity == 0
        ? (ulk_size)ULK_INITIAL_REGISTRY_CAPACITY
        : context->registered_capacity;
    while (new_capacity < required_count) {
        if (new_capacity > capacity_limit / 2) {
            new_capacity = capacity_limit;
            break;
        }
        new_capacity *= 2;
    }
    if (new_capacity < required_count ||
        new_capacity > capacity_limit ||
        new_capacity > ((ulk_size)-1) / (ulk_size)sizeof(ulk_registered_command)) {
        return ULK_STATUS_INVALID_ARGUMENT;
    }

    allocation_size = new_capacity * (ulk_size)sizeof(ulk_registered_command);
    replacement = (ulk_registered_command*)context->allocator.alloc(
        context->allocator.user,
        allocation_size);
    if (replacement == 0) {
        return ULK_STATUS_ERROR;
    }
    memset(replacement, 0, (size_t)allocation_size);
    if (context->registered_count != 0) {
        memcpy(
            replacement,
            context->registered,
            (size_t)(context->registered_count * (ulk_size)sizeof(ulk_registered_command)));
    }
    if (context->registered != 0) {
        context->allocator.free(context->allocator.user, context->registered);
    }
    context->registered = replacement;
    context->registered_capacity = new_capacity;
    return ULK_STATUS_OK;
}

static int ulk_register_internal(
    ulk_context* context,
    ulk_string_view command_name,
    ulk_string_view effects_json,
    ulk_string_view request_schema,
    ulk_string_view response_schema,
    ulk_string_view result_schema,
    ulk_string_view refusal_schema,
    ulk_string_view dry_run_behavior,
    ulk_string_view availability,
    ulk_string_view owner,
    ulk_string_view binding,
    void* user,
    ulk_command_handler_v1 handler
)
{
    ulk_registered_command pending;
    int capacity_status;

    if (context == 0 ||
        !ulk_is_canonical_command_id(command_name) ||
        ulk_string_equals(command_name, "command_graph.inspect") ||
        !ulk_effects_json_is_valid(effects_json) ||
        !ulk_view_is_text(request_schema) ||
        !ulk_view_is_text(response_schema) ||
        !ulk_view_is_text(result_schema) ||
        !ulk_view_is_text(refusal_schema) ||
        !ulk_view_is_text(dry_run_behavior) ||
        !ulk_view_is_text(availability) ||
        !ulk_view_is_text(owner) ||
        !ulk_view_is_text(binding) ||
        handler == 0 ||
        ulk_find_registered(context, command_name) != 0) {
        return ULK_STATUS_INVALID_ARGUMENT;
    }

    capacity_status = ulk_ensure_registry_capacity(context, context->registered_count + 1);
    if (capacity_status != ULK_STATUS_OK) {
        return capacity_status;
    }

    memset(&pending, 0, sizeof(pending));
    pending.command_name = ulk_copy_view(context, command_name);
    pending.effects_json = ulk_copy_view(context, effects_json);
    pending.request_schema = ulk_copy_view(context, request_schema);
    pending.response_schema = ulk_copy_view(context, response_schema);
    pending.result_schema = ulk_copy_view(context, result_schema);
    pending.refusal_schema = ulk_copy_view(context, refusal_schema);
    pending.dry_run_behavior = ulk_copy_view(context, dry_run_behavior);
    pending.availability = ulk_copy_view(context, availability);
    pending.owner = ulk_copy_view(context, owner);
    pending.binding = ulk_copy_view(context, binding);

    if (pending.command_name == 0 ||
        pending.effects_json == 0 ||
        pending.request_schema == 0 ||
        pending.response_schema == 0 ||
        pending.result_schema == 0 ||
        pending.refusal_schema == 0 ||
        pending.dry_run_behavior == 0 ||
        pending.availability == 0 ||
        pending.owner == 0 ||
        pending.binding == 0) {
        ulk_clear_registered(context, &pending);
        return ULK_STATUS_ERROR;
    }

    pending.command_name_size = command_name.size;
    pending.user = user;
    pending.handler = handler;
    context->registered[context->registered_count++] = pending;
    ulk_invalidate_graph(context);
    return ULK_STATUS_OK;
}

static ulk_command_metadata ulk_metadata_from_registered(
    const ulk_registered_command* entry
)
{
    ulk_command_metadata metadata;
    metadata.command_name = entry->command_name;
    metadata.effects_json = entry->effects_json;
    metadata.request_schema = entry->request_schema;
    metadata.response_schema = entry->response_schema;
    metadata.result_schema = entry->result_schema;
    metadata.refusal_schema = entry->refusal_schema;
    metadata.dry_run_behavior = entry->dry_run_behavior;
    metadata.availability = entry->availability;
    metadata.owner = entry->owner;
    metadata.binding = entry->binding;
    metadata.handler_status = "registered";
    metadata.builtin_kind = ULK_BUILTIN_NONE;
    return metadata;
}

static int ulk_json_append_raw(ulk_json_buffer* buffer, const char* text)
{
    ulk_size size = (ulk_size)strlen(text);
    if (size > buffer->capacity - buffer->length - 1) {
        return 0;
    }
    memcpy(buffer->data + buffer->length, text, (size_t)size);
    buffer->length += size;
    buffer->data[buffer->length] = '\0';
    return 1;
}

static int ulk_json_append_char(ulk_json_buffer* buffer, char value)
{
    if (buffer->length + 1 >= buffer->capacity) {
        return 0;
    }
    buffer->data[buffer->length++] = value;
    buffer->data[buffer->length] = '\0';
    return 1;
}

static int ulk_json_append_string(ulk_json_buffer* buffer, const char* text)
{
    static const char hex[] = "0123456789abcdef";
    const unsigned char* current = (const unsigned char*)text;

    if (!ulk_json_append_char(buffer, '"')) {
        return 0;
    }
    while (*current != 0) {
        unsigned char value = *current++;
        if (value == '"' || value == '\\') {
            if (!ulk_json_append_char(buffer, '\\') ||
                !ulk_json_append_char(buffer, (char)value)) {
                return 0;
            }
        } else if (value == '\b' || value == '\f' || value == '\n' ||
                   value == '\r' || value == '\t') {
            char escaped = value == '\b' ? 'b' :
                value == '\f' ? 'f' :
                value == '\n' ? 'n' :
                value == '\r' ? 'r' : 't';
            if (!ulk_json_append_char(buffer, '\\') ||
                !ulk_json_append_char(buffer, escaped)) {
                return 0;
            }
        } else if (value < 0x20) {
            if (!ulk_json_append_raw(buffer, "\\u00") ||
                !ulk_json_append_char(buffer, hex[(value >> 4) & 0x0f]) ||
                !ulk_json_append_char(buffer, hex[value & 0x0f])) {
                return 0;
            }
        } else if (!ulk_json_append_char(buffer, (char)value)) {
            return 0;
        }
    }
    return ulk_json_append_char(buffer, '"');
}

static int ulk_json_append_field(
    ulk_json_buffer* buffer,
    const char* name,
    const char* value
)
{
    return ulk_json_append_string(buffer, name) &&
        ulk_json_append_char(buffer, ':') &&
        ulk_json_append_string(buffer, value);
}

static int ulk_json_append_metadata(
    ulk_json_buffer* buffer,
    const ulk_command_metadata* metadata,
    int prepend_comma
)
{
    if ((prepend_comma && !ulk_json_append_char(buffer, ',')) ||
        !ulk_json_append_char(buffer, '{') ||
        !ulk_json_append_field(buffer, "command", metadata->command_name) ||
        !ulk_json_append_char(buffer, ',') ||
        !ulk_json_append_field(buffer, "request_schema", metadata->request_schema) ||
        !ulk_json_append_char(buffer, ',') ||
        !ulk_json_append_field(buffer, "response_schema", metadata->response_schema) ||
        !ulk_json_append_char(buffer, ',') ||
        !ulk_json_append_field(buffer, "result_schema", metadata->result_schema) ||
        !ulk_json_append_char(buffer, ',') ||
        !ulk_json_append_field(buffer, "refusal_schema", metadata->refusal_schema) ||
        !ulk_json_append_char(buffer, ',') ||
        !ulk_json_append_field(buffer, "dry_run_behavior", metadata->dry_run_behavior) ||
        !ulk_json_append_char(buffer, ',') ||
        !ulk_json_append_field(buffer, "availability", metadata->availability) ||
        !ulk_json_append_char(buffer, ',') ||
        !ulk_json_append_field(buffer, "owner", metadata->owner) ||
        !ulk_json_append_char(buffer, ',') ||
        !ulk_json_append_field(buffer, "binding", metadata->binding) ||
        !ulk_json_append_char(buffer, ',') ||
        !ulk_json_append_field(buffer, "handler_status", metadata->handler_status) ||
        !ulk_json_append_raw(buffer, ",\"executable\":true,\"effects\":") ||
        !ulk_json_append_raw(buffer, metadata->effects_json) ||
        !ulk_json_append_char(buffer, '}')) {
        return 0;
    }
    return 1;
}

static int ulk_capacity_add(ulk_size* capacity, ulk_size amount)
{
    ulk_size maximum = (ulk_size)((size_t)-1);
    if (amount > maximum - *capacity) {
        return 0;
    }
    *capacity += amount;
    return 1;
}

static int ulk_capacity_add_metadata(
    ulk_size* capacity,
    const ulk_command_metadata* metadata
)
{
    const char* fields[] = {
        metadata->command_name,
        metadata->effects_json,
        metadata->request_schema,
        metadata->response_schema,
        metadata->result_schema,
        metadata->refusal_schema,
        metadata->dry_run_behavior,
        metadata->availability,
        metadata->owner,
        metadata->binding,
        metadata->handler_status
    };
    ulk_size index;

    if (!ulk_capacity_add(capacity, 512)) {
        return 0;
    }
    for (index = 0; index < (ulk_size)(sizeof(fields) / sizeof(fields[0])); ++index) {
        ulk_size length = (ulk_size)strlen(fields[index]);
        if (length > ((ulk_size)((size_t)-1)) / 6 ||
            !ulk_capacity_add(capacity, length * 6)) {
            return 0;
        }
    }
    return 1;
}

static int ulk_command_is_builtin(const char* command_name)
{
    return ulk_find_builtin(ulk_view_from_cstr(command_name)) != 0;
}

static int ulk_build_command_graph(
    ulk_context* context,
    ulk_command_response_v1* response
)
{
    static const char graph_error_payload[] =
        "{\"schema\":\"ulk.command_response.v1\",\"status\":\"error\",\"payload\":null,\"error\":{\"code\":\"graph_projection_failed\",\"message\":\"Command graph projection failed\"}}";
    static const char graph_error_message[] = "Command graph projection failed";
    ulk_size capacity = 512;
    ulk_size index;
    int has_entry = 0;
    ulk_json_buffer buffer;

    ulk_invalidate_graph(context);
    for (index = 0; index < ULK_BUILTIN_COMMAND_COUNT; ++index) {
        ulk_registered_command* registered = ulk_find_registered(
            context,
            ulk_view_from_cstr(ULK_BUILTIN_COMMANDS[index].command_name));
        ulk_command_metadata metadata = registered == 0
            ? ULK_BUILTIN_COMMANDS[index]
            : ulk_metadata_from_registered(registered);
        if (!ulk_capacity_add_metadata(&capacity, &metadata)) {
            ulk_set_response(response, ULK_STATUS_ERROR, graph_error_payload, graph_error_message);
            return ULK_STATUS_ERROR;
        }
    }
    for (index = 0; index < context->registered_count; ++index) {
        ulk_command_metadata metadata;
        if (ulk_command_is_builtin(context->registered[index].command_name)) {
            continue;
        }
        metadata = ulk_metadata_from_registered(&context->registered[index]);
        if (!ulk_capacity_add_metadata(&capacity, &metadata)) {
            ulk_set_response(response, ULK_STATUS_ERROR, graph_error_payload, graph_error_message);
            return ULK_STATUS_ERROR;
        }
    }

    context->command_graph_json = (char*)context->allocator.alloc(
        context->allocator.user,
        capacity);
    if (context->command_graph_json == 0) {
        ulk_set_response(response, ULK_STATUS_ERROR, graph_error_payload, graph_error_message);
        return ULK_STATUS_ERROR;
    }
    buffer.data = context->command_graph_json;
    buffer.length = 0;
    buffer.capacity = capacity;
    buffer.data[0] = '\0';

    if (!ulk_json_append_raw(
            &buffer,
            "{\"schema\":\"ulk.command_response.v1\",\"status\":\"ok\","
            "\"payload\":{\"schema\":\"ulk.command_graph.v1\",\"commands\":["))
    {
        ulk_invalidate_graph(context);
        ulk_set_response(response, ULK_STATUS_ERROR, graph_error_payload, graph_error_message);
        return ULK_STATUS_ERROR;
    }

    for (index = 0; index < ULK_BUILTIN_COMMAND_COUNT; ++index) {
        ulk_registered_command* registered = ulk_find_registered(
            context,
            ulk_view_from_cstr(ULK_BUILTIN_COMMANDS[index].command_name));
        ulk_command_metadata metadata = registered == 0
            ? ULK_BUILTIN_COMMANDS[index]
            : ulk_metadata_from_registered(registered);
        if (!ulk_json_append_metadata(&buffer, &metadata, has_entry)) {
            ulk_invalidate_graph(context);
            ulk_set_response(response, ULK_STATUS_ERROR, graph_error_payload, graph_error_message);
            return ULK_STATUS_ERROR;
        }
        has_entry = 1;
    }
    for (index = 0; index < context->registered_count; ++index) {
        ulk_command_metadata metadata;
        if (ulk_command_is_builtin(context->registered[index].command_name)) {
            continue;
        }
        metadata = ulk_metadata_from_registered(&context->registered[index]);
        if (!ulk_json_append_metadata(&buffer, &metadata, has_entry)) {
            ulk_invalidate_graph(context);
            ulk_set_response(response, ULK_STATUS_ERROR, graph_error_payload, graph_error_message);
            return ULK_STATUS_ERROR;
        }
        has_entry = 1;
    }

    if (!ulk_json_append_raw(&buffer, "]},\"error\":null}")) {
        ulk_invalidate_graph(context);
        ulk_set_response(response, ULK_STATUS_ERROR, graph_error_payload, graph_error_message);
        return ULK_STATUS_ERROR;
    }

    ulk_set_response(response, ULK_STATUS_OK, context->command_graph_json, 0);
    return ULK_STATUS_OK;
}

static ulk_static_response ulk_dispatch_builtin(
    int builtin_kind,
    const ulk_command_request_v1* request
)
{
    static const char product_payload[] =
        "{\"schema\":\"ulk.command_response.v1\",\"status\":\"ok\",\"payload\":{\"schema\":\"ulk.product.v1\",\"product_id\":\"universal.default\",\"binding_id\":\"external.product_binding\",\"registry_status\":\"empty\",\"models\":{\"install_ref\":\"ulk.install_ref.v1\",\"instance\":\"ulk.instance.v1\",\"profile\":\"ulk.profile.v1\",\"account_ref\":\"ulk.account_ref.v1\",\"artifact_set\":\"ulk.artifact_set.v1\",\"launch_plan\":\"ulk.launch_plan.v1\",\"diagnostic_report\":\"ulk.diagnostic_report.v1\"}},\"error\":null}";
    static const char install_refs_payload[] =
        "{\"schema\":\"ulk.command_response.v1\",\"status\":\"ok\",\"payload\":{\"schema\":\"ulk.install_refs.v1\",\"install_refs\":[],\"model\":\"ulk.install_ref.v1\"},\"error\":null}";
    static const char install_ref_inspect_payload[] =
        "{\"schema\":\"ulk.command_response.v1\",\"status\":\"ok\",\"payload\":{\"schema\":\"ulk.install_ref_inspect.v1\",\"status\":\"not_found\",\"install_ref\":null,\"registry_status\":\"empty\",\"diagnostics\":[{\"code\":\"empty_registry\",\"message\":\"no install refs are registered in the minimal universal launcher kernel\"}]},\"error\":null}";
    static const char instances_payload[] =
        "{\"schema\":\"ulk.command_response.v1\",\"status\":\"ok\",\"payload\":{\"schema\":\"ulk.instances.v1\",\"instances\":[],\"model\":\"ulk.instance.v1\"},\"error\":null}";
    static const char instance_create_preview_payload[] =
        "{\"schema\":\"ulk.command_response.v1\",\"status\":\"ok\",\"payload\":{\"schema\":\"ulk.instance_create_plan.v1\",\"status\":\"planned\",\"dry_run\":true,\"instance\":null,\"effects\":[\"workspace_write\"],\"execution\":\"requires_product_binding\",\"diagnostics\":[{\"code\":\"preview_only\",\"message\":\"minimal universal launcher can plan instance creation but owns no product-specific instance materialization\"}]},\"error\":null}";
    static const char instance_create_execute_refusal_payload[] =
        "{\"schema\":\"ulk.command_response.v1\",\"status\":\"refused\",\"payload\":{\"schema\":\"ulk.refusal.v1\",\"operation\":\"instance.create\",\"code\":\"product_binding_required\",\"severity\":\"error\",\"message\":\"instance creation requires a product binding and workspace store\",\"retry_possible\":true,\"suggested_next_command\":\"run the product binding instance creation command\"},\"error\":{\"code\":\"product_binding_required\",\"message\":\"instance creation requires a product binding and workspace store\"}}";
    static const char profiles_payload[] =
        "{\"schema\":\"ulk.command_response.v1\",\"status\":\"ok\",\"payload\":{\"schema\":\"ulk.profiles.v1\",\"profiles\":[],\"model\":\"ulk.profile.v1\"},\"error\":null}";
    static const char account_refs_payload[] =
        "{\"schema\":\"ulk.command_response.v1\",\"status\":\"ok\",\"payload\":{\"schema\":\"ulk.account_refs.v1\",\"account_refs\":[],\"model\":\"ulk.account_ref.v1\"},\"error\":null}";
    static const char artifact_sets_payload[] =
        "{\"schema\":\"ulk.command_response.v1\",\"status\":\"ok\",\"payload\":{\"schema\":\"ulk.artifact_sets.v1\",\"artifact_sets\":[],\"model\":\"ulk.artifact_set.v1\"},\"error\":null}";
    static const char launch_plan_dry_run_payload[] =
        "{\"schema\":\"ulk.command_response.v1\",\"status\":\"ok\",\"payload\":{\"schema\":\"ulk.launch_plan.v1\",\"plan_id\":\"ulk.plan.dry_run\",\"dry_run\":true,\"product_id\":\"universal.default\",\"install_ref\":null,\"instance\":null,\"profile\":null,\"account_ref\":null,\"artifact_set\":null,\"argv\":[],\"preflight\":[],\"steps\":[],\"execution\":\"not_started\"},\"error\":null}";
    static const char launch_plan_execute_payload[] =
        "{\"schema\":\"ulk.command_response.v1\",\"status\":\"ok\",\"payload\":{\"schema\":\"ulk.launch_plan.v1\",\"plan_id\":\"ulk.plan.preview\",\"dry_run\":false,\"product_id\":\"universal.default\",\"install_ref\":null,\"instance\":null,\"profile\":null,\"account_ref\":null,\"artifact_set\":null,\"argv\":[],\"preflight\":[],\"steps\":[],\"execution\":\"requires_product_binding\"},\"error\":null}";
    static const char diagnostics_payload[] =
        "{\"schema\":\"ulk.command_response.v1\",\"status\":\"ok\",\"payload\":{\"schema\":\"ulk.diagnostic_report.v1\",\"report_id\":\"ulk.diagnostic.minimal\",\"status\":\"ok\",\"checks\":[{\"id\":\"command_graph\",\"status\":\"ok\"},{\"id\":\"product_registry\",\"status\":\"empty\"},{\"id\":\"setup_mutation\",\"status\":\"not_owned\"}]},\"error\":null}";
    static const char instance_create_refusal_message[] =
        "instance creation requires a product binding and workspace store";
    static const char unsupported_payload[] =
        "{\"schema\":\"ulk.command_response.v1\",\"status\":\"unsupported\",\"payload\":null,\"error\":{\"code\":\"unsupported_command\",\"message\":\"Command is not supported by the Universal Launcher command graph\"}}";
    static const char unsupported_message[] =
        "Command is not supported by the Universal Launcher command graph";
    ulk_static_response result;

    result.status = ULK_STATUS_OK;
    result.error_message = 0;

    if (builtin_kind == ULK_BUILTIN_PRODUCT_INSPECT) {
        result.payload = product_payload;
        return result;
    }
    if (builtin_kind == ULK_BUILTIN_INSTALL_REFS_LIST) {
        result.payload = install_refs_payload;
        return result;
    }
    if (builtin_kind == ULK_BUILTIN_INSTALL_REFS_INSPECT) {
        result.payload = install_ref_inspect_payload;
        return result;
    }
    if (builtin_kind == ULK_BUILTIN_INSTANCE_CREATE) {
        if (request->dry_run) {
            result.payload = instance_create_preview_payload;
            return result;
        }
        result.status = ULK_STATUS_ERROR;
        result.payload = instance_create_execute_refusal_payload;
        result.error_message = instance_create_refusal_message;
        return result;
    }
    if (builtin_kind == ULK_BUILTIN_INSTANCE_LIST) {
        result.payload = instances_payload;
        return result;
    }
    if (builtin_kind == ULK_BUILTIN_PROFILES_LIST) {
        result.payload = profiles_payload;
        return result;
    }
    if (builtin_kind == ULK_BUILTIN_ACCOUNT_REFS_LIST) {
        result.payload = account_refs_payload;
        return result;
    }
    if (builtin_kind == ULK_BUILTIN_ARTIFACT_SETS_LIST) {
        result.payload = artifact_sets_payload;
        return result;
    }
    if (builtin_kind == ULK_BUILTIN_LAUNCH_PLAN_BUILD) {
        result.payload = request->dry_run ? launch_plan_dry_run_payload : launch_plan_execute_payload;
        return result;
    }
    if (builtin_kind == ULK_BUILTIN_DIAGNOSTICS_RUN) {
        result.payload = diagnostics_payload;
        return result;
    }

    result.status = ULK_STATUS_UNSUPPORTED_VERSION;
    result.payload = unsupported_payload;
    result.error_message = unsupported_message;
    return result;
}

int ulk_context_create_v1(
    const ulk_allocator_v1* allocator,
    ulk_context** out_context
)
{
    ulk_context* context;
    ulk_allocator_v1 effective_allocator;

    if (out_context == 0) {
        return ULK_STATUS_INVALID_ARGUMENT;
    }
    *out_context = 0;

    effective_allocator.struct_size = sizeof(effective_allocator);
    effective_allocator.user = 0;
    effective_allocator.alloc = ulk_default_alloc;
    effective_allocator.free = ulk_default_free;

    if (allocator != 0 &&
        (allocator->struct_size < (ulk_size)sizeof(*allocator) ||
         ((allocator->alloc == 0) != (allocator->free == 0)))) {
        return ULK_STATUS_INVALID_ARGUMENT;
    }
    if (allocator != 0 && allocator->alloc != 0) {
        effective_allocator = *allocator;
    }

    context = (ulk_context*)effective_allocator.alloc(
        effective_allocator.user,
        (ulk_size)sizeof(*context));
    if (context == 0) {
        return ULK_STATUS_ERROR;
    }

    memset(context, 0, sizeof(*context));
    context->allocator = effective_allocator;
    context->has_allocator = 1;
    *out_context = context;
    return ULK_STATUS_OK;
}

int ulk_command_execute_v1(
    ulk_context* context,
    const ulk_command_request_v1* request,
    ulk_command_response_v1* response
)
{
    static const char invalid_payload[] =
        "{\"schema\":\"ulk.command_response.v1\",\"status\":\"invalid_argument\",\"payload\":null,\"error\":{\"code\":\"invalid_argument\",\"message\":\"Universal Launcher command request is invalid\"}}";
    static const char invalid_message[] = "Universal Launcher command request is invalid";
    ulk_static_response dispatched;
    ulk_registered_command* registered;
    const ulk_command_metadata* builtin;

    if (response == 0 || response->struct_size < (ulk_size)sizeof(*response)) {
        return ULK_STATUS_INVALID_ARGUMENT;
    }

    if (context == 0 ||
        request == 0 ||
        request->struct_size < (ulk_size)sizeof(*request) ||
        request->command_name.data == 0 ||
        request->command_name.size == 0) {
        ulk_set_response(response, ULK_STATUS_INVALID_ARGUMENT, invalid_payload, invalid_message);
        return ULK_STATUS_INVALID_ARGUMENT;
    }

    registered = ulk_find_registered(context, request->command_name);
    if (registered != 0) {
        return registered->handler(registered->user, request, response);
    }

    builtin = ulk_find_builtin(request->command_name);
    if (builtin != 0 && builtin->builtin_kind == ULK_BUILTIN_COMMAND_GRAPH_INSPECT) {
        return ulk_build_command_graph(context, response);
    }
    dispatched = ulk_dispatch_builtin(
        builtin == 0 ? ULK_BUILTIN_NONE : builtin->builtin_kind,
        request);
    ulk_set_response(response, dispatched.status, dispatched.payload, dispatched.error_message);
    return dispatched.status;
}

int ulk_command_register_v1(
    ulk_context* context,
    const ulk_command_descriptor_v1* descriptor
)
{
    static const char default_effects[] = "[\"none\"]";
    static const char request_schema[] = "ulk.command_request.v1";
    static const char response_schema[] = "ulk.command_response.v1";
    static const char result_schema[] = "ulk.result.v1";
    static const char refusal_schema[] = "ulk.refusal.v1";
    static const char dry_run_behavior[] = "binding_defined";
    static const char availability[] = "available";
    static const char owner[] = "registered.binding";
    static const char binding[] = "legacy.v1";
    ulk_string_view effects;

    if (descriptor == 0 ||
        descriptor->struct_size < (ulk_size)sizeof(*descriptor)) {
        return ULK_STATUS_INVALID_ARGUMENT;
    }
    effects = descriptor->effects_json.data == 0 || descriptor->effects_json.size == 0
        ? ulk_view_from_cstr(default_effects)
        : descriptor->effects_json;
    return ulk_register_internal(
        context,
        descriptor->command_name,
        effects,
        ulk_view_from_cstr(request_schema),
        ulk_view_from_cstr(response_schema),
        ulk_view_from_cstr(result_schema),
        ulk_view_from_cstr(refusal_schema),
        ulk_view_from_cstr(dry_run_behavior),
        ulk_view_from_cstr(availability),
        ulk_view_from_cstr(owner),
        ulk_view_from_cstr(binding),
        descriptor->user,
        descriptor->handler);
}

int ulk_command_register_v2(
    ulk_context* context,
    const ulk_command_descriptor_v2* descriptor
)
{
    if (descriptor == 0 ||
        descriptor->struct_size < (ulk_size)sizeof(*descriptor)) {
        return ULK_STATUS_INVALID_ARGUMENT;
    }
    return ulk_register_internal(
        context,
        descriptor->command_name,
        descriptor->effects_json,
        descriptor->request_schema,
        descriptor->response_schema,
        descriptor->result_schema,
        descriptor->refusal_schema,
        descriptor->dry_run_behavior,
        descriptor->availability,
        descriptor->owner,
        descriptor->binding,
        descriptor->user,
        descriptor->handler);
}

int ulk_command_unregister_v1(
    ulk_context* context,
    ulk_string_view command_name
)
{
    ulk_size index;
    if (context == 0 || !ulk_is_canonical_command_id(command_name)) {
        return ULK_STATUS_INVALID_ARGUMENT;
    }
    for (index = 0; index < context->registered_count; ++index) {
        if (context->registered[index].command_name_size == command_name.size &&
            memcmp(
                context->registered[index].command_name,
                command_name.data,
                (size_t)command_name.size) == 0) {
            ulk_size move_index;
            ulk_clear_registered(context, &context->registered[index]);
            for (move_index = index; move_index + 1 < context->registered_count; ++move_index) {
                context->registered[move_index] = context->registered[move_index + 1];
            }
            memset(
                &context->registered[context->registered_count - 1],
                0,
                sizeof(context->registered[0]));
            --context->registered_count;
            ulk_invalidate_graph(context);
            return ULK_STATUS_OK;
        }
    }
    return ULK_STATUS_INVALID_ARGUMENT;
}

uint32_t ulk_abi_version_v1(void)
{
    return ((uint32_t)ULK_API_VERSION_MAJOR << 16) | (uint32_t)ULK_API_VERSION_MINOR;
}

void ulk_context_destroy_v1(ulk_context* context)
{
    ulk_allocator_v1 allocator;
    ulk_size index;

    if (context == 0) {
        return;
    }

    allocator = context->allocator;
    ulk_invalidate_graph(context);
    for (index = 0; index < context->registered_count; ++index) {
        ulk_clear_registered(context, &context->registered[index]);
    }
    if (context->registered != 0) {
        allocator.free(allocator.user, context->registered);
    }
    if (context->has_allocator && allocator.free != 0) {
        allocator.free(allocator.user, context);
        return;
    }
    free(context);
}
