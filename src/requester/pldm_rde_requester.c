#include "libpldm/requester/pldm_rde_requester.h"

#include "libpldm/base.h"
#include "libpldm/pldm.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

LIBPLDM_ABI_STABLE
pldm_rde_requester_rc_t
free_op_context_after_dictionary_extraction(struct pldm_rde_requester_context *ctx)
{
	struct pdr_resource *current_pdr_resource = ctx->current_pdr_resource;

	if (current_pdr_resource != NULL)
	{
		fprintf(stderr, "Cleaning Dictionary PDR Object\n");
		free(current_pdr_resource);
	}
	return PLDM_RDE_REQUESTER_SUCCESS;
}

LIBPLDM_ABI_STABLE
pldm_rde_requester_rc_t
pldm_rde_init_context(const char *device_id, int net_id,
		      struct pldm_rde_requester_manager *manager,
		      uint8_t mc_concurrency, uint32_t mc_transfer_size,
		      bitfield16_t *mc_features,
		      struct pldm_rde_requester_context *(*alloc_requester_ctx)(
			  uint8_t number_of_ctx),
		      void (*free_requester_ctx)(void *ctx_memory))
{
	fprintf(stdout, "Initializing Context Manager...\n");
	if (manager == NULL) {
		fprintf(stderr, "Memory not allocated to context manager.\n");
		return PLDM_RDE_CONTEXT_INITIALIZATION_ERROR;
	}

	if ((device_id == NULL) || (strlen(device_id) == 0) ||
	    (strlen(device_id) > 8)) {
		fprintf(stderr, "Incorrect device id provided\n");
		return PLDM_RDE_CONTEXT_INITIALIZATION_ERROR;
	}

	if ((alloc_requester_ctx == NULL) || (free_requester_ctx == NULL)) {
		fprintf(stderr, "No callback functions for handling request "
				"contexts found.\n");
		return PLDM_RDE_CONTEXT_INITIALIZATION_ERROR;
	}

	manager->initialized = true;
	manager->mc_concurrency = mc_concurrency;
	manager->mc_transfer_size = mc_transfer_size;
	manager->mc_feature_support = mc_features;
	strcpy(manager->device_name, device_id);
	manager->net_id = net_id;
	manager->number_of_resources = 0;

	manager->ctx = alloc_requester_ctx(mc_concurrency);

	manager->free_requester_ctx = *free_requester_ctx;
	return PLDM_RDE_REQUESTER_SUCCESS;
}

LIBPLDM_ABI_STABLE
pldm_rde_requester_rc_t
pldm_rde_set_resources_in_context(
		      struct pldm_rde_requester_manager *manager,
		      uint8_t number_of_resources,
		      uint32_t *resource_id_address)
{
	fprintf(stdout, "Setting Resource IDs in Context Manager...\n");
	if (manager == NULL) {
		fprintf(stderr, "Memory not allocated to context manager.\n");
		return PLDM_RDE_CONTEXT_INITIALIZATION_ERROR;
	}

	if (resource_id_address == NULL)
	{
		fprintf(stderr, "Memory not allocated to resource id array.\n");
		return PLDM_RDE_CONTEXT_INITIALIZATION_ERROR;
	}

	for (int i = 0; i < number_of_resources; i++) {
		manager->resource_ids[i] = *resource_id_address;
		resource_id_address++;
	}
	manager->number_of_resources = number_of_resources;
	return PLDM_RDE_REQUESTER_SUCCESS;
}

LIBPLDM_ABI_STABLE
pldm_rde_requester_rc_t
pldm_rde_start_discovery(struct pldm_rde_requester_context *ctx)
{
	if (ctx->context_status == CONTEXT_BUSY) {
		return PLDM_RDE_CONTEXT_NOT_READY;
	}
	ctx->next_command = PLDM_NEGOTIATE_REDFISH_PARAMETERS;
	return PLDM_RDE_REQUESTER_SUCCESS;
}

LIBPLDM_ABI_STABLE
pldm_rde_requester_rc_t
pldm_rde_create_context(struct pldm_rde_requester_context *ctx)
{
	if (ctx == NULL) {
		return PLDM_RDE_CONTEXT_INITIALIZATION_ERROR;
	}
	ctx->context_status = CONTEXT_FREE;
	ctx->next_command = PLDM_RDE_REQUESTER_NO_NEXT_COMMAND_FOUND;
	ctx->requester_status = PLDM_RDE_REQUESTER_READY_TO_PICK_NEXT_REQUEST;
	return PLDM_RDE_REQUESTER_SUCCESS;
}

