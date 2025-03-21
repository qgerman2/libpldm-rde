#include "libpldm/pldm_rde.h"
#include "libpldm/base.h"
#include <endian.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

LIBPLDM_ABI_STABLE
int encode_negotiate_redfish_parameters_resp(
    uint8_t instance_id, uint8_t completion_code,
    uint8_t device_concurrency_support, bitfield8_t device_capabilities_flags,
    bitfield16_t device_feature_support,
    uint32_t device_configuration_signature, const char *device_provider_name,
    enum pldm_rde_varstring_format name_format, struct pldm_msg *msg)
{
	if ((NULL == msg) || (NULL == device_provider_name)) {
		return PLDM_ERROR_INVALID_DATA;
	}
	struct pldm_header_info header = {0};
	header.msg_type = PLDM_RESPONSE;
	header.instance = instance_id;
	header.pldm_type = PLDM_RDE;
	header.command = PLDM_NEGOTIATE_REDFISH_PARAMETERS;
	uint8_t rc = pack_pldm_header(&header, &(msg->hdr));
	if (rc != PLDM_SUCCESS) {
		return rc;
	}
	struct pldm_rde_negotiate_redfish_parameters_resp *response =
	    (struct pldm_rde_negotiate_redfish_parameters_resp *)msg->payload;
	response->completion_code = completion_code;
	if (response->completion_code != PLDM_SUCCESS) {
		return PLDM_SUCCESS;
	}
	response->device_concurrency_support = device_concurrency_support;
	response->device_capabilities_flags.byte =
	    device_capabilities_flags.byte;
	response->device_feature_support.value =
	    htole16(device_feature_support.value);
	response->device_configuration_signature =
	    htole32(device_configuration_signature);
	response->device_provider_name.string_format = name_format;
	// length should include NULL terminator.
	response->device_provider_name.string_length_bytes =
	    strlen(device_provider_name) + 1;
	// Copy including NULL terminator.
	memcpy(response->device_provider_name.string_data, device_provider_name,
	       response->device_provider_name.string_length_bytes);
	return PLDM_SUCCESS;
}

LIBPLDM_ABI_STABLE
int encode_negotiate_redfish_parameters_req(uint8_t instance_id,
					    uint8_t concurrency_support,
					    bitfield16_t *feature_support,
					    struct pldm_msg *msg)
{
	if ((msg == NULL) || (feature_support == NULL) ||
	    (concurrency_support == 0)) {
		return PLDM_ERROR_INVALID_DATA;
	}
	struct pldm_header_info header = {0};
	header.instance = instance_id;
	header.pldm_type = PLDM_RDE;
	header.msg_type = PLDM_REQUEST;
	header.command = PLDM_NEGOTIATE_REDFISH_PARAMETERS;
	uint8_t rc = pack_pldm_header(&header, &(msg->hdr));
	if (rc != PLDM_SUCCESS) {
		return rc;
	}
	struct pldm_rde_negotiate_redfish_parameters_req *req =
	    (struct pldm_rde_negotiate_redfish_parameters_req *)msg->payload;
	req->mc_concurrency_support = concurrency_support;
	req->mc_feature_support.value = htole16(feature_support->value);
	return PLDM_SUCCESS;
}

LIBPLDM_ABI_STABLE
int decode_negotiate_redfish_parameters_req(const struct pldm_msg *msg,
					    size_t payload_length,
					    uint8_t *mc_concurrency_support,
					    bitfield16_t *mc_feature_support)
{
	if ((msg == NULL) || (mc_concurrency_support == NULL) ||
	    (mc_feature_support == NULL)) {
		return PLDM_ERROR_INVALID_DATA;
	}
	if (payload_length !=
	    sizeof(struct pldm_rde_negotiate_redfish_parameters_req)) {
		return PLDM_ERROR_INVALID_LENGTH;
	}
	struct pldm_rde_negotiate_redfish_parameters_req *request =
	    (struct pldm_rde_negotiate_redfish_parameters_req *)msg->payload;
	if (request->mc_concurrency_support == 0) {
		fprintf(stderr, "Concurrency support is 0\n");
		return PLDM_ERROR_INVALID_DATA;
	}
	*mc_concurrency_support = request->mc_concurrency_support;
	mc_feature_support->value = le16toh(request->mc_feature_support.value);
	return PLDM_SUCCESS;
}

LIBPLDM_ABI_STABLE
int encode_negotiate_medium_parameters_resp(
    uint8_t instance_id, uint8_t completion_code,
    uint32_t device_maximum_transfer_bytes, struct pldm_msg *msg)
{
	if (NULL == msg) {
		return PLDM_ERROR_INVALID_DATA;
	}
	struct pldm_header_info header = {0};
	header.msg_type = PLDM_RESPONSE;
	header.instance = instance_id;
	header.pldm_type = PLDM_RDE;
	header.command = PLDM_NEGOTIATE_MEDIUM_PARAMETERS;
	uint8_t rc = pack_pldm_header(&header, &(msg->hdr));
	if (rc != PLDM_SUCCESS) {
		return rc;
	}
	struct pldm_rde_negotiate_medium_parameters_resp *response =
	    (struct pldm_rde_negotiate_medium_parameters_resp *)msg->payload;
	response->completion_code = completion_code;
	if (response->completion_code != PLDM_SUCCESS) {
		return PLDM_SUCCESS;
	}
	response->device_maximum_transfer_chunk_size_bytes =
	    htole32(device_maximum_transfer_bytes);
	return PLDM_SUCCESS;
}

LIBPLDM_ABI_STABLE
int decode_negotiate_redfish_parameters_resp(
    const struct pldm_msg *msg, size_t payload_length, uint8_t *completion_code,
    struct pldm_rde_device_info *device)
{
	if ((msg == NULL) || (device == NULL) || (completion_code == NULL)) {
		return PLDM_ERROR_INVALID_DATA;
	}

	*completion_code = msg->payload[0];

	if (PLDM_SUCCESS != *completion_code) {
		fprintf(stderr,
			"Unsuccessful completion code received in neg. params: %x\n",
			(uint8_t)(*completion_code));
		return PLDM_SUCCESS;
	}

	if (payload_length < RDE_NEGOTIATE_REDFISH_PARAMETERS_RESP_BYTES) {
		return PLDM_ERROR_INVALID_LENGTH;
	}

	struct pldm_rde_negotiate_redfish_parameters_resp *response =
	    (struct pldm_rde_negotiate_redfish_parameters_resp *)msg->payload;

	device->device_concurrency = response->device_concurrency_support;
	device->device_capabilities_flag = response->device_capabilities_flags;
	device->device_configuration_signature =
	    le32toh(response->device_configuration_signature);
	device->device_feature_support.value =
	    le16toh(response->device_feature_support.value);
	device->device_provider_name = response->device_provider_name;
	return PLDM_SUCCESS;
}

