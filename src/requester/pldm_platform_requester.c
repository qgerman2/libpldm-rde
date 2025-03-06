#include "libpldm/requester/pldm_platform_requester.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "libpldm/base.h"
#include "libpldm/pldm.h"

LIBPLDM_ABI_STABLE
pldm_platform_requester_rc_t pldm_platform_init_context(
    struct pldm_platform_requester_context* ctx, const char* device_id,
    int net_id, uint32_t negotiated_transfer_size) {
  if (ctx->initialized) {
    fprintf(stderr, "No memory allocated for platform context\n");
    return PLDM_PLATFORM_CONTEXT_INITIALIZATION_ERROR;
  }

  ctx->initialized = true;

  ctx->requester_status = PLDM_PLATFORM_REQUESTER_NO_PENDING_ACTION;
  strcpy(ctx->device_name, device_id);
  ctx->net_id = net_id;
  ctx->negotiated_transfer_size = negotiated_transfer_size;
  return PLDM_PLATFORM_REQUESTER_SUCCESS;
}

LIBPLDM_ABI_STABLE
pldm_platform_requester_rc_t pldm_platform_init_get_pdr_operation_context(
    struct pldm_platform_requester_context* ctx,
    struct pldm_platform_get_pdr_operation* get_pdr_ctx) {
  if ((ctx == NULL) || (get_pdr_ctx == NULL)) {
    return PLDM_PLATFORM_CONTEXT_INITIALIZATION_ERROR;
  }
  ctx->next_command = -1;

  get_pdr_ctx->record_handle = 0x00000000;
  get_pdr_ctx->data_transfer_handle = 0x00000000;
  get_pdr_ctx->transfer_op_flag = PLDM_GET_FIRSTPART;
  get_pdr_ctx->record_change_num = 0x0000;
  get_pdr_ctx->request_count =
      ctx->negotiated_transfer_size - sizeof(struct pldm_get_pdr_resp) - 1;

  ctx->operation_ctx = get_pdr_ctx;

  return PLDM_PLATFORM_REQUESTER_SUCCESS;
}

LIBPLDM_ABI_STABLE
pldm_platform_requester_rc_t pldm_platform_start_pdr_discovery(
    struct pldm_platform_requester_context* ctx) {
  if (!ctx->initialized &&
      ctx->requester_status != PLDM_PLATFORM_REQUESTER_NO_PENDING_ACTION) {
    return PLDM_PLATFORM_CONTEXT_NOT_READY;
  }

  // Set the default first command as PLDM_GET_PDR
  ctx->next_command = PLDM_GET_PDR;

  ctx->requester_status = PLDM_PLATFORM_REQUESTER_READY_TO_PICK_NEXT_REQUEST;
  return PLDM_PLATFORM_REQUESTER_SUCCESS;
}

LIBPLDM_ABI_STABLE
pldm_platform_requester_rc_t pldm_platform_get_next_get_pdr_request(
    struct pldm_platform_requester_context* ctx, uint8_t instance_id,
    struct pldm_msg* request, size_t request_length) {
  if ((ctx == NULL) || (request == NULL)) {
    fprintf(stderr, "Inputs cannot be NULL\n");
    return PLDM_PLATFORM_REQUESTER_IO_FAILURE;
  }

  int rc;
  switch (ctx->next_command) {
    case PLDM_GET_PDR: {
      struct pldm_platform_get_pdr_operation* get_pdr_ctx = ctx->operation_ctx;
      rc = encode_get_pdr_req(
          instance_id, get_pdr_ctx->record_handle,
          get_pdr_ctx->data_transfer_handle, get_pdr_ctx->transfer_op_flag,
          get_pdr_ctx->request_count, get_pdr_ctx->record_change_num, request,
          request_length);
      break;
    }
    default:
      return PLDM_PLATFORM_REQUESTER_NO_NEXT_COMMAND_FOUND;
  }

  if (rc) {
    fprintf(stderr, "Unable to encode request with rc: %d\n", rc);
    return PLDM_PLATFORM_REQUESTER_ENCODING_REQUEST_FAILURE;
  }
  return PLDM_PLATFORM_REQUESTER_SUCCESS;
}

