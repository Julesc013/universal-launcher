#include "ulk/ulk_api.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(condition, code) \
    do { \
        if (!(condition)) { \
            fprintf(stderr, "check failed at line %d (code %d)\n", __LINE__, code); \
            return code; \
        } \
    } while (0)

#define TEST_REGISTRY_STORAGE_BUDGET_BYTES (64u * 1024u)

typedef struct expected_descriptor_v1_layout {
    ulk_size struct_size;
    ulk_string_view command_name;
    ulk_string_view effects_json;
    void* user;
    ulk_command_handler_v1 handler;
} expected_descriptor_v1_layout;

typedef struct allocator_state {
    ulk_size allocations;
    ulk_size frees;
    ulk_size fail_after;
} allocator_state;

static ulk_string_view view_from_cstr(const char* text)
{
    ulk_string_view view;
    view.data = text;
    view.size = (ulk_size)strlen(text);
    return view;
}

static int contains(ulk_string_view haystack, const char* needle)
{
    ulk_size index;
    ulk_size needle_size;

    if (haystack.data == 0 || needle == 0) {
        return 0;
    }

    needle_size = (ulk_size)strlen(needle);
    if (needle_size == 0 || needle_size > haystack.size) {
        return 0;
    }

    for (index = 0; index + needle_size <= haystack.size; ++index) {
        if (memcmp(haystack.data + index, needle, (size_t)needle_size) == 0) {
            return 1;
        }
    }
    return 0;
}

static int count_fragment(ulk_string_view haystack, const char* needle)
{
    ulk_size index;
    ulk_size needle_size = (ulk_size)strlen(needle);
    int count = 0;
    if (needle_size == 0 || haystack.data == 0) {
        return 0;
    }
    for (index = 0; index + needle_size <= haystack.size; ++index) {
        if (memcmp(haystack.data + index, needle, (size_t)needle_size) == 0) {
            ++count;
            index += needle_size - 1;
        }
    }
    return count;
}

static int ULK_CALL registered_handler(
    void* user,
    const ulk_command_request_v1* request,
    ulk_command_response_v1* response
)
{
    static const char payload[] =
        "{\"schema\":\"ulk.command_response.v1\",\"status\":\"ok\","
        "\"payload\":{\"schema\":\"fixture.registered.v1\"},\"error\":null}";
    (void)user;
    (void)request;
    response->status = ULK_STATUS_OK;
    response->json_payload = view_from_cstr(payload);
    response->error.struct_size = sizeof(response->error);
    response->error.code = ULK_STATUS_OK;
    return ULK_STATUS_OK;
}

static int execute_command(
    ulk_context* context,
    const char* command_name,
    int dry_run,
    ulk_command_response_v1* response
)
{
    ulk_command_request_v1 request;
    memset(&request, 0, sizeof(request));
    memset(response, 0, sizeof(*response));
    request.struct_size = sizeof(request);
    request.command_name = view_from_cstr(command_name);
    request.json_payload = view_from_cstr("{}");
    request.dry_run = dry_run;
    response->struct_size = sizeof(*response);
    return ulk_command_execute_v1(context, &request, response);
}

static int run_command(
    ulk_context* context,
    const char* command_name,
    int dry_run,
    const char* required_fragment
)
{
    ulk_command_response_v1 response;
    int status = execute_command(context, command_name, dry_run, &response);
    if (status != ULK_STATUS_OK || response.status != ULK_STATUS_OK) {
        return 1;
    }
    if (!contains(response.json_payload, "\"status\":\"ok\"")) {
        return 2;
    }
    if (!contains(response.json_payload, required_fragment)) {
        return 3;
    }
    return 0;
}

static int command_is_unsupported(ulk_context* context, const char* command_name)
{
    ulk_command_response_v1 response;
    int status = execute_command(context, command_name, 1, &response);
    return status == ULK_STATUS_UNSUPPORTED_VERSION &&
        response.status == ULK_STATUS_UNSUPPORTED_VERSION &&
        contains(response.json_payload, "\"code\":\"unsupported_command\"");
}