LIBPLDM_ABI_STABLE
int decode_negotiate_medium_parameters_resp(
    const struct pldm_msg *msg, size_t payload_length, uint8_t *completion_code,
    uint32_t *device_maximum_transfer_bytes)
{
	if ((msg == NULL) || (device_maximum_transfer_bytes == NULL) ||
	    (completion_code == NULL)) {
		return PLDM_ERROR_INVALID_DATA;
	}

	*completion_code = msg->payload[0];

	if (PLDM_SUCCESS != *completion_code) {
        fprintf(stderr,
            "Unsuccessful completion code received in neg med params: %x\n",
            (uint8_t)(*completion_code));
		return PLDM_SUCCESS;
	}

	if (payload_length < RDE_NEGOTIATE_MEDIUM_PARAMETERS_RESP_BYTES) {
		return PLDM_ERROR_INVALID_LENGTH;
	}

	struct pldm_rde_negotiate_medium_parameters_resp *response =
	    (struct pldm_rde_negotiate_medium_parameters_resp *)msg->payload;

	*device_maximum_transfer_bytes =
	    le32toh(response->device_maximum_transfer_chunk_size_bytes);

	return PLDM_SUCCESS;
}

LIBPLDM_ABI_STABLE
int encode_negotiate_medium_parameters_req(uint8_t instance_id,
					   uint32_t maximum_transfer_size,
					   struct pldm_msg *msg)
{
	if (msg == NULL) {
		return PLDM_ERROR_INVALID_DATA;
	}
	struct pldm_header_info header = {0};
	header.instance = instance_id;
	header.pldm_type = PLDM_RDE;
	header.msg_type = PLDM_REQUEST;
	header.command = PLDM_NEGOTIATE_MEDIUM_PARAMETERS;
	uint8_t rc = pack_pldm_header(&header, &(msg->hdr));
	if (rc != PLDM_SUCCESS) {
		return rc;
	}
	struct pldm_rde_negotiate_medium_parameters_req *req =
	    (struct pldm_rde_negotiate_medium_parameters_req *)msg->payload;
	req->mc_maximum_transfer_chunk_size_bytes =
	    htole32(maximum_transfer_size);
	return PLDM_SUCCESS;
}

LIBPLDM_ABI_STABLE
int decode_negotiate_medium_parameters_req(const struct pldm_msg *msg,
					   size_t payload_length,
					   uint32_t *mc_maximum_transfer_size)
{
	if ((msg == NULL) || (mc_maximum_transfer_size == NULL)) {
		return PLDM_ERROR_INVALID_DATA;
	}
	if (payload_length !=
	    sizeof(struct pldm_rde_negotiate_medium_parameters_req)) {
		return PLDM_ERROR_INVALID_LENGTH;
	}
	struct pldm_rde_negotiate_medium_parameters_req *request =
	    (struct pldm_rde_negotiate_medium_parameters_req *)msg->payload;
	*mc_maximum_transfer_size =
	    le32toh(request->mc_maximum_transfer_chunk_size_bytes);
	if (*mc_maximum_transfer_size < PLDM_RDE_MIN_TRANSFER_SIZE_BYTES) {
		return PLDM_ERROR_INVALID_DATA;
	}
	return PLDM_SUCCESS;
}

LIBPLDM_ABI_STABLE
int encode_get_schema_dictionary_req(uint8_t instance_id, uint32_t resource_id,
				     uint8_t schema_class, struct pldm_msg *msg)
{
	if (msg == NULL) {
		return PLDM_ERROR_INVALID_DATA;
	}
	struct pldm_header_info header = {0};
	header.instance = instance_id;
	header.pldm_type = PLDM_RDE;
	header.msg_type = PLDM_REQUEST;
	header.command = PLDM_GET_SCHEMA_DICTIONARY;
	uint8_t rc = pack_pldm_header(&header, &(msg->hdr));
	if (rc != PLDM_SUCCESS) {
		return rc;
	}
	struct pldm_rde_get_schema_dictionary_req *req =
	    (struct pldm_rde_get_schema_dictionary_req *)msg->payload;
	req->resource_id = htole32(resource_id);
	req->requested_schema_class = schema_class;
	return PLDM_SUCCESS;
}

LIBPLDM_ABI_STABLE
int decode_get_schema_dictionary_req(const struct pldm_msg *msg,
				     size_t payload_length,
				     uint32_t *resource_id,
				     uint8_t *requested_schema_class)
{
	if ((msg == NULL) || (resource_id == NULL) ||
	    (requested_schema_class == NULL)) {
		return PLDM_ERROR_INVALID_DATA;
	}
	if (payload_length !=
	    sizeof(struct pldm_rde_get_schema_dictionary_req)) {
		return PLDM_ERROR_INVALID_LENGTH;
	}
	struct pldm_rde_get_schema_dictionary_req *request =
	    (struct pldm_rde_get_schema_dictionary_req *)msg->payload;
	*resource_id = le32toh(request->resource_id);
	*requested_schema_class = request->requested_schema_class;
	if (*requested_schema_class > PLDM_RDE_SCHEMA_REGISTRY) {
		return PLDM_ERROR_INVALID_DATA;
	}
	return PLDM_SUCCESS;
}

LIBPLDM_ABI_STABLE
int encode_get_schema_dictionary_resp(uint8_t instance_id,
				      uint8_t completion_code,
				      uint8_t dictionary_format,
				      uint32_t transfer_handle,
				      struct pldm_msg *msg)
{
	if (NULL == msg) {
		return PLDM_ERROR_INVALID_DATA;
	}
	struct pldm_header_info header = {0};
	header.msg_type = PLDM_RESPONSE;
	header.instance = instance_id;
	header.pldm_type = PLDM_RDE;
	header.command = PLDM_GET_SCHEMA_DICTIONARY;
	uint8_t rc = pack_pldm_header(&header, &(msg->hdr));
	if (rc != PLDM_SUCCESS) {
		return rc;
	}
	struct pldm_rde_get_schema_dictionary_resp *response =
	    (struct pldm_rde_get_schema_dictionary_resp *)msg->payload;
	response->completion_code = completion_code;
	if (response->completion_code != PLDM_SUCCESS) {
		return PLDM_SUCCESS;
	}
	response->dictionary_format = dictionary_format;
	response->transfer_handle = htole32(transfer_handle);
	return PLDM_SUCCESS;
}