LIBPLDM_ABI_STABLE
pldm_rde_requester_rc_t pldm_rde_get_next_discovery_command(
    uint8_t instance_id, struct pldm_rde_requester_manager *manager,
    struct pldm_rde_requester_context *current_ctx, struct pldm_msg *request)
{
	if (!manager->initialized) {
		return PLDM_RDE_CONTEXT_INITIALIZATION_ERROR;
	}

	if (current_ctx->context_status == CONTEXT_BUSY) {
		return PLDM_RDE_CONTEXT_NOT_READY;
	}

	int rc = 0;
	switch (current_ctx->next_command) {
	case PLDM_NEGOTIATE_REDFISH_PARAMETERS: {
		rc = encode_negotiate_redfish_parameters_req(
		    instance_id, manager->mc_concurrency,
		    manager->mc_feature_support, request);
		break;
	}
	case PLDM_NEGOTIATE_MEDIUM_PARAMETERS: {
		rc = encode_negotiate_medium_parameters_req(
		    instance_id, manager->mc_transfer_size, request);
		break;
	}
	default:
		rc = PLDM_RDE_REQUESTER_NO_NEXT_COMMAND_FOUND;
	}
	if (rc) {
		fprintf(stderr, "Unable to encode request with rc: %d\n", rc);
		return PLDM_RDE_REQUESTER_ENCODING_REQUEST_FAILURE;
	}
	return PLDM_RDE_REQUESTER_SUCCESS;
}

LIBPLDM_ABI_STABLE
pldm_rde_requester_rc_t
pldm_rde_discovery_push_response(struct pldm_rde_requester_manager *manager,
				 struct pldm_rde_requester_context *ctx,
				 void *resp_msg, size_t resp_size)
{
	int rc = 0;
	switch (ctx->next_command) {
	case PLDM_NEGOTIATE_REDFISH_PARAMETERS: {
		uint8_t completion_code = DEFAULT_INIT;
		rc = decode_negotiate_redfish_parameters_resp(
		    resp_msg, resp_size - sizeof(struct pldm_msg_hdr),
		    &completion_code, &manager->device);
		if (rc || completion_code) {
			ctx->requester_status =
			    PLDM_RDE_REQUESTER_REQUEST_FAILED;
		}
		ctx->next_command = PLDM_NEGOTIATE_MEDIUM_PARAMETERS;
		ctx->context_status = CONTEXT_FREE;
		ctx->requester_status =
		    PLDM_RDE_REQUESTER_READY_TO_PICK_NEXT_REQUEST;
		return PLDM_RDE_REQUESTER_SUCCESS;
	}
	case PLDM_NEGOTIATE_MEDIUM_PARAMETERS: {
		uint8_t completion_code = DEFAULT_INIT;
		uint32_t max_device_transfer_size = DEFAULT_INIT;
		rc = decode_negotiate_medium_parameters_resp(
		    resp_msg, resp_size - sizeof(struct pldm_msg_hdr),
		    &completion_code, &max_device_transfer_size);
		if (rc || completion_code) {
			ctx->requester_status =
			    PLDM_RDE_REQUESTER_REQUEST_FAILED;
		}
		manager->device.device_maximum_transfer_chunk_size =
		    max_device_transfer_size;

		manager->negotiated_transfer_size =
		    max_device_transfer_size > manager->mc_transfer_size
			? manager->mc_transfer_size
			: max_device_transfer_size;

		ctx->next_command = 0;
		ctx->context_status = CONTEXT_FREE;
		ctx->requester_status = PLDM_RDE_REQUESTER_NO_PENDING_ACTION;
		return PLDM_RDE_REQUESTER_SUCCESS;
	}

	default:
		return PLDM_RDE_REQUESTER_NO_NEXT_COMMAND_FOUND;
	}
}

LIBPLDM_ABI_STABLE
pldm_rde_requester_rc_t
pldm_rde_init_get_dictionary_schema(struct pldm_rde_requester_context *ctx)
{
	if (ctx->context_status == CONTEXT_BUSY) {
		return PLDM_RDE_CONTEXT_INITIALIZATION_ERROR;
	}
	ctx->next_command = PLDM_GET_SCHEMA_DICTIONARY;
	struct pdr_resource *current_pdr_resource =
	    (struct pdr_resource *)malloc(sizeof(struct pdr_resource));
	ctx->current_pdr_resource = current_pdr_resource;
	// start with 0th index of resource id array
	current_pdr_resource->resource_id_index = 0;
	current_pdr_resource->schema_class = PLDM_RDE_SCHEMA_MAJOR;
	ctx->requester_status = PLDM_RDE_REQUESTER_READY_TO_PICK_NEXT_REQUEST;
	return PLDM_RDE_REQUESTER_SUCCESS;
}