static void fill_descriptor_v2(
    ulk_command_descriptor_v2* descriptor,
    const char* command_name
)
{
    memset(descriptor, 0, sizeof(*descriptor));
    descriptor->struct_size = sizeof(*descriptor);
    descriptor->command_name = view_from_cstr(command_name);
    descriptor->effects_json = view_from_cstr("[\"workspace_read\"]");
    descriptor->request_schema = view_from_cstr("fixture.request.v1");
    descriptor->response_schema = view_from_cstr("fixture.response.v1");
    descriptor->result_schema = view_from_cstr("fixture.result.v1");
    descriptor->refusal_schema = view_from_cstr("fixture.refusal.v1");
    descriptor->dry_run_behavior = view_from_cstr("always_preview");
    descriptor->availability = view_from_cstr("available");
    descriptor->owner = view_from_cstr("fixture.product");
    descriptor->binding = view_from_cstr("fixture.binding");
    descriptor->handler = registered_handler;
}

static void* counting_alloc(void* user, ulk_size size)
{
    allocator_state* state = (allocator_state*)user;
    void* result;
    if (state->fail_after != 0 && state->allocations >= state->fail_after) {
        return 0;
    }
    result = malloc((size_t)size);
    if (result != 0) {
        ++state->allocations;
    }
    return result;
}

static void counting_free(void* user, void* ptr)
{
    allocator_state* state = (allocator_state*)user;
    if (ptr != 0) {
        ++state->frees;
        free(ptr);
    }
}

static int test_abi_layout(void)
{
    CHECK(sizeof(ulk_command_descriptor_v1) == sizeof(expected_descriptor_v1_layout), 10);
    CHECK(
        offsetof(ulk_command_descriptor_v1, command_name) ==
        offsetof(expected_descriptor_v1_layout, command_name),
        11);
    CHECK(
        offsetof(ulk_command_descriptor_v1, effects_json) ==
        offsetof(expected_descriptor_v1_layout, effects_json),
        12);
    CHECK(
        offsetof(ulk_command_descriptor_v1, user) ==
        offsetof(expected_descriptor_v1_layout, user),
        13);
    CHECK(
        offsetof(ulk_command_descriptor_v1, handler) ==
        offsetof(expected_descriptor_v1_layout, handler),
        14);
    CHECK(
        ulk_abi_version_v1() ==
        ((ULK_API_VERSION_MAJOR << 16) | ULK_API_VERSION_MINOR),
        15);
    return 0;
}