LIBPLDM_ABI_STABLE
int decode_get_schema_dictionary_resp(const struct pldm_msg *msg,
				      size_t payload_length,
				      uint8_t *completion_code,
				      uint8_t *dictionary_format,
				      uint32_t *transfer_handle)
{
	if ((msg == NULL) || (dictionary_format == NULL) ||
	    (completion_code == NULL) || (transfer_handle == NULL)) {
		return PLDM_ERROR_INVALID_DATA;
	}

	*completion_code = msg->payload[0];

	if (PLDM_SUCCESS != *completion_code) {
        fprintf(stderr, "Unsuccessful completion code received in schema: %x\n",
            (uint8_t)(*completion_code));
		return PLDM_SUCCESS;
	}

	if (payload_length < RDE_GET_DICTIONARY_SCHEMA_RESP_BYTES) {
        fprintf(stderr, "Unsuccessful completion code received %x",
            (uint8_t)(*completion_code));
		return PLDM_ERROR_INVALID_LENGTH;
	}

	struct pldm_rde_get_schema_dictionary_resp *response =
	    (struct pldm_rde_get_schema_dictionary_resp *)msg->payload;
	*dictionary_format = response->dictionary_format;
	*transfer_handle = le32toh(response->transfer_handle);
	return PLDM_SUCCESS;
}

LIBPLDM_ABI_STABLE
int encode_rde_multipart_receive_req(uint8_t instance_id,
				     uint32_t data_transfer_handle,
				     uint16_t operation_id,
				     uint8_t transfer_operation,
				     struct pldm_msg *msg)
{
	if (msg == NULL) {
		return PLDM_ERROR_INVALID_DATA;
	}
	struct pldm_header_info header = {0};
	header.instance = instance_id;
	header.pldm_type = PLDM_RDE;
	header.msg_type = PLDM_REQUEST;
	header.command = PLDM_RDE_MULTIPART_RECEIVE;
	uint8_t rc = pack_pldm_header(&header, &(msg->hdr));
	if (rc != PLDM_SUCCESS) {
		return rc;
	}
	struct pldm_rde_multipart_receive_req *req =
	    (struct pldm_rde_multipart_receive_req *)msg->payload;
	req->data_transfer_handle = htole32(data_transfer_handle);
	req->operation_id = htole16(operation_id);
	req->transfer_operation = transfer_operation;
	return PLDM_SUCCESS;
}

LIBPLDM_ABI_STABLE
int decode_rde_multipart_receive_req(const struct pldm_msg *msg,
				     size_t payload_length,
				     uint32_t *data_transfer_handle,
				     uint16_t *operation_id,
				     uint8_t *transfer_operation)
{
	if ((msg == NULL) || (data_transfer_handle == NULL) ||
	    (operation_id == NULL) || (transfer_operation == NULL)) {
		return PLDM_ERROR_INVALID_DATA;
	}
	if (payload_length != sizeof(struct pldm_rde_multipart_receive_req)) {
		return PLDM_ERROR_INVALID_LENGTH;
	}
	struct pldm_rde_multipart_receive_req *request =
	    (struct pldm_rde_multipart_receive_req *)msg->payload;
	*data_transfer_handle = le32toh(request->data_transfer_handle);
	*operation_id = le16toh(request->operation_id);
	*transfer_operation = request->transfer_operation;
	if (*transfer_operation > PLDM_RDE_XFER_ABORT) {
		return PLDM_ERROR_INVALID_DATA;
	}
	return PLDM_SUCCESS;
}

LIBPLDM_ABI_STABLE
int encode_rde_multipart_receive_resp(
    uint8_t instance_id, uint8_t completion_code, uint8_t transfer_flag,
    uint32_t next_data_transfer_handle, uint32_t data_length_bytes,
    bool add_checksum, uint32_t checksum, const uint8_t *payload,
    struct pldm_msg *msg)
{
	if (NULL == msg) {
		return PLDM_ERROR_INVALID_DATA;
	}
	struct pldm_header_info header = {0};
	header.msg_type = PLDM_RESPONSE;
	header.instance = instance_id;
	header.pldm_type = PLDM_RDE;
	header.command = PLDM_RDE_MULTIPART_RECEIVE;
	uint8_t rc = pack_pldm_header(&header, &(msg->hdr));
	if (rc != PLDM_SUCCESS) {
		return rc;
	}
	struct pldm_rde_multipart_receive_resp *response =
	    (struct pldm_rde_multipart_receive_resp *)msg->payload;
	response->completion_code = completion_code;
	if (response->completion_code != PLDM_SUCCESS) {
		return PLDM_SUCCESS;
	}
	response->transfer_flag = transfer_flag;
	response->next_data_transfer_handle =
	    htole32(next_data_transfer_handle);
	memcpy(response->payload, (uint8_t *)payload, data_length_bytes);
	uint32_t tot_length = data_length_bytes;
	if (add_checksum) {
		tot_length += 4;
		checksum = htole32(checksum);
		memcpy(response->payload + data_length_bytes, &checksum,
		       sizeof(uint32_t));
	}
	response->data_length_bytes = htole32(tot_length);
	return PLDM_SUCCESS;
}

LIBPLDM_ABI_STABLE
int decode_rde_multipart_receive_resp(
    const struct pldm_msg *msg, size_t payload_length, uint8_t *completion_code,
    uint8_t *ret_transfer_flag, uint32_t *ret_data_transfer_handle,
    uint32_t *data_length_bytes, uint8_t **payload)
{
	if ((msg == NULL) || (completion_code == NULL) ||
	    (ret_transfer_flag == NULL) || (ret_data_transfer_handle == NULL) ||
	    (data_length_bytes == NULL) || (payload == NULL)) {
		return PLDM_ERROR_INVALID_DATA;
	}

	*completion_code = msg->payload[0];

	if (PLDM_SUCCESS != *completion_code) {
		fprintf(stderr,
            "Unsuccessful completion code received in multipart: %x\n",
            (uint8_t)(*completion_code));
		return PLDM_ERROR;
	}

	if (payload_length < RDE_MULTIPART_RECV_MINIMUM_RESP_BYTES) {
		fprintf(stderr,
			"Decoded successfully with failed payload length in multipart"
			" with payload length: %zu and expected: %d\n",
			payload_length, RDE_MULTIPART_RECV_MINIMUM_RESP_BYTES);
		return PLDM_ERROR_INVALID_LENGTH;
	}

	struct pldm_rde_multipart_receive_resp *response =
	    (struct pldm_rde_multipart_receive_resp *)msg->payload;
	*ret_transfer_flag = response->transfer_flag;
	*ret_data_transfer_handle =
	    le32toh(response->next_data_transfer_handle);
	*data_length_bytes = le32toh(response->data_length_bytes);
	*payload = &response->payload[0];
	return PLDM_SUCCESS;
}