LIBPLDM_ABI_STABLE
pldm_rde_requester_rc_t pldm_rde_get_next_dictionary_schema_command(
    uint8_t instance_id, struct pldm_rde_requester_manager *manager,
    struct pldm_rde_requester_context *current_ctx, struct pldm_msg *request)
{
	if (manager->number_of_resources == 0) {
		return PLDM_RDE_NO_PDR_RESOURCES_FOUND;
	}
	if (!manager->initialized) {
		return PLDM_RDE_CONTEXT_INITIALIZATION_ERROR;
	}

	if (current_ctx->context_status == CONTEXT_BUSY) {
		return PLDM_RDE_CONTEXT_NOT_READY;
	}

	int rc = 0;
	switch (current_ctx->next_command) {
	case PLDM_GET_SCHEMA_DICTIONARY: {
		uint32_t resource_id =
		    current_ctx->current_pdr_resource->resource_id_index;
		rc = encode_get_schema_dictionary_req(
		    instance_id, manager->resource_ids[resource_id],
		    PLDM_RDE_SCHEMA_MAJOR, request);
		break;
	}
	case PLDM_RDE_MULTIPART_RECEIVE:
		rc = encode_rde_multipart_receive_req(
		    instance_id,
		    current_ctx->current_pdr_resource->transfer_handle, 0x00,
		    current_ctx->current_pdr_resource->transfer_operation,
		    request);
		break;
	default:
		rc = PLDM_RDE_REQUESTER_NO_NEXT_COMMAND_FOUND;
		break;
	}
	if (rc) {
		fprintf(stderr, "Unable to encode request with rc: %d\n", rc);
		return PLDM_RDE_REQUESTER_ENCODING_REQUEST_FAILURE;
	}
	return rc;
}

int set_next_dictionary_index(struct pldm_rde_requester_manager *manager,
			      struct pldm_rde_requester_context *ctx)
{
	uint8_t new_rid_idx = ctx->current_pdr_resource->resource_id_index + 1;

	if (new_rid_idx == manager->number_of_resources) {
		fprintf(stdout,
			"Processed all resources for dictionaries: %x \n",
			(uint8_t)new_rid_idx);
		ctx->next_command =
		    PLDM_RDE_REQUESTER_READY_TO_PICK_NEXT_REQUEST;
		ctx->current_pdr_resource->transfer_operation =
		    PLDM_XFER_COMPLETE;
		ctx->requester_status = PLDM_RDE_REQUESTER_NO_PENDING_ACTION;
		ctx->context_status = CONTEXT_FREE;
		free_op_context_after_dictionary_extraction(ctx);
	} else {
		ctx->next_command = PLDM_GET_SCHEMA_DICTIONARY;
		ctx->current_pdr_resource->resource_id_index = new_rid_idx;
		ctx->current_pdr_resource->schema_class = PLDM_RDE_SCHEMA_MAJOR;
		ctx->requester_status =
		    PLDM_RDE_REQUESTER_READY_TO_PICK_NEXT_REQUEST;
	}
	return PLDM_RDE_REQUESTER_SUCCESS;
}

