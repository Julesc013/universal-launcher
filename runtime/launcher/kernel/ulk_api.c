#include "ulk/ulk_api.h"

#include <stdlib.h>
#include <string.h>

struct ulk_context {
    ulk_allocator_v1 allocator;
    int has_allocator;
};

typedef struct ulk_static_response {
    int status;
    const char* payload;
    const char* error_message;
} ulk_static_response;

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

static int ulk_string_equals(ulk_string_view value, const char* expected)
{
    ulk_size index;
    ulk_size expected_size;

    if (value.data == 0 || expected == 0) {
        return 0;
    }

    expected_size = (ulk_size)strlen(expected);
    if (value.size != expected_size) {
        return 0;
    }

    for (index = 0; index < expected_size; ++index) {
        if (value.data[index] != expected[index]) {
            return 0;
        }
    }

    return 1;
}

static void ulk_set_response(
    ulk_command_response_v1* response,
    int status,
    const char* payload,
    const char* error_message
)
{
    if (response == 0) {
        return;
    }

    memset(response, 0, sizeof(*response));
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

static ulk_static_response ulk_dispatch_command(const ulk_command_request_v1* request)
{
    static const char command_graph_payload[] =
        "{\"schema\":\"ulk.command_response.v1\",\"status\":\"ok\",\"payload\":{\"schema\":\"ulk.command_graph.v1\",\"commands\":[{\"command\":\"product.inspect\",\"request_schema\":\"ulk.command_request.v1\",\"response_schema\":\"ulk.command_response.v1\",\"dry_run\":true,\"effects\":[\"none\"]},{\"command\":\"install_refs.list\",\"request_schema\":\"ulk.command_request.v1\",\"response_schema\":\"ulk.command_response.v1\",\"dry_run\":true,\"effects\":[\"workspace_read\"]},{\"command\":\"install_refs.inspect\",\"request_schema\":\"ulk.command_request.v1\",\"response_schema\":\"ulk.command_response.v1\",\"dry_run\":true,\"effects\":[\"workspace_read\"]},{\"command\":\"instance.create\",\"request_schema\":\"ulk.command_request.v1\",\"response_schema\":\"ulk.command_response.v1\",\"dry_run\":true,\"effects\":[\"workspace_write\"]},{\"command\":\"instance.list\",\"request_schema\":\"ulk.command_request.v1\",\"response_schema\":\"ulk.command_response.v1\",\"dry_run\":true,\"effects\":[\"workspace_read\"]},{\"command\":\"launch_plan.build\",\"request_schema\":\"ulk.command_request.v1\",\"response_schema\":\"ulk.command_response.v1\",\"dry_run\":true,\"effects\":[\"workspace_read\"]},{\"command\":\"diagnostics.run\",\"request_schema\":\"ulk.command_request.v1\",\"response_schema\":\"ulk.command_response.v1\",\"dry_run\":true,\"effects\":[\"none\"]},{\"command\":\"profiles.list\",\"request_schema\":\"ulk.command_request.v1\",\"response_schema\":\"ulk.command_response.v1\",\"dry_run\":true,\"effects\":[\"workspace_read\"]},{\"command\":\"account_refs.list\",\"request_schema\":\"ulk.command_request.v1\",\"response_schema\":\"ulk.command_response.v1\",\"dry_run\":true,\"effects\":[\"workspace_read\"]},{\"command\":\"artifact_sets.list\",\"request_schema\":\"ulk.command_request.v1\",\"response_schema\":\"ulk.command_response.v1\",\"dry_run\":true,\"effects\":[\"workspace_read\"]},{\"command\":\"instances.list\",\"alias_for\":\"instance.list\",\"request_schema\":\"ulk.command_request.v1\",\"response_schema\":\"ulk.command_response.v1\",\"dry_run\":true,\"effects\":[\"workspace_read\"]},{\"command\":\"diagnostics.report\",\"alias_for\":\"diagnostics.run\",\"request_schema\":\"ulk.command_request.v1\",\"response_schema\":\"ulk.command_response.v1\",\"dry_run\":true,\"effects\":[\"none\"]}]},\"error\":null}";
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

    if (ulk_string_equals(request->command_name, "command_graph.inspect")) {
        result.payload = command_graph_payload;
        return result;
    }
    if (ulk_string_equals(request->command_name, "product.inspect")) {
        result.payload = product_payload;
        return result;
    }
    if (ulk_string_equals(request->command_name, "install_refs.list")) {
        result.payload = install_refs_payload;
        return result;
    }
    if (ulk_string_equals(request->command_name, "install_refs.inspect")) {
        result.payload = install_ref_inspect_payload;
        return result;
    }
    if (ulk_string_equals(request->command_name, "instance.create")) {
        if (request->dry_run) {
            result.payload = instance_create_preview_payload;
            return result;
        }
        result.status = ULK_STATUS_ERROR;
        result.payload = instance_create_execute_refusal_payload;
        result.error_message = instance_create_refusal_message;
        return result;
    }
    if (ulk_string_equals(request->command_name, "instance.list") ||
        ulk_string_equals(request->command_name, "instances.list")) {
        result.payload = instances_payload;
        return result;
    }
    if (ulk_string_equals(request->command_name, "profiles.list")) {
        result.payload = profiles_payload;
        return result;
    }
    if (ulk_string_equals(request->command_name, "account_refs.list")) {
        result.payload = account_refs_payload;
        return result;
    }
    if (ulk_string_equals(request->command_name, "artifact_sets.list")) {
        result.payload = artifact_sets_payload;
        return result;
    }
    if (ulk_string_equals(request->command_name, "launch_plan.build")) {
        result.payload = request->dry_run ? launch_plan_dry_run_payload : launch_plan_execute_payload;
        return result;
    }
    if (ulk_string_equals(request->command_name, "diagnostics.run") ||
        ulk_string_equals(request->command_name, "diagnostics.report")) {
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

    effective_allocator.struct_size = sizeof(effective_allocator);
    effective_allocator.user = 0;
    effective_allocator.alloc = ulk_default_alloc;
    effective_allocator.free = ulk_default_free;

    if (allocator != 0 && allocator->alloc != 0 && allocator->free != 0) {
        effective_allocator = *allocator;
    }

    context = (ulk_context*)effective_allocator.alloc(effective_allocator.user, (ulk_size)sizeof(*context));
    if (context == 0) {
        *out_context = 0;
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

    (void)context;

    if (request == 0 ||
        request->struct_size < (ulk_size)sizeof(*request) ||
        request->command_name.data == 0 ||
        request->command_name.size == 0) {
        ulk_set_response(response, ULK_STATUS_INVALID_ARGUMENT, invalid_payload, invalid_message);
        return ULK_STATUS_INVALID_ARGUMENT;
    }

    dispatched = ulk_dispatch_command(request);
    ulk_set_response(response, dispatched.status, dispatched.payload, dispatched.error_message);
    return dispatched.status;
}

void ulk_context_destroy_v1(ulk_context* context)
{
    ulk_allocator_v1 allocator;

    if (context == 0) {
        return;
    }

    allocator = context->allocator;
    if (context->has_allocator && allocator.free != 0) {
        allocator.free(allocator.user, context);
        return;
    }

    free(context);
}