LIBPLDM_ABI_STABLE
int encode_rde_operation_init_req(
    uint8_t instance_id, uint32_t resource_id, uint16_t operation_id,
    uint8_t operation_type,
    const union pldm_rde_operation_flags *operation_flags,
    uint32_t send_data_transfer_handle, uint8_t operation_locator_length,
    uint32_t request_payload_length, const uint8_t *operation_locator,
    uint8_t *request_payload, struct pldm_msg *msg)
{
	if ((msg == NULL) || (operation_flags == NULL)) {
		return PLDM_ERROR_INVALID_DATA;
	}
	if ((operation_locator_length > 0) && (operation_locator == NULL)) {
		return PLDM_ERROR_INVALID_DATA;
	}
	if ((request_payload_length > 0) && (request_payload == NULL)) {
		return PLDM_ERROR_INVALID_DATA;
	}
	struct pldm_header_info header = {0};
	header.instance = instance_id;
	header.pldm_type = PLDM_RDE;
	header.msg_type = PLDM_REQUEST;
	header.command = PLDM_RDE_OPERATION_INIT;
	uint8_t rc = pack_pldm_header(&header, &(msg->hdr));
	if (rc != PLDM_SUCCESS) {
		return rc;
	}
	struct pldm_rde_operation_init_req *req =
	    (struct pldm_rde_operation_init_req *)msg->payload;
	req->resource_id = htole32(resource_id);
	req->operation_id = htole16(operation_id);
	req->operation_type = operation_type;
	req->operation_flags.byte = operation_flags->byte;
	req->send_data_transfer_handle = htole32(send_data_transfer_handle);
	req->operation_locator_length = operation_locator_length;
	req->request_payload_length = htole32(request_payload_length);
	if (operation_locator_length > 0) {
		memcpy(req->var_data, (uint8_t *)operation_locator,
		       operation_locator_length);
	}
	if (request_payload_length > 0) {
		memcpy(req->var_data + operation_locator_length,
		       (uint8_t *)request_payload, request_payload_length);
		request_payload = NULL;
	}
	return PLDM_SUCCESS;
}

static bool pldm_rde_is_valid_mc_op_id(uint16_t operation_id)
{
	// Operation identifiers with the MSBit set are reserved for use by the
	// MC. Operation identifiers with the MSBit clear are reserved for use
	// by the RDE Device. The value 0x0000 is reserved to indicate no
	// Operation.
	if ((operation_id == 0) || ((operation_id >> 15) == 0)) {
		return false;
	}
	return true;
}

LIBPLDM_ABI_STABLE
int decode_rde_operation_init_req(
    const struct pldm_msg *msg, size_t payload_length, uint32_t *resource_id,
    uint16_t *operation_id, uint8_t *operation_type,
    union pldm_rde_operation_flags *operation_flags,
    uint32_t *send_data_transfer_handle, uint8_t *operation_locator_length,
    uint32_t *request_payload_length, uint8_t **operation_locator,
    uint8_t **request_payload)
{
	if ((msg == NULL) || (resource_id == NULL) || (operation_id == NULL) ||
	    (operation_type == NULL) || (operation_flags == NULL) ||
	    (send_data_transfer_handle == NULL) ||
	    (operation_locator_length == NULL) ||
	    (request_payload_length == NULL)) {
		return PLDM_ERROR_INVALID_DATA;
	}
	if (payload_length < PLDM_RDE_OPERATION_INIT_REQ_HDR_SIZE) {
		return PLDM_ERROR_INVALID_LENGTH;
	}
	struct pldm_rde_operation_init_req *request =
	    (struct pldm_rde_operation_init_req *)msg->payload;
	*operation_id = le16toh(request->operation_id);
	if (!pldm_rde_is_valid_mc_op_id(*operation_id)) {
		return PLDM_ERROR_INVALID_DATA;
	}
	*resource_id = le32toh(request->resource_id);
	*operation_type = request->operation_type;
	operation_flags->byte = request->operation_flags.byte;
	*send_data_transfer_handle =
	    le32toh(request->send_data_transfer_handle);
	*operation_locator_length = request->operation_locator_length;
	*request_payload_length = le32toh(request->request_payload_length);
	if (*operation_locator_length > 0) {
		*operation_locator = request->var_data;
	} else {
		*operation_locator = NULL;
	}
	if (*request_payload_length > 0) {
		*request_payload =
		    request->var_data + *operation_locator_length;
	} else {
		*request_payload = NULL;
	}
	return PLDM_SUCCESS;
}

LIBPLDM_ABI_STABLE
int encode_rde_operation_init_resp(
    uint8_t instance_id, uint8_t completion_code, uint8_t operation_status,
    uint8_t completion_percentage, uint32_t completion_time_seconds,
    const union pldm_rde_op_execution_flags *operation_execution_flags,
    uint32_t result_transfer_handle,
    const union pldm_rde_permission_flags *permission_flags,
    uint32_t response_payload_length,
    enum pldm_rde_varstring_format etag_format, const char *etag,
    const uint8_t *response_payload, struct pldm_msg *msg)
{
	if ((msg == NULL) || (operation_execution_flags == NULL) ||
	    (permission_flags == NULL)) {
		return PLDM_ERROR_INVALID_DATA;
	}
	if ((response_payload_length > 0) && (response_payload == NULL)) {
		return PLDM_ERROR_INVALID_DATA;
	}
	struct pldm_header_info header = {0};
	header.msg_type = PLDM_RESPONSE;
	header.instance = instance_id;
	header.pldm_type = PLDM_RDE;
	header.command = PLDM_RDE_OPERATION_INIT;
	uint8_t rc = pack_pldm_header(&header, &(msg->hdr));
	if (rc != PLDM_SUCCESS) {
		return rc;
	}
	struct pldm_rde_operation_init_resp *response =
	    (struct pldm_rde_operation_init_resp *)msg->payload;
	response->completion_code = completion_code;
	if (response->completion_code != PLDM_SUCCESS) {
		return PLDM_SUCCESS;
	}
	response->operation_status = operation_status;
	response->completion_percentage = completion_percentage;
	response->completion_time_seconds = htole32(completion_time_seconds);
	response->operation_execution_flags.byte =
	    operation_execution_flags->byte;
	response->result_transfer_handle = htole32(result_transfer_handle);
	response->permission_flags.byte = permission_flags->byte;
	response->response_payload_length = htole32(response_payload_length);
	struct pldm_rde_varstring *resp_etag =
	    (struct pldm_rde_varstring *)response->var_data;
	resp_etag->string_format = etag_format;
	// length should include NULL terminator.
	size_t etag_len_wo_null = strlen(etag);
	resp_etag->string_length_bytes = etag_len_wo_null + 1;
	// Copy including NULL terminator.
	memcpy(resp_etag->string_data, etag, resp_etag->string_length_bytes);
	// Copy the payload.
	if (response_payload_length > 0) {
		memcpy(response->var_data + sizeof(struct pldm_rde_varstring) +
			   etag_len_wo_null,
		       (uint8_t *)response_payload, response_payload_length);
	}
	return PLDM_SUCCESS;
}