static int test_registry_projection(void)
{
    ulk_context* context = 0;
    ulk_command_descriptor_v1 descriptor_v1;
    ulk_command_descriptor_v2 descriptor_v2;
    ulk_command_response_v1 response;
    char command_name[] = "fixture.echo";
    char owner[] = "fixture.product";
    int status;

    CHECK(ulk_context_create_v1(0, &context) == ULK_STATUS_OK && context != 0, 20);
    CHECK(
        run_command(context, "command_graph.inspect", 1, "\"command\":\"product.inspect\"") == 0,
        21);
    CHECK(execute_command(context, "command_graph.inspect", 1, &response) == ULK_STATUS_OK, 23);
    CHECK(contains(response.json_payload, "\"handler_status\":\"builtin\""), 24);
    CHECK(!contains(response.json_payload, "\"command\":\"install_refs.scan\""), 25);
    CHECK(!contains(response.json_payload, "\"command\":\"instances.list\""), 26);
    CHECK(!contains(response.json_payload, "\"command\":\"diagnostics.report\""), 27);
    CHECK(command_is_unsupported(context, "install_refs.scan"), 28);
    CHECK(command_is_unsupported(context, "instances.list"), 29);
    CHECK(command_is_unsupported(context, "diagnostics.report"), 30);

    memset(&descriptor_v1, 0, sizeof(descriptor_v1));
    descriptor_v1.struct_size = sizeof(descriptor_v1);
    descriptor_v1.command_name = view_from_cstr("install_refs.scan");
    descriptor_v1.effects_json = view_from_cstr("[\"workspace_read\"]");
    descriptor_v1.handler = registered_handler;
    CHECK(ulk_command_register_v1(context, &descriptor_v1) == ULK_STATUS_OK, 31);
    CHECK(ulk_command_register_v1(context, &descriptor_v1) == ULK_STATUS_INVALID_ARGUMENT, 32);
    CHECK(run_command(context, "install_refs.scan", 1, "fixture.registered.v1") == 0, 33);
    CHECK(execute_command(context, "command_graph.inspect", 1, &response) == ULK_STATUS_OK, 34);
    CHECK(contains(response.json_payload, "\"command\":\"install_refs.scan\""), 35);
    CHECK(contains(response.json_payload, "\"effects\":[\"workspace_read\"]"), 36);
    CHECK(contains(response.json_payload, "\"binding\":\"legacy.v1\""), 37);
    CHECK(contains(response.json_payload, "\"handler_status\":\"registered\""), 38);

    fill_descriptor_v2(&descriptor_v2, command_name);
    descriptor_v2.owner = view_from_cstr(owner);
    CHECK(ulk_command_register_v2(context, &descriptor_v2) == ULK_STATUS_OK, 39);
    command_name[0] = 'x';
    owner[0] = 'x';
    CHECK(run_command(context, "fixture.echo", 1, "fixture.registered.v1") == 0, 40);
    CHECK(execute_command(context, "command_graph.inspect", 1, &response) == ULK_STATUS_OK, 41);
    CHECK(contains(response.json_payload, "\"command\":\"fixture.echo\""), 42);
    CHECK(contains(response.json_payload, "\"request_schema\":\"fixture.request.v1\""), 43);
    CHECK(contains(response.json_payload, "\"response_schema\":\"fixture.response.v1\""), 44);
    CHECK(contains(response.json_payload, "\"result_schema\":\"fixture.result.v1\""), 45);
    CHECK(contains(response.json_payload, "\"refusal_schema\":\"fixture.refusal.v1\""), 46);
    CHECK(contains(response.json_payload, "\"dry_run_behavior\":\"always_preview\""), 47);
    CHECK(contains(response.json_payload, "\"availability\":\"available\""), 48);
    CHECK(contains(response.json_payload, "\"owner\":\"fixture.product\""), 49);
    CHECK(contains(response.json_payload, "\"binding\":\"fixture.binding\""), 50);
    CHECK(
        count_fragment(response.json_payload, "\"command\":\"fixture.echo\"") == 1,
        51);

    status = ulk_command_unregister_v1(context, view_from_cstr("fixture.echo"));
    CHECK(status == ULK_STATUS_OK, 52);
    CHECK(command_is_unsupported(context, "fixture.echo"), 53);
    CHECK(execute_command(context, "command_graph.inspect", 1, &response) == ULK_STATUS_OK, 54);
    CHECK(!contains(response.json_payload, "\"command\":\"fixture.echo\""), 55);
    CHECK(
        ulk_command_unregister_v1(context, view_from_cstr("fixture.echo")) ==
        ULK_STATUS_INVALID_ARGUMENT,
        56);

    fill_descriptor_v2(&descriptor_v2, "launch_plan.build");
    CHECK(ulk_command_register_v2(context, &descriptor_v2) == ULK_STATUS_OK, 57);
    CHECK(execute_command(context, "command_graph.inspect", 1, &response) == ULK_STATUS_OK, 58);
    CHECK(contains(response.json_payload, "\"owner\":\"fixture.product\""), 59);
    CHECK(
        count_fragment(response.json_payload, "\"command\":\"launch_plan.build\"") == 1,
        60);
    CHECK(
        ulk_command_unregister_v1(context, view_from_cstr("launch_plan.build")) ==
        ULK_STATUS_OK,
        61);
    CHECK(execute_command(context, "command_graph.inspect", 1, &response) == ULK_STATUS_OK, 62);
    CHECK(!contains(response.json_payload, "\"owner\":\"fixture.product\""), 63);
    CHECK(contains(response.json_payload, "\"handler_status\":\"builtin\""), 67);
    CHECK(run_command(context, "launch_plan.build", 1, "\"dry_run\":true") == 0, 64);

    CHECK(
        ulk_command_unregister_v1(context, view_from_cstr("install_refs.scan")) ==
        ULK_STATUS_OK,
        65);
    CHECK(command_is_unsupported(context, "install_refs.scan"), 66);
    ulk_context_destroy_v1(context);
    return 0;
}