LIBPLDM_ABI_STABLE
pldm_rde_requester_rc_t pldm_rde_push_get_dictionary_response(
    struct pldm_rde_requester_manager *manager,
    struct pldm_rde_requester_context *ctx, void *resp_msg, size_t resp_size,
    callback_funct callback)
{
	int rc = 0;
	switch (ctx->next_command) {
	case PLDM_GET_SCHEMA_DICTIONARY: {
		uint8_t completion_code = DEFAULT_INIT;
		rc = decode_get_schema_dictionary_resp(
		    resp_msg, resp_size - sizeof(struct pldm_msg_hdr),
		    &completion_code,
		    &(ctx->current_pdr_resource->dictionary_format),
		    &(ctx->current_pdr_resource->transfer_handle));
		if (rc || completion_code) {
			ctx->context_status = CONTEXT_FREE;
			set_next_dictionary_index(manager, ctx);
			break;
		}

		ctx->next_command = PLDM_RDE_MULTIPART_RECEIVE;
		ctx->current_pdr_resource->transfer_operation =
		    PLDM_XFER_FIRST_PART;
		ctx->context_status = CONTEXT_FREE;
		ctx->requester_status =
		    PLDM_RDE_REQUESTER_READY_TO_PICK_NEXT_REQUEST;
		return PLDM_RDE_REQUESTER_SUCCESS;
	}
	case PLDM_RDE_MULTIPART_RECEIVE: {
		uint8_t completion_code = DEFAULT_INIT;
		uint8_t ret_transfer_flag = DEFAULT_INIT;
		uint8_t *payload = NULL;
		uint32_t ret_data_transfer_handle = DEFAULT_INIT;
		uint32_t data_length_bytes = DEFAULT_INIT;

		rc = decode_rde_multipart_receive_resp(
		    resp_msg, resp_size - sizeof(struct pldm_msg_hdr),
		    &completion_code, &ret_transfer_flag,
		    &ret_data_transfer_handle, &data_length_bytes, &payload);

		if (rc || completion_code) {
			ctx->context_status = CONTEXT_FREE;
			set_next_dictionary_index(manager, ctx);
			break;
		}
		if ((ret_transfer_flag == PLDM_RDE_START) ||
		    (ret_transfer_flag == PLDM_RDE_MIDDLE)) {
			// Call the callback method to send back response
			// payload to requester
			callback(manager, ctx, &payload, data_length_bytes,
				 false);
			ctx->next_command = PLDM_RDE_MULTIPART_RECEIVE;
			ctx->current_pdr_resource->transfer_operation =
			    PLDM_XFER_NEXT_PART;
			ctx->current_pdr_resource->transfer_handle =
			    ret_data_transfer_handle;
			ctx->requester_status =
			    PLDM_RDE_REQUESTER_READY_TO_PICK_NEXT_REQUEST;
		} else if ((ret_transfer_flag == PLDM_RDE_START_AND_END) ||
			   (ret_transfer_flag == PLDM_RDE_END)) {
			callback(manager, ctx, &payload, data_length_bytes,
				 true);
			// find the next resource id from the resource id array
			// if exists
			set_next_dictionary_index(manager, ctx);
		}
		ctx->context_status = CONTEXT_FREE;
		return PLDM_RDE_REQUESTER_SUCCESS;
	}
	}
	return rc;
}

LIBPLDM_ABI_STABLE
pldm_rde_requester_rc_t pldm_rde_init_rde_operation_context(
	struct pldm_rde_requester_context *ctx, uint8_t request_id,
	uint32_t resource_id, uint16_t operation_id, uint8_t operation_type,
	uint8_t op_flags_byte, struct rde_query_options *query_options,
    uint32_t send_data_transfer_handle, uint8_t operation_locator_length,
    uint32_t request_payload_length, uint8_t *operation_locator,
    uint8_t *request_payload)
{
	if (ctx == NULL || ctx->context_status == CONTEXT_BUSY) {
		return PLDM_RDE_CONTEXT_INITIALIZATION_ERROR;
	}
	ctx->next_command = PLDM_RDE_OPERATION_INIT;

	struct rde_operation *operation =
		(struct rde_operation *)malloc(sizeof(struct rde_operation));
	operation->request_id = request_id;
	operation->resource_id = resource_id;
	operation->operation_id = operation_id;
	operation->operation_type = operation_type;
	union pldm_rde_operation_flags opflags;
	opflags.byte = op_flags_byte;
	operation->operation_flags = opflags;

	if (operation_locator != NULL) {
		operation->operation_locator = operation_locator;
	}
	operation->operation_locator_length = operation_locator_length;
	operation->request_payload_length = request_payload_length;
	operation->send_data_transfer_handle = send_data_transfer_handle;
	operation->resp_permission_flags =
		(union pldm_rde_permission_flags *)malloc(
			sizeof(union pldm_rde_permission_flags));
	operation->resp_operation_flags =
		(union pldm_rde_op_execution_flags *)malloc(
			sizeof(union pldm_rde_op_execution_flags));
	if (request_payload != NULL) {
		operation->request_payload = request_payload;
	}
    operation->query_options = NULL;
    if (operation->operation_flags.bits.contains_custom_request_parameters &&
        query_options && query_options->expand == true) {
        // This is freed in free_rde_op_init_context in all success/error
        // conditions
        operation->query_options =
            (struct rde_query_options *) malloc(sizeof(struct rde_query_options));
        operation->query_options->expand = true;
        operation->query_options->skip_param = query_options->skip_param;
        operation->query_options->top_param = query_options->top_param;
        operation->query_options->etag_count = query_options->etag_count;
        operation->query_options->header_count = query_options->header_count;
        for (uint8_t i = 0; i < query_options->etag_count; i++) {
            operation->query_options->etags[i] = query_options->etags[i];
        }
        for (uint8_t i = 0; i < query_options->header_count; i++) {
            operation->query_options->hdrnames[i] = query_options->hdrnames[i];
            operation->query_options->hdrparams[i] = query_options->hdrparams[i];
        }
        operation->query_options->expand_levels = query_options->expand_levels;
    }
	ctx->operation_ctx = operation;
	ctx->requester_status = PLDM_RDE_REQUESTER_READY_TO_PICK_NEXT_REQUEST;
	return PLDM_RDE_REQUESTER_SUCCESS;
}