LIBPLDM_ABI_STABLE
pldm_platform_requester_rc_t pldm_platform_push_get_pdr_response(
    struct pldm_platform_requester_context* ctx, void* resp_msg,
    size_t resp_size, uint8_t expected_pdr_header_version, uint8_t* record_data,
    size_t record_data_length, bool* is_record_data_complete) {
  switch (ctx->next_command) {
    case PLDM_GET_PDR: {
      struct pldm_platform_get_pdr_operation* get_pdr_ctx = ctx->operation_ctx;
      uint8_t completion_code;
      uint16_t response_count;
      uint8_t transfer_flag;
      uint32_t next_record_hndl;
      uint32_t next_data_transfer_hndl;
      uint8_t transfer_crc;

      int rc = decode_get_pdr_resp(
          resp_msg, resp_size - sizeof(struct pldm_msg_hdr), &completion_code,
          &next_record_hndl, &next_data_transfer_hndl, &transfer_flag,
          &response_count, record_data, record_data_length, &transfer_crc);
      if (rc || completion_code) {
        ctx->requester_status = PLDM_PLATFORM_REQUESTER_REQUEST_FAILED;
        fprintf(stderr,
                "Response decode failed with rc: %d, "
                "completion code: %d\n",
                rc, completion_code);
        return PLDM_PLATFORM_REQUESTER_DECODING_RESPONSE_FAILURE;
      }

      uint32_t resp_record_handle = 0;
      uint32_t resp_record_num = 0;
      uint16_t actual_pdr_byte_count = 0;
      rc = decode_pdr_header_and_get_record_data(
          record_data, response_count, expected_pdr_header_version,
          &resp_record_handle, &resp_record_num, &actual_pdr_byte_count);
      if (rc) {
        ctx->requester_status = PLDM_PLATFORM_REQUESTER_REQUEST_FAILED;
        fprintf(stderr,
                "Response decode failed with rc: %d, "
                "completion code: %d\n",
                rc, completion_code);
        return PLDM_PLATFORM_REQUESTER_DECODING_RESPONSE_FAILURE;
      }

      if ((sizeof(struct pldm_pdr_hdr) + actual_pdr_byte_count) !=
          (response_count)) {
        ctx->requester_status = PLDM_PLATFORM_REQUESTER_REQUEST_FAILED;
        fprintf(stderr,
                "Sum of actual PDR bytes and common PDR header does not match "
                "the Total PDR byte count in GetPDR response."
                "(Actual PDR bytes + common PDR header bytes) : %u, Total PDR "
                "bytes : %u\n",
                (uint32_t)(sizeof(struct pldm_pdr_hdr) + actual_pdr_byte_count),
                response_count);
        return PLDM_PLATFORM_REQUESTER_DECODING_RESPONSE_FAILURE;
      }

      if ((transfer_flag == PLDM_START) || (transfer_flag == PLDM_MIDDLE)) {
        get_pdr_ctx->transfer_op_flag = PLDM_GET_NEXTPART;
        get_pdr_ctx->data_transfer_handle = next_data_transfer_hndl;
        get_pdr_ctx->record_handle = next_record_hndl;
        ctx->next_command = PLDM_GET_PDR;
        *is_record_data_complete = false;
      } else {
        *is_record_data_complete = true;
        get_pdr_ctx->transfer_op_flag = PLDM_GET_FIRSTPART;
        get_pdr_ctx->data_transfer_handle = 0x00000000;
        // Check if there are more PDRs to be retrieved
        if (next_record_hndl != 0x00000000) {
          get_pdr_ctx->record_handle = next_record_hndl;
          ctx->next_command = PLDM_GET_PDR;
          ctx->requester_status =
              PLDM_PLATFORM_REQUESTER_READY_TO_PICK_NEXT_REQUEST;
        } else {
          get_pdr_ctx->record_handle = 0x00000000;
          ctx->next_command = -1;
          ctx->requester_status = PLDM_PLATFORM_REQUESTER_NO_PENDING_ACTION;
        }
      }

      get_pdr_ctx->record_change_num = resp_record_num;
      return PLDM_PLATFORM_REQUESTER_SUCCESS;
    }
    default:
      return PLDM_PLATFORM_REQUESTER_NO_NEXT_COMMAND_FOUND;
  }

  return PLDM_PLATFORM_REQUESTER_NOT_PLDM_PLATFORM_MSG;
}

LIBPLDM_ABI_STABLE
pldm_platform_requester_rc_t pldm_platform_decode_first_pdr_resource(
    uint8_t* record_data, uint32_t* resource_id, uint8_t* resource_flags,
    uint32_t* containing_resource_id,
    uint16_t* proposed_containing_resource_length,
    char** proposed_containing_resource_name, uint16_t* resource_uri_length,
    char** resource_uri, uint16_t* additional_resource_id_count) {
  if (get_redfish_pdr_from_decoded_get_pdr_resp(
          record_data, resource_id, resource_flags, containing_resource_id,
          proposed_containing_resource_length,
          proposed_containing_resource_name, resource_uri_length, resource_uri,
          additional_resource_id_count) != PLDM_SUCCESS) {
    return PLDM_PLATFORM_REQUESTER_DECODING_RESPONSE_FAILURE;
  }

  return PLDM_PLATFORM_REQUESTER_SUCCESS;
}

LIBPLDM_ABI_STABLE
pldm_platform_requester_rc_t pldm_platform_decode_additional_pdr_resource(
    uint8_t* record_data, uint16_t resource_index, uint32_t* resource_id,
    uint16_t* resource_uri_length, char** resource_uri) {
  if (get_additional_redfish_resource_from_decoded_get_pdr_resp(
          record_data, resource_index, resource_id, resource_uri_length,
          resource_uri) != PLDM_SUCCESS) {
    return PLDM_PLATFORM_REQUESTER_DECODING_RESPONSE_FAILURE;
  }

  return PLDM_PLATFORM_REQUESTER_SUCCESS;
}