static int test_builtin_dispatch(void)
{
    ulk_context* context = 0;
    ulk_command_response_v1 response;
    int status;

    CHECK(ulk_context_create_v1(0, &context) == ULK_STATUS_OK, 70);
    CHECK(run_command(context, "product.inspect", 1, "\"schema\":\"ulk.product.v1\"") == 0, 71);
    CHECK(run_command(context, "install_refs.list", 1, "\"install_refs\":[]") == 0, 72);
    CHECK(run_command(context, "install_refs.inspect", 1, "\"status\":\"not_found\"") == 0, 73);
    CHECK(run_command(context, "instance.create", 1, "ulk.instance_create_plan.v1") == 0, 74);
    status = execute_command(context, "instance.create", 0, &response);
    CHECK(status == ULK_STATUS_ERROR, 75);
    CHECK(contains(response.json_payload, "\"code\":\"product_binding_required\""), 76);
    CHECK(run_command(context, "instance.list", 1, "\"instances\":[]") == 0, 77);
    CHECK(run_command(context, "profiles.list", 1, "\"profiles\":[]") == 0, 78);
    CHECK(run_command(context, "account_refs.list", 1, "\"account_refs\":[]") == 0, 79);
    CHECK(run_command(context, "artifact_sets.list", 1, "\"artifact_sets\":[]") == 0, 80);
    CHECK(run_command(context, "launch_plan.build", 1, "\"argv\":[]") == 0, 81);
    CHECK(run_command(context, "diagnostics.run", 1, "ulk.diagnostic.minimal") == 0, 82);

    memset(&response, 0, sizeof(response));
    response.struct_size = sizeof(response);
    status = ulk_command_execute_v1(context, 0, &response);
    CHECK(status == ULK_STATUS_INVALID_ARGUMENT, 83);
    CHECK(contains(response.json_payload, "\"code\":\"invalid_argument\""), 84);
    ulk_context_destroy_v1(context);
    return 0;
}

static int test_registration_validation_and_capacity(void)
{
    ulk_context* context = 0;
    ulk_command_descriptor_v2 descriptor;
    ulk_command_descriptor_v1 descriptor_v1;
    ulk_command_response_v1 response;
    static const int milestones[] = {1, 32, 33, 64, 256};
    char name[48];
    int milestone_index;
    int index;

    CHECK(ulk_context_create_v1(0, &context) == ULK_STATUS_OK, 90);
    fill_descriptor_v2(&descriptor, "instances.list");
    CHECK(ulk_command_register_v2(context, &descriptor) == ULK_STATUS_INVALID_ARGUMENT, 91);
    fill_descriptor_v2(&descriptor, "Fixture.echo");
    CHECK(ulk_command_register_v2(context, &descriptor) == ULK_STATUS_INVALID_ARGUMENT, 92);
    fill_descriptor_v2(&descriptor, "fixture.echo");
    descriptor.effects_json = view_from_cstr("[\"workspace_read\"],\"bad\":true");
    CHECK(ulk_command_register_v2(context, &descriptor) == ULK_STATUS_INVALID_ARGUMENT, 93);
    fill_descriptor_v2(&descriptor, "fixture.echo");
    descriptor.struct_size = sizeof(descriptor) - 1;
    CHECK(ulk_command_register_v2(context, &descriptor) == ULK_STATUS_INVALID_ARGUMENT, 94);
    fill_descriptor_v2(&descriptor, "fixture.echo");
    descriptor.handler = 0;
    CHECK(ulk_command_register_v2(context, &descriptor) == ULK_STATUS_INVALID_ARGUMENT, 95);
    ulk_context_destroy_v1(context);

    for (milestone_index = 0;
         milestone_index < (int)(sizeof(milestones) / sizeof(milestones[0]));
         ++milestone_index) {
        CHECK(ulk_context_create_v1(0, &context) == ULK_STATUS_OK, 96);
        for (index = 0; index < milestones[milestone_index]; ++index) {
            snprintf(name, sizeof(name), "fixture.capacity_%03d", index);
            memset(&descriptor_v1, 0, sizeof(descriptor_v1));
            descriptor_v1.struct_size = sizeof(descriptor_v1);
            descriptor_v1.command_name = view_from_cstr(name);
            descriptor_v1.effects_json = view_from_cstr("[\"none\"]");
            descriptor_v1.handler = registered_handler;
            CHECK(ulk_command_register_v1(context, &descriptor_v1) == ULK_STATUS_OK, 97);
        }
        CHECK(execute_command(context, "command_graph.inspect", 1, &response) == ULK_STATUS_OK, 98);
        CHECK(contains(response.json_payload, "\"command\":\"fixture.capacity_000\""), 99);
        snprintf(name, sizeof(name), "\"command\":\"fixture.capacity_%03d\"", milestones[milestone_index] - 1);
        CHECK(contains(response.json_payload, name), 100);
        ulk_context_destroy_v1(context);
        context = 0;
    }

    CHECK(ulk_context_create_v1(0, &context) == ULK_STATUS_OK, 101);
    for (index = 0;; ++index) {
        int registration_status;
        snprintf(name, sizeof(name), "fixture.budget_%04d", index);
        memset(&descriptor_v1, 0, sizeof(descriptor_v1));
        descriptor_v1.struct_size = sizeof(descriptor_v1);
        descriptor_v1.command_name = view_from_cstr(name);
        descriptor_v1.effects_json = view_from_cstr("[\"none\"]");
        descriptor_v1.handler = registered_handler;
        registration_status = ulk_command_register_v1(context, &descriptor_v1);
        if (registration_status != ULK_STATUS_OK) {
            CHECK(registration_status == ULK_STATUS_INVALID_ARGUMENT, 102);
            break;
        }
        CHECK(index < (int)TEST_REGISTRY_STORAGE_BUDGET_BYTES, 103);
    }
    CHECK(index >= 256, 104);
    ulk_context_destroy_v1(context);
    return 0;
}