LIBPLDM_ABI_STABLE
pldm_rde_requester_rc_t
pldm_rde_get_next_rde_operation(uint8_t instance_id,
				struct pldm_rde_requester_manager *manager,
				struct pldm_rde_requester_context *current_ctx,
				struct pldm_msg *request)
{
	if (!manager->initialized) {
		return PLDM_RDE_CONTEXT_INITIALIZATION_ERROR;
	}
	if (manager->number_of_resources == 0) {
		return PLDM_RDE_NO_PDR_RESOURCES_FOUND;
	}

	if (current_ctx->context_status == CONTEXT_BUSY) {
		return PLDM_RDE_CONTEXT_NOT_READY;
	}

	int rc = 0;
	struct rde_operation *operation_ctx =
		(struct rde_operation *)current_ctx->operation_ctx;
	switch (current_ctx->next_command) {
	case PLDM_RDE_OPERATION_INIT: {
		rc = encode_rde_operation_init_req(
			instance_id, operation_ctx->resource_id,
			operation_ctx->operation_id,
			operation_ctx->operation_type,
			&(operation_ctx->operation_flags),
			operation_ctx->send_data_transfer_handle,
			operation_ctx->operation_locator_length,
			operation_ctx->request_payload_length,
			operation_ctx->operation_locator,
			operation_ctx->request_payload, request);
		break;
	}
    case PLDM_SUPPLY_CUSTOM_REQUEST_PARAMETERS: {
        if(operation_ctx->operation_flags.bits.contains_custom_request_parameters) {
            if (operation_ctx->query_options->expand) {

                rc = encode_supply_custom_request_parameters_req(
                    instance_id, operation_ctx->resource_id,
                    operation_ctx->operation_id,
                    operation_ctx->query_options->expand_levels,
                    operation_ctx->query_options->skip_param,
                    operation_ctx->query_options->top_param,
                    0, request);
                if (rc != PLDM_SUCCESS)
                {
                    fprintf(stderr, "Encoding of suppy custom req failed : %u\n", rc);
                    return rc;
                }

                uint8_t offset = 0;
                rc = encode_etags_in_supply_custom_request_parameters_req (
                    operation_ctx->query_options->etag_operation,
                    operation_ctx->query_options->etag_count,
                    operation_ctx->query_options->etag_formats,
                    operation_ctx->query_options->etags, &offset, request);
                if (rc != PLDM_SUCCESS)
                {
                    fprintf(stderr, "Encoding of suppy custom req failed : %u\n", rc);
                    return rc;
                }

                rc = encode_headers_in_supply_custom_request_parameters_req (
                    operation_ctx->query_options->header_count,
                    operation_ctx->query_options->hdrname_formats,
                    operation_ctx->query_options->hdrnames,
                    operation_ctx->query_options->hdrparam_formats,
                    operation_ctx->query_options->hdrparams, &offset, request);
                if (rc != PLDM_SUCCESS)
                {
                    fprintf(stderr, "Encoding of suppy custom req failed : %u\n", rc);
                    return rc;
                }
            }
        }
        break;
    }
	case PLDM_RDE_OPERATION_STATUS: {
		rc = encode_rde_operation_status_req(
			instance_id, operation_ctx->resource_id,
			operation_ctx->operation_id, request);
		break;
	}

	case PLDM_RDE_OPERATION_COMPLETE: {
		rc = encode_rde_operation_complete_req(
			instance_id, operation_ctx->resource_id,
			operation_ctx->operation_id, request);
		break;
	}
	case PLDM_RDE_MULTIPART_RECEIVE: {
		rc = encode_rde_multipart_receive_req(
			instance_id, operation_ctx->result_transfer_handle,
			operation_ctx->operation_id,
			operation_ctx->transfer_operation, request);
		break;
	}
	}
	return rc;
}