LIBPLDM_ABI_STABLE
int decode_rde_operation_init_resp(
    const struct pldm_msg *msg, size_t payload_length, uint8_t *completion_code,
    uint8_t *completion_percentage, uint8_t *operation_status,
    uint32_t *completion_time_seconds, uint32_t *result_transfer_handle,
    uint32_t *response_payload_length,
    union pldm_rde_permission_flags **permission_flags,
    union pldm_rde_op_execution_flags **operation_execution_flags,
    struct pldm_rde_varstring **resp_etag, uint8_t **response_payload)
{
	if (msg == NULL) {
		return PLDM_ERROR_INVALID_DATA;
	}
	*completion_code = msg->payload[0];
	if (PLDM_SUCCESS != *completion_code) {
		fprintf(stderr, "Unsuccessful completion code received in op init: %x\n",
            (uint8_t)(*completion_code));
		return PLDM_ERROR;
	}

	if (payload_length < RDE_READ_OPERATION_INIT_MIN_BYTES) {
		fprintf(stderr,
			"Decoded successfully with failed payload length with payload "
			"length: %zu and expected: %d\n",
			payload_length, RDE_READ_OPERATION_INIT_MIN_BYTES);
		return PLDM_ERROR_INVALID_LENGTH;
	}

	struct pldm_rde_operation_init_resp *response =
	    (struct pldm_rde_operation_init_resp *)msg->payload;

	*operation_status = response->operation_status;
	*completion_percentage = response->completion_percentage;
	*completion_time_seconds = le32toh(response->completion_time_seconds);
	(*operation_execution_flags)->byte =
	    response->operation_execution_flags.byte;
	*result_transfer_handle = le32toh(response->result_transfer_handle);
	(*permission_flags)->byte = response->permission_flags.byte;
	*response_payload_length = le32toh(response->response_payload_length);

	*resp_etag = (struct pldm_rde_varstring *)response->var_data;

	if (*operation_status == PLDM_RDE_OPERATION_COMPLETED) {
		*response_payload = &(*resp_etag)->string_data[0] +
				    (*resp_etag)->string_length_bytes;
	}
	return 0;
}

LIBPLDM_ABI_STABLE
int encode_supply_custom_request_parameters_req(
    uint8_t instance_id, uint32_t resource_id, uint16_t operation_id,
    uint16_t link_expand, uint16_t collection_skip, uint16_t collection_top,
    uint16_t pagination_offset, struct pldm_msg *msg)
{
    struct pldm_header_info header = {0};
    header.instance = instance_id;
    header.pldm_type = PLDM_RDE;
    header.msg_type = PLDM_REQUEST;
    header.command = PLDM_SUPPLY_CUSTOM_REQUEST_PARAMETERS;
    uint8_t rc = pack_pldm_header(&header, &(msg->hdr));
    if (rc != PLDM_SUCCESS) {
        return rc;
    }

    struct pldm_supply_custom_request_parameters_req *req =
        (struct pldm_supply_custom_request_parameters_req *)msg->payload;
    req->resource_id = htole32(resource_id);
    req->operation_id = htole16(operation_id);
    req->link_expand = link_expand;
    req->collection_skip = collection_skip;
    req->collection_top = collection_top;
    req->pagination_offset = pagination_offset;
    req->etag_operation = PLDM_RDE_ETAG_IGNORE;
    req->etag_count = 0;

	return PLDM_SUCCESS;
}

LIBPLDM_ABI_STABLE
int encode_etags_in_supply_custom_request_parameters_req(
    pldm_rde_etag_operation etag_operation,
    uint8_t etag_count, enum pldm_rde_varstring_format *etag_formats, char **etags,
    uint8_t *end_of_etags_offset, struct pldm_msg *msg)
{
    if ((etag_count > 0) && (etags == NULL)) {
        return PLDM_ERROR_INVALID_DATA;
    }

    struct pldm_header_info header;
    uint8_t rc = unpack_pldm_header(&msg->hdr, &header);
    if (rc != PLDM_SUCCESS) {
        return rc;
    }
    if (header.pldm_type != PLDM_RDE ||
        header.msg_type != PLDM_REQUEST ||
        header.command != PLDM_SUPPLY_CUSTOM_REQUEST_PARAMETERS) {
            //Not PLDM_SUPPLY_CUSTOM_REQUEST_PARAMETERS req
            return PLDM_ERROR_INVALID_DATA;
    }

    struct pldm_supply_custom_request_parameters_req *req =
        (struct pldm_supply_custom_request_parameters_req *)msg->payload;

    req->etag_operation = etag_operation;
    req->etag_count = etag_count;

    uint8_t offset = 0;

    if( req->etag_operation != PLDM_RDE_ETAG_IGNORE) {
        for (uint8_t i = 0; i < etag_count; i++)
        {
            req->var_data[offset++] = etag_formats[i];
            uint8_t len = strlen(etags[i]) + 1;
            req->var_data[offset++] = len;
            memcpy(req->var_data + offset, etags[i], len);
            offset += len;
        }
    }

    // store the end of etags offset to encode headers.
    *end_of_etags_offset = offset;
  	return PLDM_SUCCESS;
}

LIBPLDM_ABI_STABLE
int encode_headers_in_supply_custom_request_parameters_req(
    uint8_t header_count, enum pldm_rde_varstring_format *hdrname_formats, char **hdrnames,
    enum pldm_rde_varstring_format *hdrparam_formats, char **hdrparams,
    const uint8_t *header_offset, struct pldm_msg *msg)
{
    if ((header_count > 0) &&
    (hdrnames == NULL || hdrparams == NULL )) {
        return PLDM_ERROR_INVALID_DATA;
    }
    struct pldm_header_info header;
    uint8_t rc = unpack_pldm_header(&msg->hdr, &header);
    if (rc != PLDM_SUCCESS) {
        return rc;
    }
    if (header.pldm_type != PLDM_RDE ||
        header.msg_type != PLDM_REQUEST ||
        header.command != PLDM_SUPPLY_CUSTOM_REQUEST_PARAMETERS) {
            //Not PLDM_SUPPLY_CUSTOM_REQUEST_PARAMETERS req
            return PLDM_ERROR_INVALID_DATA;
    }

    uint8_t offset = *header_offset;
    struct pldm_supply_custom_request_parameters_req *req =
        (struct pldm_supply_custom_request_parameters_req *)msg->payload;

    //header count
    req->var_data[offset++] = header_count;
    for (uint8_t i = 0; i < header_count; i++)
    {
        req->var_data[offset++] = hdrname_formats[i];
        uint8_t len = strlen(hdrnames[i]) + 1;
        req->var_data[offset++] = len;
        memcpy(req->var_data + offset, hdrnames[i], len);
        offset += len;
    }

    for (uint8_t i = 0; i < header_count; i++)
    {
        req->var_data[offset++] = hdrparam_formats[i];
        uint8_t len = strlen(hdrparams[i]) + 1;
        req->var_data[offset++] = len;
        memcpy(req->var_data + offset, hdrparams[i], len);
        offset += len;
    }
	return PLDM_SUCCESS;
}