static int test_allocator_and_failure_cleanup(void)
{
    allocator_state state;
    ulk_allocator_v1 allocator;
    ulk_context* context = 0;
    ulk_command_descriptor_v2 descriptor;
    ulk_command_response_v1 response;

    memset(&state, 0, sizeof(state));
    memset(&allocator, 0, sizeof(allocator));
    allocator.struct_size = sizeof(allocator);
    allocator.user = &state;
    allocator.alloc = counting_alloc;
    allocator.free = counting_free;
    CHECK(ulk_context_create_v1(&allocator, &context) == ULK_STATUS_OK, 110);
    fill_descriptor_v2(&descriptor, "fixture.allocator");
    CHECK(ulk_command_register_v2(context, &descriptor) == ULK_STATUS_OK, 111);
    CHECK(execute_command(context, "command_graph.inspect", 1, &response) == ULK_STATUS_OK, 112);
    CHECK(contains(response.json_payload, "\"command\":\"fixture.allocator\""), 113);
    ulk_context_destroy_v1(context);
    CHECK(state.allocations == state.frees, 114);

    memset(&state, 0, sizeof(state));
    context = 0;
    CHECK(ulk_context_create_v1(&allocator, &context) == ULK_STATUS_OK, 115);
    state.fail_after = state.allocations;
    fill_descriptor_v2(&descriptor, "fixture.failure");
    CHECK(ulk_command_register_v2(context, &descriptor) == ULK_STATUS_ERROR, 116);
    ulk_context_destroy_v1(context);
    CHECK(state.allocations == state.frees, 117);

    memset(&allocator, 0, sizeof(allocator));
    allocator.struct_size = sizeof(allocator);
    allocator.user = &state;
    allocator.alloc = counting_alloc;
    context = 0;
    CHECK(ulk_context_create_v1(&allocator, &context) == ULK_STATUS_INVALID_ARGUMENT, 118);
    CHECK(context == 0, 119);
    return 0;
}

static int dump_graph(void)
{
    ulk_context* context = 0;
    ulk_command_descriptor_v2 descriptor;
    ulk_command_response_v1 response;
    if (ulk_context_create_v1(0, &context) != ULK_STATUS_OK) {
        return 120;
    }
    fill_descriptor_v2(&descriptor, "fixture.dump");
    if (ulk_command_register_v2(context, &descriptor) != ULK_STATUS_OK ||
        execute_command(context, "command_graph.inspect", 1, &response) != ULK_STATUS_OK) {
        ulk_context_destroy_v1(context);
        return 121;
    }
    if (fwrite(response.json_payload.data, 1, (size_t)response.json_payload.size, stdout) !=
        (size_t)response.json_payload.size) {
        ulk_context_destroy_v1(context);
        return 122;
    }
    ulk_context_destroy_v1(context);
    return 0;
}

int main(int argc, char** argv)
{
    int status;
    if (argc == 2 && strcmp(argv[1], "--dump-graph") == 0) {
        return dump_graph();
    }
    status = test_abi_layout();
    if (status != 0) {
        return status;
    }
    status = test_registry_projection();
    if (status != 0) {
        return status;
    }
    status = test_builtin_dispatch();
    if (status != 0) {
        return status;
    }
    status = test_registration_validation_and_capacity();
    if (status != 0) {
        return status;
    }
    return test_allocator_and_failure_cleanup();
}