int set_next_rde_operation(struct pldm_rde_requester_manager **manager,
			   struct pldm_rde_requester_context *ctx,
			   callback_funct callback)
{
	struct rde_operation *operation_ctx =
		(struct rde_operation *)ctx->operation_ctx;

	int rc = PLDM_RDE_REQUESTER_SUCCESS;
	switch (operation_ctx->operation_status) {
    case PLDM_RDE_OPERATION_NEEDS_INPUT: {
        if (operation_ctx->operation_flags.bits.contains_custom_request_parameters)
        {
                ctx->next_command = PLDM_SUPPLY_CUSTOM_REQUEST_PARAMETERS;
        } else {
                ctx->next_command = PLDM_RDE_MULTIPART_SEND;
        }
        ctx->context_status = CONTEXT_CONTINUE;
        ctx->requester_status =
                PLDM_RDE_REQUESTER_READY_TO_PICK_NEXT_REQUEST;
        break;
    }
	case PLDM_RDE_OPERATION_RUNNING: {
		ctx->next_command = PLDM_RDE_OPERATION_STATUS;
		ctx->context_status = CONTEXT_CONTINUE;
		ctx->requester_status =
			PLDM_RDE_REQUESTER_READY_TO_PICK_NEXT_REQUEST;
		break;
	}
	case PLDM_RDE_OPERATION_TRIGGERED: {
		ctx->next_command = PLDM_RDE_OPERATION_STATUS;
		ctx->context_status = CONTEXT_CONTINUE;
		ctx->requester_status =
			PLDM_RDE_REQUESTER_READY_TO_PICK_NEXT_REQUEST;
		break;
	}
	case PLDM_RDE_OPERATION_HAVE_RESULTS: {
		ctx->next_command = PLDM_RDE_MULTIPART_RECEIVE;
		ctx->context_status = CONTEXT_CONTINUE;
		ctx->requester_status =
			PLDM_RDE_REQUESTER_READY_TO_PICK_NEXT_REQUEST;
		operation_ctx->transfer_operation = PLDM_XFER_FIRST_PART;
		break;
	}
	case PLDM_RDE_OPERATION_COMPLETED: {
		// skip the etag bytes and then call callback on the payloads
		callback(*manager, ctx, &(operation_ctx->response_data),
			 operation_ctx->resp_payload_length, false);
		ctx->next_command = PLDM_RDE_OPERATION_COMPLETE;
		ctx->context_status = CONTEXT_FREE;
		ctx->requester_status =
			PLDM_RDE_REQUESTER_READY_TO_PICK_NEXT_REQUEST;
		break;
	}
	case PLDM_RDE_OPERATION_ABANDONED: {
		ctx->next_command = PLDM_RDE_OPERATION_COMPLETE;
		ctx->context_status = CONTEXT_CONTINUE;
		ctx->requester_status =
			PLDM_RDE_REQUESTER_READY_TO_PICK_NEXT_REQUEST;
		break;
	}
	case PLDM_RDE_OPERATION_FAILED: {
		ctx->next_command = PLDM_RDE_OPERATION_COMPLETE;
		ctx->context_status = CONTEXT_CONTINUE;
		ctx->requester_status =
			PLDM_RDE_REQUESTER_READY_TO_PICK_NEXT_REQUEST;
		break;
	}
	default: {
		rc = PLDM_RDE_OPERATION_FAILED;
		fprintf(stderr, "Received default operation_status : %u\n",
			operation_ctx->operation_status);
	}
	}
	return rc;
}


