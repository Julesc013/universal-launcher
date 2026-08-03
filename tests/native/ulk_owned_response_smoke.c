// SPDX-FileCopyrightText: 2026 Jules C
// SPDX-License-Identifier: MIT

#include "ulk/ulk_api.h"

#include <stdlib.h>
#include <string.h>

#define CHECK(condition, code) do { if (!(condition)) return (code); } while (0)

typedef struct allocation_state {
    int allocation_calls;
    int free_calls;
    int fail_on_call;
    ulk_size last_size;
} allocation_state;

typedef struct handler_fixture {
    char payload[64];
    char message[32];
    char detail[32];
} handler_fixture;

static ulk_string_view view_bytes(const char* data, ulk_size size)
{
    ulk_string_view result;
    result.data = data;
    result.size = size;
    return result;
}

static ulk_string_view view(const char* text)
{
    return view_bytes(text, text == 0 ? 0u : (ulk_size)strlen(text));
}

static void fill_response(
    ulk_command_response_v1* response,
    int status,
    ulk_string_view payload,
    ulk_string_view message,
    ulk_string_view detail
)
{
    memset(response, 0, sizeof(*response));
    response->struct_size = sizeof(*response);
    response->status = status;
    response->json_payload = payload;
    response->error.struct_size = sizeof(response->error);
    response->error.code = status;
    response->error.message = message;
    response->error.detail = detail;
}

static void prepare_owned(ulk_owned_command_response_v1* response)
{
    memset(response, 0, sizeof(*response));
    response->struct_size = sizeof(*response);
}

static int view_equals(ulk_string_view value, const char* expected, ulk_size size)
{
    return
        value.size == size &&
        (size == 0u || memcmp(value.data, expected, (size_t)size) == 0);
}

static int view_contains(ulk_string_view value, const char* expected)
{
    const ulk_size expected_size = (ulk_size)strlen(expected);
    ulk_size index;
    if (expected_size == 0u) {
        return 1;
    }
    if (value.data == 0 || value.size < expected_size) {
        return 0;
    }
    for (index = 0u; index <= value.size - expected_size; ++index) {
        if (memcmp(value.data + index, expected, (size_t)expected_size) == 0) {
            return 1;
        }
    }
    return 0;
}

static void* ULK_CALL counting_alloc(void* user, ulk_size size)
{
    allocation_state* state = (allocation_state*)user;
    state->allocation_calls += 1;
    state->last_size = size;
    if (state->fail_on_call == state->allocation_calls) {
        return 0;
    }
    return malloc((size_t)size);
}

static void ULK_CALL counting_free(void* user, void* pointer)
{
    allocation_state* state = (allocation_state*)user;
    state->free_calls += 1;
    free(pointer);
}

static ulk_allocator_v1 allocator_for(allocation_state* state)
{
    ulk_allocator_v1 allocator;
    memset(&allocator, 0, sizeof(allocator));
    allocator.struct_size = sizeof(allocator);
    allocator.user = state;
    allocator.alloc = counting_alloc;
    allocator.free = counting_free;
    return allocator;
}

static int test_validation(void)
{
    ulk_command_response_v1 response;

    fill_response(
        &response,
        ULK_STATUS_OK,
        view("{}"),
        view(0),
        view(0));
    CHECK(ulk_command_response_validate_v1(&response) == ULK_STATUS_OK, 1);
    CHECK(ulk_command_response_validate_v1(0) == ULK_STATUS_INVALID_ARGUMENT, 2);

    response.struct_size = sizeof(response) - 1u;
    CHECK(ulk_command_response_validate_v1(&response) == ULK_STATUS_INVALID_ARGUMENT, 3);
    response.struct_size = sizeof(response);

    response.status = 99;
    CHECK(ulk_command_response_validate_v1(&response) == ULK_STATUS_INVALID_ARGUMENT, 4);
    response.status = ULK_STATUS_OK;

    response.json_payload.data = 0;
    response.json_payload.size = 1u;
    CHECK(ulk_command_response_validate_v1(&response) == ULK_STATUS_INVALID_ARGUMENT, 5);
    response.json_payload = view("{}");

    response.error.struct_size = sizeof(response.error) - 1u;
    CHECK(ulk_command_response_validate_v1(&response) == ULK_STATUS_INVALID_ARGUMENT, 6);
    response.error.struct_size = sizeof(response.error);

    response.error.message.data = 0;
    response.error.message.size = 1u;
    CHECK(ulk_command_response_validate_v1(&response) == ULK_STATUS_INVALID_ARGUMENT, 7);
    response.error.message = view(0);

    response.error.detail.data = 0;
    response.error.detail.size = 1u;
    CHECK(ulk_command_response_validate_v1(&response) == ULK_STATUS_INVALID_ARGUMENT, 8);
    response.error.detail = view(0);

    response.struct_size = (ulk_size)-1;
    response.error.struct_size = (ulk_size)-1;
    CHECK(ulk_command_response_validate_v1(&response) == ULK_STATUS_OK, 9);
    response.status = ULK_STATUS_ERROR;
    CHECK(ulk_command_response_validate_v1(&response) == ULK_STATUS_OK, 86);
    response.status = ULK_STATUS_INVALID_ARGUMENT;
    CHECK(ulk_command_response_validate_v1(&response) == ULK_STATUS_OK, 87);
    response.status = ULK_STATUS_UNSUPPORTED_VERSION;
    CHECK(ulk_command_response_validate_v1(&response) == ULK_STATUS_OK, 88);
    return 0;
}