LIBPLDM_ABI_STABLE
int encode_supply_custom_request_parameters_resp(
    uint8_t instance_id, uint8_t completion_code, uint8_t operation_status,
    uint8_t completion_percentage, uint32_t completion_time_seconds,
    const union pldm_rde_op_execution_flags *operation_execution_flags,
    uint32_t result_transfer_handle,
    const union pldm_rde_permission_flags *permission_flags,
    uint32_t response_payload_length,
    enum pldm_rde_varstring_format etag_format, char *etag,
    const uint8_t *response_payload, struct pldm_msg *msg)
{
    if ((msg == NULL) || (operation_execution_flags == NULL) ||
        (permission_flags == NULL) || (etag == NULL)) {
        return PLDM_ERROR_INVALID_DATA;
    }
    if ((response_payload_length > 0) && (response_payload == NULL)) {
        return PLDM_ERROR_INVALID_DATA;
    }
    struct pldm_header_info header = {0};
    header.msg_type = PLDM_RESPONSE;
    header.instance = instance_id;
    header.pldm_type = PLDM_RDE;
    header.command = PLDM_SUPPLY_CUSTOM_REQUEST_PARAMETERS;
    uint8_t rc = pack_pldm_header(&header, &(msg->hdr));
    if (rc != PLDM_SUCCESS) {
        return rc;
    }
    struct pldm_supply_custom_request_parameters_resp *response =
        (struct pldm_supply_custom_request_parameters_resp *)msg->payload;
    response->completion_code = completion_code;
    if (response->completion_code != PLDM_SUCCESS) {
        return PLDM_SUCCESS;
    }
    response->operation_status = operation_status;
    response->completion_percentage = completion_percentage;
    response->completion_time_seconds = htole32(completion_time_seconds);
    response->operation_execution_flags.byte =
        operation_execution_flags->byte;
    response->result_transfer_handle = htole32(result_transfer_handle);
    response->permission_flags.byte = permission_flags->byte;
    response->response_payload_length = htole32(response_payload_length);
    struct pldm_rde_varstring *resp_etag =
        (struct pldm_rde_varstring *)response->var_data;
    resp_etag->string_format = etag_format;
    resp_etag->string_length_bytes = strlen(etag) + 1;
    memcpy(resp_etag->string_data, etag, resp_etag->string_length_bytes);
    // Copy the payload.
    if (response_payload_length > 0) {
        memcpy(response->var_data + sizeof(struct pldm_rde_varstring) +
                resp_etag->string_length_bytes - 1,  (uint8_t *)response_payload, response_payload_length);
    }
    return PLDM_SUCCESS;
}

LIBPLDM_ABI_STABLE
int decode_supply_custom_request_parameters_resp(
    const struct pldm_msg *msg, size_t payload_length, uint8_t *completion_code,
    uint8_t *completion_percentage, uint8_t *operation_status,
    uint32_t *completion_time_seconds, uint32_t *result_transfer_handle,
    uint32_t *response_payload_length,
    union pldm_rde_permission_flags **permission_flags,
    union pldm_rde_op_execution_flags **operation_execution_flags,
    struct pldm_rde_varstring **resp_etag, uint8_t **response_payload)
{
    if (msg == NULL) {
        fprintf(stderr, "Invalid msg object\n");
        return PLDM_ERROR_INVALID_DATA;
    }
    *completion_code = msg->payload[0];
    if (PLDM_SUCCESS != *completion_code) {
        fprintf(stderr,
            "Unsuccessful completion code received in op status: %x\n",
            (uint8_t)(*completion_code));
        return PLDM_SUCCESS;
    }
    if (payload_length < RDE_READ_OPERATION_INIT_MIN_BYTES) {
        fprintf(stderr,
            "Decoded sucessfully with failed payload length\n");
        return PLDM_ERROR_INVALID_LENGTH;
    }
    struct pldm_supply_custom_request_parameters_resp *response =
        (struct pldm_supply_custom_request_parameters_resp *)msg->payload;

    *operation_status = response->operation_status;
    *completion_percentage = response->completion_percentage;
    *completion_time_seconds = le32toh(response->completion_time_seconds);
    (*operation_execution_flags)->byte =
        response->operation_execution_flags.byte;
    *result_transfer_handle = le32toh(response->result_transfer_handle);
    (*permission_flags)->byte = response->permission_flags.byte;

    *resp_etag = (struct pldm_rde_varstring *)response->var_data;
    if (*operation_status == PLDM_RDE_OPERATION_COMPLETED &&
    (*operation_execution_flags)->bits.have_result_payload) {
        *response_payload_length = le32toh(response->response_payload_length);
    } else {
        *response_payload_length = 0;
    }

    if ( *response_payload_length > 0 ) {
        *response_payload = &(*resp_etag)->string_data[0] +
            (*resp_etag)->string_length_bytes;
    }
    return 0;
}

LIBPLDM_ABI_STABLE
int encode_rde_operation_complete_req(uint8_t instance_id, uint32_t resource_id,
				      uint16_t operation_id,
				      struct pldm_msg *msg)
{
	if (msg == NULL) {
		return PLDM_ERROR_INVALID_DATA;
	}
	struct pldm_header_info header = {0};
	header.instance = instance_id;
	header.pldm_type = PLDM_RDE;
	header.msg_type = PLDM_REQUEST;
	header.command = PLDM_RDE_OPERATION_COMPLETE;
	uint8_t rc = pack_pldm_header(&header, &(msg->hdr));
	if (rc != PLDM_SUCCESS) {
		return rc;
	}
	struct pldm_rde_operation_complete_req *req =
	    (struct pldm_rde_operation_complete_req *)msg->payload;
	req->resource_id = htole32(resource_id);
	req->operation_id = htole16(operation_id);
	return PLDM_SUCCESS;
}

LIBPLDM_ABI_STABLE
int decode_rde_operation_complete_req(const struct pldm_msg *msg,
				      size_t payload_length,
				      uint32_t *resource_id,
				      uint16_t *operation_id)
{
	if ((msg == NULL) || (resource_id == NULL) || (operation_id == NULL)) {
		return PLDM_ERROR_INVALID_DATA;
	}
	if (payload_length < sizeof(struct pldm_rde_operation_complete_req)) {
		return PLDM_ERROR_INVALID_LENGTH;
	}
	struct pldm_rde_operation_complete_req *request =
	    (struct pldm_rde_operation_complete_req *)msg->payload;
	*resource_id = le32toh(request->resource_id);
	*operation_id = le16toh(request->operation_id);
	return PLDM_SUCCESS;
}

LIBPLDM_ABI_STABLE
int encode_rde_operation_complete_resp(uint8_t instance_id,
				       uint8_t completion_code,
				       struct pldm_msg *msg)
{
	if (msg == NULL) {
		return PLDM_ERROR_INVALID_DATA;
	}
	struct pldm_header_info header = {0};
	header.msg_type = PLDM_RESPONSE;
	header.instance = instance_id;
	header.pldm_type = PLDM_RDE;
	header.command = PLDM_RDE_OPERATION_COMPLETE;
	uint8_t rc = pack_pldm_header(&header, &(msg->hdr));
	if (rc != PLDM_SUCCESS) {
		return rc;
	}
	struct pldm_rde_operation_complete_resp *response =
	    (struct pldm_rde_operation_complete_resp *)msg->payload;
	response->completion_code = completion_code;
	return PLDM_SUCCESS;
}