LIBPLDM_ABI_STABLE
pldm_rde_requester_rc_t pldm_rde_push_read_operation_response(
	struct pldm_rde_requester_manager *manager,
	struct pldm_rde_requester_context *ctx, void *resp_msg,
	size_t resp_size, callback_funct callback)
{
	int rc = 0;
	struct rde_operation *operation_ctx =
		(struct rde_operation *)ctx->operation_ctx;
	IGNORE(operation_ctx);
	switch (ctx->next_command) {
	case PLDM_RDE_OPERATION_INIT: {
		uint8_t completion_code;
		rc = decode_rde_operation_init_resp(
			resp_msg, resp_size - sizeof(struct pldm_msg_hdr),
			&completion_code, &(operation_ctx->percentage_complete),
			&(operation_ctx->operation_status),
			&(operation_ctx->completion_time),
			&(operation_ctx->result_transfer_handle),
			&(operation_ctx->resp_payload_length),
			&(operation_ctx->resp_permission_flags),
			&(operation_ctx->resp_operation_flags),
			&(operation_ctx->resp_etag),
			&(operation_ctx->response_data));

		if (rc || completion_code) {
			// If operation init failed, then there is not need to
			// send the rest of the requests for the resource.
			ctx->requester_status =
				PLDM_RDE_REQUESTER_REQUEST_FAILED;
			ctx->next_command =
				PLDM_RDE_REQUESTER_NO_NEXT_COMMAND_FOUND;
			ctx->context_status = CONTEXT_FREE;
			break;
		}

		rc = set_next_rde_operation(&manager, ctx, callback);

		if (rc) {
			ctx->requester_status =
				PLDM_RDE_REQUESTER_REQUEST_FAILED;
			ctx->next_command = PLDM_RDE_OPERATION_COMPLETE;
		}
		ctx->requester_status =
			PLDM_RDE_REQUESTER_READY_TO_PICK_NEXT_REQUEST;
		return PLDM_RDE_REQUESTER_SUCCESS;
	}
    case PLDM_SUPPLY_CUSTOM_REQUEST_PARAMETERS: {
        uint8_t completion_code;
        rc = decode_supply_custom_request_parameters_resp(
                resp_msg, resp_size - sizeof(struct pldm_msg_hdr),
                &completion_code, &(operation_ctx->percentage_complete),
                &(operation_ctx->operation_status),
                &(operation_ctx->completion_time),
                &(operation_ctx->result_transfer_handle),
                &(operation_ctx->resp_payload_length),
                &(operation_ctx->resp_permission_flags),
                &(operation_ctx->resp_operation_flags),
                &(operation_ctx->resp_etag),
                &(operation_ctx->response_data));

        if (rc || completion_code) {
                // If operation init failed, then there is not need to
                // send the rest of the requests for the resource.
                ctx->requester_status =
                        PLDM_RDE_REQUESTER_REQUEST_FAILED;
                ctx->next_command =
                        PLDM_RDE_REQUESTER_NO_NEXT_COMMAND_FOUND;
                ctx->context_status = CONTEXT_FREE;
                break;
        }

        rc = set_next_rde_operation(&manager, ctx, callback);

        if (rc) {
                ctx->requester_status =
                        PLDM_RDE_REQUESTER_REQUEST_FAILED;
                ctx->next_command = PLDM_RDE_OPERATION_COMPLETE;
        }
        ctx->requester_status =
                PLDM_RDE_REQUESTER_READY_TO_PICK_NEXT_REQUEST;
        return PLDM_RDE_REQUESTER_SUCCESS;
    }
	case PLDM_RDE_OPERATION_STATUS: {
		// Same as operation init
		uint8_t completion_code;
		rc = decode_rde_operation_status_resp(
			resp_msg, resp_size - sizeof(struct pldm_msg_hdr),
			&completion_code, &(operation_ctx->percentage_complete),
			&(operation_ctx->operation_status),
			&(operation_ctx->completion_time),
			&(operation_ctx->result_transfer_handle),
			&(operation_ctx->resp_payload_length),
			&(operation_ctx->resp_permission_flags),
			&(operation_ctx->resp_operation_flags),
			&(operation_ctx->resp_etag),
			&(operation_ctx->response_data));

		if (rc || completion_code) {
			ctx->requester_status =
				PLDM_RDE_REQUESTER_REQUEST_FAILED;
			ctx->next_command = PLDM_RDE_OPERATION_COMPLETE;
			ctx->context_status = CONTEXT_FREE;
			break;
		}

		rc = set_next_rde_operation(&manager, ctx, callback);
		if (rc) {
			ctx->requester_status =
				PLDM_RDE_REQUESTER_REQUEST_FAILED;
			ctx->next_command = PLDM_RDE_OPERATION_COMPLETE;
		}
		ctx->requester_status =
			PLDM_RDE_REQUESTER_READY_TO_PICK_NEXT_REQUEST;
		return PLDM_RDE_REQUESTER_SUCCESS;
	}

	case PLDM_RDE_OPERATION_COMPLETE: {
		rc = decode_rde_operation_complete_resp(
			resp_msg, resp_size - sizeof(struct pldm_msg_hdr),
			&(operation_ctx->completion_code));
		ctx->next_command = PLDM_RDE_REQUESTER_NO_NEXT_COMMAND_FOUND;
		ctx->context_status = CONTEXT_FREE;
		ctx->requester_status = PLDM_RDE_REQUESTER_NO_PENDING_ACTION;

		break;
	}
	case PLDM_RDE_MULTIPART_RECEIVE: {
		uint8_t completion_code, ret_transfer_flag;
		uint8_t *payload = NULL;
		uint32_t ret_data_transfer_handle, data_length_bytes;

		rc = decode_rde_multipart_receive_resp(
			resp_msg, resp_size - sizeof(struct pldm_msg_hdr),
			&completion_code, &ret_transfer_flag,
			&ret_data_transfer_handle, &data_length_bytes,
			&payload);
		if (rc || completion_code) {
			ctx->requester_status =
				PLDM_RDE_REQUESTER_REQUEST_FAILED;
			ctx->next_command = PLDM_RDE_OPERATION_COMPLETE;
			ctx->context_status = CONTEXT_FREE;
			break;
		}

		if (ret_transfer_flag == PLDM_RDE_START ||
		    ret_transfer_flag == PLDM_RDE_MIDDLE) {
			// next command is MULTIPART
			callback(manager, ctx, &payload, data_length_bytes,
				 false);
			ctx->next_command = PLDM_RDE_MULTIPART_RECEIVE;
			operation_ctx->transfer_operation = PLDM_XFER_NEXT_PART;
			operation_ctx->result_transfer_handle =
				ret_data_transfer_handle;
		} else if (ret_transfer_flag == PLDM_RDE_START_AND_END ||
			   ret_transfer_flag == PLDM_RDE_END) {
			// next command is Opertaion Complete
			callback(manager, ctx, &payload, data_length_bytes,
				 true);
			ctx->next_command = PLDM_RDE_OPERATION_COMPLETE;
		}
		ctx->context_status = CONTEXT_CONTINUE;
		ctx->requester_status =
			PLDM_RDE_REQUESTER_READY_TO_PICK_NEXT_REQUEST;
		break;
	}
	}

	return rc;
}