static int test_default_copy_and_release(void)
{
    char payload[] = {'a', 'b', 'c'};
    char message[] = {'m', 's', 'g'};
    char detail[] = {'d', 'e', 't', 'a', 'i', 'l'};
    ulk_command_response_v1 source;
    ulk_owned_command_response_v1 owned;
    void* storage;

    fill_response(
        &source,
        ULK_STATUS_ERROR,
        view_bytes(payload, sizeof(payload)),
        view_bytes(message, sizeof(message)),
        view_bytes(detail, sizeof(detail)));
    source.error.code = 731;
    prepare_owned(&owned);

    CHECK(
        ulk_command_response_copy_owned_v1(&source, 0, &owned) == ULK_STATUS_OK,
        10);
    CHECK(owned.storage != 0, 11);
    CHECK(owned.storage_size == sizeof(payload) + sizeof(message) + sizeof(detail), 12);
    CHECK(owned.response.status == ULK_STATUS_ERROR, 13);
    CHECK(owned.response.error.code == 731, 14);
    CHECK(view_equals(owned.response.json_payload, "abc", 3u), 15);
    CHECK(view_equals(owned.response.error.message, "msg", 3u), 16);
    CHECK(view_equals(owned.response.error.detail, "detail", 6u), 17);
    CHECK(owned.response.json_payload.data != payload, 18);
    CHECK(owned.response.error.message.data != message, 19);
    CHECK(owned.response.error.detail.data != detail, 20);

    memset(payload, 'x', sizeof(payload));
    memset(message, 'x', sizeof(message));
    memset(detail, 'x', sizeof(detail));
    CHECK(view_equals(owned.response.json_payload, "abc", 3u), 21);
    CHECK(view_equals(owned.response.error.message, "msg", 3u), 22);
    CHECK(view_equals(owned.response.error.detail, "detail", 6u), 23);

    storage = owned.storage;
    ulk_owned_command_response_release_v1(&owned);
    CHECK(storage != owned.storage && owned.storage == 0, 24);
    CHECK(owned.storage_size == 0u, 25);
    CHECK(owned.struct_size == sizeof(owned), 26);
    CHECK(owned.response.struct_size == sizeof(owned.response), 27);
    CHECK(owned.response.error.struct_size == sizeof(owned.response.error), 28);
    CHECK(owned.response.json_payload.size == 0u, 29);
    ulk_owned_command_response_release_v1(&owned);
    CHECK(owned.storage == 0, 30);
    ulk_owned_command_response_release_v1(0);
    return 0;
}