LIBPLDM_ABI_STABLE
int decode_rde_operation_complete_resp(const struct pldm_msg *msg,
				       size_t payload_length,
				       uint8_t *completion_code)
{
	if (msg == NULL) {
		fprintf(stderr, "Invalid message object\n");
		return PLDM_ERROR_INVALID_DATA;
	}
	if (payload_length < 1) {
		fprintf(stderr, "Invalid payload length\n");
		return PLDM_ERROR_INVALID_LENGTH;
	}

	*completion_code = msg->payload[0];

	if (PLDM_SUCCESS != *completion_code) {
		fprintf(stderr,
            "Unsuccessful completion code received in op complete: %x\n",
            (uint8_t)(*completion_code));
		return PLDM_SUCCESS;
	}
	return PLDM_SUCCESS;
}

LIBPLDM_ABI_STABLE
int encode_rde_operation_enumerate_req(uint8_t instance_id,
				       struct pldm_msg *msg)
{
	if (msg == NULL) {
		return PLDM_ERROR_INVALID_DATA;
	}
	struct pldm_header_info header = {0};
	header.instance = instance_id;
	header.pldm_type = PLDM_RDE;
	header.msg_type = PLDM_REQUEST;
	header.command = PLDM_RDE_OPERATION_ENUMERATE;
	uint8_t rc = pack_pldm_header(&header, &(msg->hdr));
	if (rc != PLDM_SUCCESS) {
		return rc;
	}

	return PLDM_SUCCESS;
}

LIBPLDM_ABI_STABLE
int decode_rde_operation_enumerate_req(const struct pldm_msg *msg,
				       size_t payload_length)
{
	if (msg == NULL) {
		return PLDM_ERROR_INVALID_DATA;
	}

	// Enumerate operation should not have any request payload
	if (payload_length != 0) {
		return PLDM_ERROR_INVALID_LENGTH;
	}

	return PLDM_SUCCESS;
}

LIBPLDM_ABI_STABLE
int encode_rde_operation_enumerate_resp(uint8_t instance_id,
					uint8_t completion_code,
					struct pldm_msg *msg)
{
	if (msg == NULL) {
		return PLDM_ERROR_INVALID_DATA;
	}

	struct pldm_header_info header = {0};
	header.msg_type = PLDM_RESPONSE;
	header.instance = instance_id;
	header.pldm_type = PLDM_RDE;
	header.command = PLDM_RDE_OPERATION_ENUMERATE;
	uint8_t rc = pack_pldm_header(&header, &(msg->hdr));
	if (rc != PLDM_SUCCESS) {
		return rc;
	}
	struct pldm_rde_operation_enumerate_resp *response =
	    (struct pldm_rde_operation_enumerate_resp *)msg->payload;
	response->completion_code = completion_code;

	// Initially set the operation count to 0. Operation data can be added
	// by calling update_encoded_rde_operation_enumerate_resp API
	response->operation_count = 0;
	return PLDM_SUCCESS;
}

LIBPLDM_ABI_STABLE
int update_encoded_rde_operation_enumerate_resp(const uint32_t resource_id,
						const uint16_t operation_id,
						const uint8_t operation_type,
						struct pldm_msg *msg)
{
	if (msg == NULL) {
		return PLDM_ERROR_INVALID_DATA;
	}

	struct pldm_rde_operation_enumerate_operation_data operation_data = {0};
	operation_data.resource_id = htole32(resource_id);
	operation_data.operation_id = htole16(operation_id);
	operation_data.operation_type = operation_type;

	struct pldm_rde_operation_enumerate_resp *response =
	    (struct pldm_rde_operation_enumerate_resp *)msg->payload;

	uint16_t operation_count = le16toh(response->operation_count);
	memcpy(response->var_data +
		   (sizeof(struct pldm_rde_operation_enumerate_operation_data) *
		    operation_count),
	       (void *)&operation_data,
	       sizeof(struct pldm_rde_operation_enumerate_operation_data));

	response->operation_count = htole16(operation_count + 1);
	return PLDM_SUCCESS;
}

LIBPLDM_ABI_STABLE
int decode_rde_operation_enumerate_resp(
    const struct pldm_msg *msg, uint8_t *completion_code,
    uint16_t *operation_count,
    struct pldm_rde_operation_enumerate_operation_data *operation_data,
    uint16_t operation_data_array_size)
{
	if (msg == NULL) {
		fprintf(stderr, "Invalid msg object\n");
		return PLDM_ERROR_INVALID_DATA;
	}

	if ((completion_code == NULL) || (operation_count == NULL) ||
	    (operation_data == NULL)) {
		fprintf(stderr, "Invalid input pointers\n");
		return PLDM_ERROR_INVALID_DATA;
	}

	struct pldm_rde_operation_enumerate_resp *response =
	    (struct pldm_rde_operation_enumerate_resp *)msg->payload;

	*completion_code = response->completion_code;
	if (*completion_code != PLDM_SUCCESS) {
		fprintf(
		    stderr,
		    "Unsuccessful completion code received in op status: %x\n",
		    (uint8_t)(*completion_code));
		return PLDM_SUCCESS;
	}

	*operation_count = le16toh(response->operation_count);
	if (operation_data_array_size < *operation_count) {
		fprintf(stderr,
			"Input operation_data struct array too small\n");
		return PLDM_ERROR_INVALID_DATA;
	}

	for (uint16_t i = 0; i < *operation_count; i++) {
		struct pldm_rde_operation_enumerate_operation_data
		    native_operation_data;
		memcpy(
		    (void *)&native_operation_data,
		    response->var_data +
			i * sizeof(struct
				   pldm_rde_operation_enumerate_operation_data),
		    sizeof(struct pldm_rde_operation_enumerate_operation_data));
		operation_data[i].resource_id =
		    le32toh(native_operation_data.resource_id);
		operation_data[i].operation_id =
		    le16toh(native_operation_data.operation_id);
		operation_data[i].operation_type =
		    native_operation_data.operation_type;
	}

	return PLDM_SUCCESS;
}

LIBPLDM_ABI_STABLE
int encode_rde_operation_status_req(uint8_t instance_id, uint32_t resource_id,
				    uint16_t operation_id, struct pldm_msg *msg)
{
	if (msg == NULL) {
		return PLDM_ERROR_INVALID_DATA;
	}
	struct pldm_header_info header = {0};
	header.instance = instance_id;
	header.pldm_type = PLDM_RDE;
	header.msg_type = PLDM_REQUEST;
	header.command = PLDM_RDE_OPERATION_STATUS;
	uint8_t rc = pack_pldm_header(&header, &(msg->hdr));
	if (rc != PLDM_SUCCESS) {
		return rc;
	}
	struct pldm_rde_operation_status_req *req =
	    (struct pldm_rde_operation_status_req *)msg->payload;
	req->resource_id = htole32(resource_id);
	req->operation_id = htole16(operation_id);
	return PLDM_SUCCESS;
}

