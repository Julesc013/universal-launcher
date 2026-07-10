#include "ulk/ulk_api.h"

#include <string.h>

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

static int ULK_CALL registered_handler(
    void* user,
    const ulk_command_request_v1* request,
    ulk_command_response_v1* response)
{
    static const char payload[] =
        "{\"schema\":\"ulk.command_response.v1\",\"status\":\"ok\",\"payload\":{\"schema\":\"fixture.registered.v1\"},\"error\":null}";
    (void)user;
    (void)request;
    response->status = ULK_STATUS_OK;
    response->json_payload = view_from_cstr(payload);
    response->error.struct_size = sizeof(response->error);
    response->error.code = ULK_STATUS_OK;
    return ULK_STATUS_OK;
}

static int run_command(
    ulk_context* context,
    const char* command_name,
    int dry_run,
    const char* required_fragment
)
{
    ulk_command_request_v1 request;
    ulk_command_response_v1 response;
    int status;

    memset(&request, 0, sizeof(request));
    memset(&response, 0, sizeof(response));
    response.struct_size = sizeof(response);
    request.struct_size = sizeof(request);
    request.command_name = view_from_cstr(command_name);
    request.json_payload = view_from_cstr("{}");
    request.dry_run = dry_run;

    status = ulk_command_execute_v1(context, &request, &response);
    if (status != ULK_STATUS_OK) {
        return 1;
    }
    if (response.status != ULK_STATUS_OK) {
        return 2;
    }
    if (!contains(response.json_payload, "\"status\":\"ok\"")) {
        return 3;
    }
    if (!contains(response.json_payload, required_fragment)) {
        return 4;
    }

    return 0;
}

int main(void)
{
    ulk_context* context = 0;
    ulk_command_request_v1 request;
    ulk_command_response_v1 response;
    int status;
    ulk_command_descriptor_v1 descriptor;
    ulk_allocator_v1 truncated_allocator;

    if (ulk_context_create_v1(0, &context) != ULK_STATUS_OK || context == 0) {
        return 10;
    }
    if (ulk_abi_version_v1() != ((ULK_API_VERSION_MAJOR << 16) | ULK_API_VERSION_MINOR)) {
        return 11;
    }

    memset(&descriptor, 0, sizeof(descriptor));
    descriptor.struct_size = sizeof(descriptor);
    descriptor.command_name = view_from_cstr("install_refs.scan");
    descriptor.effects_json = view_from_cstr("[\"workspace_read\"]");
    descriptor.handler = registered_handler;
    if (ulk_command_register_v1(context, &descriptor) != ULK_STATUS_OK) {
        return 12;
    }
    if (run_command(context, "install_refs.scan", 1, "\"schema\":\"fixture.registered.v1\"") != 0) {
        return 13;
    }

    if (run_command(context, "command_graph.inspect", 1, "\"command\":\"launch_plan.build\"") != 0) {
        return 20;
    }
    if (run_command(context, "product.inspect", 1, "\"schema\":\"ulk.product.v1\"") != 0) {
        return 21;
    }
    if (run_command(context, "install_refs.list", 1, "\"install_refs\":[]") != 0) {
        return 22;
    }
    if (run_command(context, "install_refs.inspect", 1, "\"status\":\"not_found\"") != 0) {
        return 23;
    }
    if (run_command(context, "instance.create", 1, "\"schema\":\"ulk.instance_create_plan.v1\"") != 0) {
        return 24;
    }
    memset(&request, 0, sizeof(request));
    memset(&response, 0, sizeof(response));
    response.struct_size = sizeof(response);
    request.struct_size = sizeof(request);
    request.command_name = view_from_cstr("instance.create");
    request.json_payload = view_from_cstr("{}");
    request.dry_run = 0;
    status = ulk_command_execute_v1(context, &request, &response);
    if (status != ULK_STATUS_ERROR ||
        response.status != ULK_STATUS_ERROR ||
        !contains(response.json_payload, "\"code\":\"product_binding_required\"")) {
        return 36;
    }
    if (run_command(context, "instance.list", 1, "\"instances\":[]") != 0) {
        return 25;
    }
    if (run_command(context, "instances.list", 1, "\"instances\":[]") != 0) {
        return 26;
    }
    if (run_command(context, "profiles.list", 1, "\"profiles\":[]") != 0) {
        return 27;
    }
    if (run_command(context, "account_refs.list", 1, "\"account_refs\":[]") != 0) {
        return 28;
    }
    if (run_command(context, "artifact_sets.list", 1, "\"artifact_sets\":[]") != 0) {
        return 29;
    }
    if (run_command(context, "launch_plan.build", 1, "\"dry_run\":true") != 0) {
        return 32;
    }
    if (run_command(context, "launch_plan.build", 1, "\"argv\":[]") != 0) {
        return 37;
    }
    if (run_command(context, "launch_plan.build", 1, "\"preflight\":[]") != 0) {
        return 38;
    }
    if (run_command(context, "launch_plan.build", 0, "\"dry_run\":false") != 0) {
        return 33;
    }
    if (run_command(context, "diagnostics.run", 1, "\"report_id\":\"ulk.diagnostic.minimal\"") != 0) {
        return 34;
    }
    if (run_command(context, "diagnostics.report", 1, "\"report_id\":\"ulk.diagnostic.minimal\"") != 0) {
        return 35;
    }

    memset(&request, 0, sizeof(request));
    memset(&response, 0, sizeof(response));
    response.struct_size = sizeof(response);
    request.struct_size = sizeof(request);
    request.command_name = view_from_cstr("missing.command");
    request.json_payload = view_from_cstr("{}");
    request.dry_run = 1;
    status = ulk_command_execute_v1(context, &request, &response);
    if (status != ULK_STATUS_UNSUPPORTED_VERSION ||
        response.status != ULK_STATUS_UNSUPPORTED_VERSION ||
        !contains(response.json_payload, "\"status\":\"unsupported\"") ||
        !contains(response.json_payload, "\"code\":\"unsupported_command\"") ||
        !contains(response.json_payload, "Command is not supported by the Universal Launcher command graph")) {
        return 30;
    }

    memset(&response, 0, sizeof(response));
    response.struct_size = sizeof(response);
    status = ulk_command_execute_v1(context, 0, &response);
    if (status != ULK_STATUS_INVALID_ARGUMENT ||
        response.status != ULK_STATUS_INVALID_ARGUMENT ||
        !contains(response.json_payload, "\"status\":\"invalid_argument\"") ||
        !contains(response.json_payload, "\"code\":\"invalid_argument\"") ||
        !contains(response.json_payload, "Universal Launcher command request is invalid")) {
        return 31;
    }

    memset(&response, 0, sizeof(response));
    response.struct_size = sizeof(response) - 1;
    status = ulk_command_execute_v1(context, &request, &response);
    if (status != ULK_STATUS_INVALID_ARGUMENT) {
        return 39;
    }

    ulk_context_destroy_v1(context);

    memset(&truncated_allocator, 0, sizeof(truncated_allocator));
    truncated_allocator.struct_size = sizeof(truncated_allocator) - 1;
    context = 0;
    if (ulk_context_create_v1(&truncated_allocator, &context) != ULK_STATUS_INVALID_ARGUMENT || context != 0) {
        return 40;
    }
    return 0;
}