static int test_allocator_and_empty_views(void)
{
    allocation_state state;
    ulk_allocator_v1 allocator;
    ulk_command_response_v1 source;
    ulk_owned_command_response_v1 owned;
    void* storage;

    memset(&state, 0, sizeof(state));
    allocator = allocator_for(&state);
    fill_response(
        &source,
        ULK_STATUS_OK,
        view("payload"),
        view(0),
        view("detail"));
    prepare_owned(&owned);
    CHECK(
        ulk_command_response_copy_owned_v1(&source, &allocator, &owned) ==
            ULK_STATUS_OK,
        31);
    CHECK(state.allocation_calls == 1, 32);
    CHECK(state.last_size == 13u, 33);
    CHECK(owned.storage_size == state.last_size, 34);
    CHECK(owned.response.error.message.data == 0, 35);
    CHECK(owned.response.error.message.size == 0u, 36);
    storage = owned.storage;
    CHECK(
        ulk_command_response_copy_owned_v1(&source, &allocator, &owned) ==
            ULK_STATUS_INVALID_ARGUMENT,
        37);
    CHECK(owned.storage == storage, 38);
    CHECK(state.allocation_calls == 1 && state.free_calls == 0, 39);
    allocator.free = 0;
    ulk_owned_command_response_release_v1(&owned);
    CHECK(state.free_calls == 1, 40);
    ulk_owned_command_response_release_v1(&owned);
    CHECK(state.free_calls == 1, 41);

    memset(&state, 0, sizeof(state));
    allocator = allocator_for(&state);
    fill_response(
        &source,
        ULK_STATUS_OK,
        view(0),
        view(0),
        view(0));
    prepare_owned(&owned);
    CHECK(
        ulk_command_response_copy_owned_v1(&source, &allocator, &owned) ==
            ULK_STATUS_OK,
        42);
    CHECK(state.allocation_calls == 0, 43);
    CHECK(owned.storage == 0 && owned.storage_size == 0u, 44);
    CHECK(owned.response.json_payload.data == 0, 45);
    ulk_owned_command_response_release_v1(&owned);
    CHECK(state.free_calls == 0, 46);
    return 0;
}

static int test_invalid_allocator_and_destination(void)
{
    allocation_state state;
    ulk_allocator_v1 allocator;
    ulk_command_response_v1 source;
    ulk_owned_command_response_v1 owned;

    memset(&state, 0, sizeof(state));
    allocator = allocator_for(&state);
    fill_response(&source, ULK_STATUS_OK, view("{}"), view(0), view(0));
    prepare_owned(&owned);

    CHECK(
        ulk_command_response_copy_owned_v1(&source, &allocator, 0) ==
            ULK_STATUS_INVALID_ARGUMENT,
        47);
    owned.struct_size = sizeof(owned) - 1u;
    CHECK(
        ulk_command_response_copy_owned_v1(&source, &allocator, &owned) ==
            ULK_STATUS_INVALID_ARGUMENT,
        48);
    prepare_owned(&owned);

    allocator.struct_size = sizeof(allocator) - 1u;
    CHECK(
        ulk_command_response_copy_owned_v1(&source, &allocator, &owned) ==
            ULK_STATUS_INVALID_ARGUMENT,
        49);
    CHECK(owned.storage == 0, 50);

    allocator = allocator_for(&state);
    allocator.free = 0;
    CHECK(
        ulk_command_response_copy_owned_v1(&source, &allocator, &owned) ==
            ULK_STATUS_INVALID_ARGUMENT,
        51);

    allocator = allocator_for(&state);
    allocator.alloc = 0;
    CHECK(
        ulk_command_response_copy_owned_v1(&source, &allocator, &owned) ==
            ULK_STATUS_INVALID_ARGUMENT,
        52);

    memset(&allocator, 0, sizeof(allocator));
    allocator.struct_size = sizeof(allocator);
    CHECK(
        ulk_command_response_copy_owned_v1(&source, &allocator, &owned) ==
            ULK_STATUS_OK,
        53);
    CHECK(owned.storage != 0, 54);
    ulk_owned_command_response_release_v1(&owned);

    allocator = allocator_for(&state);
    source.status = 99;
    prepare_owned(&owned);
    owned.response.status = 99;
    CHECK(
        ulk_command_response_copy_owned_v1(&source, &allocator, &owned) ==
            ULK_STATUS_INVALID_ARGUMENT,
        121);
    CHECK(state.allocation_calls == 0 && owned.storage == 0, 122);
    CHECK(
        owned.response.status == 0 &&
        owned.response.struct_size == sizeof(owned.response) &&
        owned.response.error.struct_size == sizeof(owned.response.error),
        123);
    return 0;
}