LIBPLDM_ABI_STABLE
pldm_rde_requester_rc_t
free_rde_op_init_context(struct pldm_rde_requester_context *ctx)
{
	struct rde_operation *operation =
		(struct rde_operation *)ctx->operation_ctx;

	if (operation != NULL) {
		if (operation->resp_permission_flags != NULL) {
			free(operation->resp_permission_flags);
			operation->resp_permission_flags = NULL;
		}

		if (operation->resp_operation_flags != NULL) {
			free(operation->resp_operation_flags);
			operation->resp_operation_flags = NULL;
		}

		if (operation->query_options != NULL) {
			free(operation->query_options);
			operation->query_options = NULL;
		}

		free(operation);
		ctx->operation_ctx = NULL;
	}
	return PLDM_RDE_REQUESTER_SUCCESS;
}

/**
 * =============== Workaround begins for b/293742455 ===================
 */
LIBPLDM_ABI_STABLE
pldm_rde_requester_rc_t get_pldm_rde_operation_complete_request(
    uint8_t instance_id, uint32_t resource_id,
    uint16_t operation_id, struct pldm_msg *request)
{
	return encode_rde_operation_complete_req(
	    instance_id, resource_id,
	    operation_id, request);
}

LIBPLDM_ABI_STABLE
pldm_rde_requester_rc_t get_pldm_rde_operation_enumerate_request(
    uint8_t instance_id, struct pldm_msg *request)
{
	return encode_rde_operation_enumerate_req(
	    instance_id, request);
}

LIBPLDM_ABI_STABLE
pldm_rde_requester_rc_t get_pldm_rde_operation_enumerate_response(
    struct pldm_msg *response, uint8_t *completion_code,
    uint16_t *operation_count,
    struct pldm_rde_operation_enumerate_operation_data *operation_data,
    uint16_t operation_data_array_size)
{
	return decode_rde_operation_enumerate_resp(
	    response, completion_code, operation_count,
		operation_data, operation_data_array_size);
}

LIBPLDM_ABI_STABLE
pldm_rde_requester_rc_t get_pldm_rde_operation_kill_request(
    uint8_t instance_id, uint32_t resource_id, uint16_t operation_id,
    struct pldm_msg *request)
{
	return encode_rde_operation_kill_req(
	    instance_id, resource_id,
	    operation_id, request);
}
/**
 * =============== Workaround ends ===================
 */