LIBPLDM_ABI_STABLE
int decode_rde_operation_status_req(const struct pldm_msg *msg,
				    size_t payload_length,
				    uint32_t *resource_id,
				    uint16_t *operation_id)
{
	if ((msg == NULL) || (resource_id == NULL) || (operation_id == NULL)) {
		return PLDM_ERROR_INVALID_DATA;
	}
	if (payload_length < sizeof(struct pldm_rde_operation_status_req)) {
		return PLDM_ERROR_INVALID_LENGTH;
	}
	struct pldm_rde_operation_status_req *request =
	    (struct pldm_rde_operation_status_req *)msg->payload;
	*resource_id = le32toh(request->resource_id);
	*operation_id = le16toh(request->operation_id);
	return PLDM_SUCCESS;
}

LIBPLDM_ABI_STABLE
int encode_rde_operation_status_resp(
    uint8_t instance_id, uint8_t completion_code, uint8_t operation_status,
    uint8_t completion_percentage, uint32_t completion_time_seconds,
    const union pldm_rde_op_execution_flags *operation_execution_flags,
    uint32_t result_transfer_handle,
    const union pldm_rde_permission_flags *permission_flags,
    uint32_t response_payload_length,
    enum pldm_rde_varstring_format etag_format, const char *etag,
    const uint8_t *response_payload, struct pldm_msg *msg)
{
	if ((msg == NULL) || (operation_execution_flags == NULL) ||
	    (permission_flags == NULL)) {
		return PLDM_ERROR_INVALID_DATA;
	}
	if ((response_payload_length > 0) && (response_payload == NULL)) {
		return PLDM_ERROR_INVALID_DATA;
	}
	struct pldm_header_info header = {0};
	header.msg_type = PLDM_RESPONSE;
	header.instance = instance_id;
	header.pldm_type = PLDM_RDE;
	header.command = PLDM_RDE_OPERATION_STATUS;
	uint8_t rc = pack_pldm_header(&header, &(msg->hdr));
	if (rc != PLDM_SUCCESS) {
		return rc;
	}
	struct pldm_rde_operation_status_resp *response =
	    (struct pldm_rde_operation_status_resp *)msg->payload;
	response->completion_code = completion_code;
	if (response->completion_code != PLDM_SUCCESS) {
		return PLDM_SUCCESS;
	}
	response->operation_status = operation_status;
	response->completion_percentage = completion_percentage;
	response->completion_time_seconds = htole32(completion_time_seconds);
	response->operation_execution_flags.byte =
	    operation_execution_flags->byte;
	response->result_transfer_handle = htole32(result_transfer_handle);
	response->permission_flags.byte = permission_flags->byte;
	response->response_payload_length = htole32(response_payload_length);
	struct pldm_rde_varstring *resp_etag =
	    (struct pldm_rde_varstring *)response->var_data;
	resp_etag->string_format = etag_format;
	// length should include NULL terminator.
	size_t etag_len_wo_null = strlen(etag);
	resp_etag->string_length_bytes = etag_len_wo_null + 1;
	// Copy including NULL terminator.
	memcpy(resp_etag->string_data, etag, resp_etag->string_length_bytes);
	// Copy the payload.
	if (response_payload_length > 0) {
		memcpy(response->var_data + sizeof(struct pldm_rde_varstring) +
			   etag_len_wo_null,
		       (uint8_t *)response_payload, response_payload_length);
	}
	return PLDM_SUCCESS;
}

LIBPLDM_ABI_STABLE
int decode_rde_operation_status_resp(
    const struct pldm_msg *msg, size_t payload_length, uint8_t *completion_code,
    uint8_t *completion_percentage, uint8_t *operation_status,
    uint32_t *completion_time_seconds, uint32_t *result_transfer_handle,
    uint32_t *response_payload_length,
    union pldm_rde_permission_flags **permission_flags,
    union pldm_rde_op_execution_flags **operation_execution_flags,
    struct pldm_rde_varstring **resp_etag, uint8_t **response_payload)
{
	if (msg == NULL) {
		fprintf(stderr, "Invalid msg object\n");
		return PLDM_ERROR_INVALID_DATA;
	}

	*completion_code = msg->payload[0];
	if (PLDM_SUCCESS != *completion_code) {
		fprintf(stderr,
            "Unsuccessful completion code received in op status: %x\n",
            (uint8_t)(*completion_code));
		return PLDM_SUCCESS;
	}

	if (payload_length < RDE_READ_OPERATION_INIT_MIN_BYTES) {
		fprintf(stderr,
			"Decoded sucessfully with failed payload length\n");
		return PLDM_ERROR_INVALID_LENGTH;
	}

	struct pldm_rde_operation_init_resp *response =
	    (struct pldm_rde_operation_init_resp *)msg->payload;

	*operation_status = response->operation_status;
	*completion_percentage = response->completion_percentage;
	*completion_time_seconds = le32toh(response->completion_time_seconds);
	(*operation_execution_flags)->byte =
	    response->operation_execution_flags.byte;
	*result_transfer_handle = le32toh(response->result_transfer_handle);
	(*permission_flags)->byte = response->permission_flags.byte;
	*response_payload_length = le32toh(response->response_payload_length);

	*resp_etag = (struct pldm_rde_varstring *)response->var_data;
	if (*operation_status == PLDM_RDE_OPERATION_COMPLETED) {
		*response_payload = &(*resp_etag)->string_data[0] +
				    (*resp_etag)->string_length_bytes;
	}
	return 0;
}

LIBPLDM_ABI_STABLE
int encode_rde_operation_kill_req(uint8_t instance_id, uint32_t resource_id,
				  uint16_t operation_id, struct pldm_msg *msg)
{
	if (msg == NULL) {
		return PLDM_ERROR_INVALID_DATA;
	}
	struct pldm_header_info header = { 0 };
	header.instance = instance_id;
	header.pldm_type = PLDM_RDE;
	header.msg_type = PLDM_REQUEST;
	header.command = PLDM_RDE_OPERATION_KILL;
	uint8_t rc = pack_pldm_header(&header, &(msg->hdr));
	if (rc != PLDM_SUCCESS) {
		return rc;
	}
	struct pldm_rde_operation_kill_req *req =
		(struct pldm_rde_operation_kill_req *)msg->payload;
	req->resource_id = htole32(resource_id);
	req->operation_id = htole16(operation_id);
	req->killflags.byte = 4;
	return PLDM_SUCCESS;
}