static int test_allocation_failure(void)
{
    allocation_state state;
    ulk_allocator_v1 allocator;
    ulk_command_response_v1 source;
    ulk_owned_command_response_v1 owned;

    memset(&state, 0, sizeof(state));
    state.fail_on_call = 1;
    allocator = allocator_for(&state);
    fill_response(
        &source,
        ULK_STATUS_ERROR,
        view("payload"),
        view("message"),
        view("detail"));
    prepare_owned(&owned);
    CHECK(
        ulk_command_response_copy_owned_v1(&source, &allocator, &owned) ==
            ULK_STATUS_ERROR,
        55);
    CHECK(state.allocation_calls == 1 && state.free_calls == 0, 56);
    CHECK(owned.storage == 0 && owned.storage_size == 0u, 57);
    CHECK(owned.response.json_payload.data == 0, 58);
    ulk_owned_command_response_release_v1(&owned);
    ulk_owned_command_response_release_v1(&owned);
    CHECK(state.free_calls == 0, 59);
    return 0;
}

static int test_budget_and_overflow(void)
{
    allocation_state state;
    ulk_allocator_v1 allocator;
    ulk_command_response_v1 source;
    ulk_owned_command_response_v1 owned;
    char* exact_budget;
    const char byte = 'x';

    exact_budget = (char*)malloc(
        (size_t)ULK_OWNED_COMMAND_RESPONSE_BYTE_BUDGET_V1);
    CHECK(exact_budget != 0, 60);
    memset(
        exact_budget,
        'b',
        (size_t)ULK_OWNED_COMMAND_RESPONSE_BYTE_BUDGET_V1);
    memset(&state, 0, sizeof(state));
    allocator = allocator_for(&state);
    fill_response(
        &source,
        ULK_STATUS_OK,
        view_bytes(
            exact_budget,
            ULK_OWNED_COMMAND_RESPONSE_BYTE_BUDGET_V1),
        view(0),
        view(0));
    prepare_owned(&owned);
    CHECK(
        ulk_command_response_copy_owned_v1(&source, &allocator, &owned) ==
            ULK_STATUS_OK,
        61);
    CHECK(
        state.last_size == ULK_OWNED_COMMAND_RESPONSE_BYTE_BUDGET_V1,
        62);
    CHECK(
        owned.response.json_payload.size ==
            ULK_OWNED_COMMAND_RESPONSE_BYTE_BUDGET_V1,
        63);
    CHECK(
        owned.response.json_payload.data[0] == 'b' &&
        owned.response.json_payload.data[
            ULK_OWNED_COMMAND_RESPONSE_BYTE_BUDGET_V1 - 1u] == 'b',
        64);
    ulk_owned_command_response_release_v1(&owned);
    CHECK(state.free_calls == 1, 65);
    free(exact_budget);

    memset(&state, 0, sizeof(state));
    allocator = allocator_for(&state);
    fill_response(
        &source,
        ULK_STATUS_OK,
        view_bytes(
            &byte,
            ULK_OWNED_COMMAND_RESPONSE_BYTE_BUDGET_V1 + 1u),
        view(0),
        view(0));
    prepare_owned(&owned);
    CHECK(
        ulk_command_response_copy_owned_v1(&source, &allocator, &owned) ==
            ULK_STATUS_INVALID_ARGUMENT,
        66);
    CHECK(state.allocation_calls == 0 && owned.storage == 0, 67);

    fill_response(
        &source,
        ULK_STATUS_OK,
        view_bytes(&byte, (ulk_size)-1),
        view_bytes(&byte, 2u),
        view(0));
    prepare_owned(&owned);
    CHECK(
        ulk_command_response_copy_owned_v1(&source, &allocator, &owned) ==
            ULK_STATUS_INVALID_ARGUMENT,
        68);
    CHECK(state.allocation_calls == 0 && owned.storage == 0, 69);

    fill_response(
        &source,
        ULK_STATUS_OK,
        view_bytes(&byte, (ulk_size)-2),
        view_bytes(&byte, 1u),
        view_bytes(&byte, 2u));
    prepare_owned(&owned);
    CHECK(
        ulk_command_response_copy_owned_v1(&source, &allocator, &owned) ==
            ULK_STATUS_INVALID_ARGUMENT,
        124);
    CHECK(state.allocation_calls == 0 && owned.storage == 0, 125);
    return 0;
}

static int ULK_CALL fixture_handler(
    void* user,
    const ulk_command_request_v1* request,
    ulk_command_response_v1* response
)
{
    handler_fixture* fixture = (handler_fixture*)user;
    (void)request;
    fill_response(
        response,
        ULK_STATUS_ERROR,
        view(fixture->payload),
        view(fixture->message),
        view(fixture->detail));
    response->error.code = 812;
    return ULK_STATUS_ERROR;
}

static int execute_command(
    ulk_context* context,
    const char* command_name,
    ulk_command_response_v1* response
)
{
    ulk_command_request_v1 request;
    memset(&request, 0, sizeof(request));
    memset(response, 0, sizeof(*response));
    request.struct_size = sizeof(request);
    request.command_name = view(command_name);
    request.json_payload = view("{}");
    request.dry_run = 1;
    response->struct_size = sizeof(*response);
    return ulk_command_execute_v1(context, &request, response);
}

static int test_context_response_sources(void)
{
    ulk_context* context = 0;
    ulk_command_response_v1 borrowed;
    ulk_command_descriptor_v1 descriptor;
    ulk_owned_command_response_v1 builtin_owned;
    ulk_owned_command_response_v1 graph_owned;
    ulk_owned_command_response_v1 registered_owned;
    handler_fixture fixture;

    memset(&fixture, 0, sizeof(fixture));
    memcpy(fixture.payload, "{\"schema\":\"fixture.response.v1\"}", 33u);
    memcpy(fixture.message, "fixture error", 14u);
    memcpy(fixture.detail, "fixture detail", 15u);
    prepare_owned(&builtin_owned);
    prepare_owned(&graph_owned);
    prepare_owned(&registered_owned);

    CHECK(ulk_context_create_v1(0, &context) == ULK_STATUS_OK, 70);
    CHECK(execute_command(context, "diagnostics.run", &borrowed) == ULK_STATUS_OK, 71);
    CHECK(
        ulk_command_response_copy_owned_v1(&borrowed, 0, &builtin_owned) ==
            ULK_STATUS_OK,
        72);

    CHECK(
        execute_command(context, "command_graph.inspect", &borrowed) ==
            ULK_STATUS_OK,
        73);
    CHECK(
        ulk_command_response_copy_owned_v1(&borrowed, 0, &graph_owned) ==
            ULK_STATUS_OK,
        74);

    memset(&descriptor, 0, sizeof(descriptor));
    descriptor.struct_size = sizeof(descriptor);
    descriptor.command_name = view("fixture.owned");
    descriptor.effects_json = view("[\"none\"]");
    descriptor.user = &fixture;
    descriptor.handler = fixture_handler;
    CHECK(ulk_command_register_v1(context, &descriptor) == ULK_STATUS_OK, 75);
    CHECK(
        execute_command(context, "fixture.owned", &borrowed) == ULK_STATUS_ERROR,
        76);
    CHECK(
        ulk_command_response_copy_owned_v1(&borrowed, 0, &registered_owned) ==
            ULK_STATUS_OK,
        77);

    memset(&fixture, 'x', sizeof(fixture));
    CHECK(
        execute_command(context, "command_graph.inspect", &borrowed) ==
            ULK_STATUS_OK,
        78);
    ulk_context_destroy_v1(context);

    CHECK(
        view_contains(
            builtin_owned.response.json_payload,
            "ulk.diagnostic_report.v1"),
        79);
    CHECK(
        view_contains(graph_owned.response.json_payload, "ulk.command_graph.v1"),
        80);
    CHECK(
        view_contains(
            registered_owned.response.json_payload,
            "fixture.response.v1"),
        81);
    CHECK(
        view_equals(
            registered_owned.response.error.message,
            "fixture error",
            13u),
        82);
    CHECK(
        view_equals(
            registered_owned.response.error.detail,
            "fixture detail",
            14u),
        83);
    CHECK(registered_owned.response.error.code == 812, 84);

    ulk_owned_command_response_release_v1(&builtin_owned);
    ulk_owned_command_response_release_v1(&graph_owned);
    ulk_owned_command_response_release_v1(&registered_owned);
    return 0;
}

int main(void)
{
    int status;
    CHECK(ULK_API_VERSION_MAJOR == 1 && ULK_API_VERSION_MINOR == 7, 85);
    status = test_validation();
    if (status != 0) return status;
    status = test_default_copy_and_release();
    if (status != 0) return status;
    status = test_allocator_and_empty_views();
    if (status != 0) return status;
    status = test_invalid_allocator_and_destination();
    if (status != 0) return status;
    status = test_allocation_failure();
    if (status != 0) return status;
    status = test_budget_and_overflow();
    if (status != 0) return status;
    return test_context_response_sources();
}
