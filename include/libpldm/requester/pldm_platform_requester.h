#ifndef PLDM_PLATFORM_REQUESTER_H
#define PLDM_PLATFORM_REQUESTER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "libpldm/base.h"
#include "libpldm/platform.h"
#include "libpldm/requester/pldm_base_requester.h"

/**
 * @brief Platform Requester Error enums
 */
typedef enum platform_requester_return_codes {
  PLDM_PLATFORM_REQUESTER_SUCCESS = 0,
  PLDM_PLATFORM_REQUESTER_NOT_PLDM_PLATFORM_MSG = -1,
  PLDM_PLATFORM_REQUESTER_NOT_RESP_MSG = -2,
  PLDM_PLATFORM_REQUESTER_SEND_FAIL = -3,
  PLDM_PLATFORM_REQUESTER_RECV_FAIL = -4,
  PLDM_PLATFORM_REQUESTER_NO_NEXT_COMMAND_FOUND = -5,
  PLDM_PLATFORM_REQUESTER_ENCODING_REQUEST_FAILURE = -6,
  PLDM_PLATFORM_REQUESTER_DECODING_RESPONSE_FAILURE = -7,
  PLDM_PLATFORM_REQUESTER_IO_FAILURE = -8,
  PLDM_PLATFORM_CONTEXT_INITIALIZATION_ERROR = -9,
  PLDM_PLATFORM_CONTEXT_NOT_READY = -10,

} pldm_platform_requester_rc_t;

/**
 * @brief Platform Requester Status enums
 */
typedef enum platform_requester_status {
  PLDM_PLATFORM_REQUESTER_REQUEST_FAILED = -1,
  PLDM_PLATFORM_REQUESTER_READY_TO_PICK_NEXT_REQUEST = 0,
  PLDM_PLATFORM_REQUESTER_WAITING_FOR_RESPONSE = 1,
  PLDM_PLATFORM_REQUESTER_NO_PENDING_ACTION = 2
} platform_req_status_t;

/**
 * @brief Platform Requester context
 */
struct pldm_platform_requester_context {
  bool initialized;
  int next_command;
  uint8_t requester_status;
  void *operation_ctx;

  char device_name[8];
  int net_id;
  uint32_t negotiated_transfer_size;
};

/**
 * @brief Get PDR operation context
 */
struct pldm_platform_get_pdr_operation {
  uint32_t record_handle;
  uint32_t data_transfer_handle;
  uint8_t transfer_op_flag;
  uint16_t record_change_num;
  uint16_t request_count;
};

/**
 * @brief Initializes the context for PLDM Platform commands
 *
 * @param[inout] ctx - Platform requester context
 * @param[in] device_id - RDE Device ID String
 * @param[in] net_id - Network ID to distinguish between RDE Devices
 * @param[in] mc_transfer_size - Negotiated Transfer Size for the PLDM message
 *
 * @return pldm_platform_requester_rc_t (errno may be set)
 */
pldm_platform_requester_rc_t pldm_platform_init_context(
    struct pldm_platform_requester_context *ctx, const char *device_id,
    int net_id, uint32_t negotiated_transfer_size);

/**
 * @brief Initializes the Platform Operation context
 *
 * @param[inout] ctx - Platform requester context
 * @param[in] get_pdr_ctx - Platform Operation context
 *
 * @return pldm_platform_requester_rc_t (errno may be set)
 */
pldm_platform_requester_rc_t pldm_platform_init_get_pdr_operation_context(
    struct pldm_platform_requester_context *ctx,
    struct pldm_platform_get_pdr_operation *get_pdr_ctx);

/**
 * @brief Sets up the state machine for starting PDR discovery
 *
 * @param[inout] ctx - Platform requester context
 *
 * @return pldm_platform_requester_rc_t (errno may be set)
 */
pldm_platform_requester_rc_t pldm_platform_start_pdr_discovery(
    struct pldm_platform_requester_context *ctx);

/**
 * @brief Gets the next GetPDR request
 *
 * @param[inout] ctx - Platform requester context
 * @param[in] instance_id - Instance Id of the PLDM message
 * @param[inout] request - PLDM Request
 * @param[in] request_length - Max request length
 *
 * @return pldm_platform_requester_rc_t (errno may be set)
 */
pldm_platform_requester_rc_t pldm_platform_get_next_get_pdr_request(
    struct pldm_platform_requester_context *ctx, uint8_t instance_id,
    struct pldm_msg *request, size_t request_length);

/**
 * @brief Pushes the response values to the context of the PLDM_PLATFORM type
 * command that was executed and updates the command status. It alse sets
 * the next_command attribute of the context based on the last executed
 * command.
 *
 * @param[inout] ctx - Platform requester context
 * @param[inout] resp_msg - Platform PLDM message
 * @param[in] resp_size - Max response length
 * @param[in] expected_pdr_header_version - Expected PDR Header version
 * @param[inout] record_data - PDR record data
 * @param[in] record_data_length - PDR record data length
 * @param[out] is_record_data_complete - Flag to indicate if the PDR data
 * part received marks the complete PDR transfer
 *
 * @return pldm_platform_requester_rc_t (errno may be set)
 */
pldm_platform_requester_rc_t pldm_platform_push_get_pdr_response(
    struct pldm_platform_requester_context *ctx, void *resp_msg,
    size_t resp_size, uint8_t expected_pdr_header_version, uint8_t *record_data,
    size_t record_data_length, bool *is_record_data_complete);

/**
 * @brief Decodes the PDR record metadata and the first PDR resource data
 *
 * @param[in] record_data - PDR record data
 * @param[out] resource_id - Memory to store resource ID
 * @param[out] resource_flags - Memory to store resource flags
 * @param[out] containing_resource_id - Memory to store containing resource ID
 * @param[out] proposed_containing_resource_length - Memory to store containing
 * resource name length
 * @param[out] proposed_containing_resource_name - Memory to store containing
 * resource name
 * @param[out] resource_uri_length - Memory to store first resource uri length
 * @param[out] resource_uri - Memory to store the resource uri
 * @param[out] additional_resource_id_count - Memory to store the additional
 * resource count in the PDR
 *
 * @return pldm_platform_requester_rc_t (errno may be set)
 */
pldm_platform_requester_rc_t pldm_platform_decode_first_pdr_resource(
    uint8_t *record_data, uint32_t *resource_id, uint8_t *resource_flags,
    uint32_t *containing_resource_id,
    uint16_t *proposed_containing_resource_length,
    char **proposed_containing_resource_name, uint16_t *resource_uri_length,
    char **resource_uri, uint16_t *additional_resource_id_count);

/**
 * @brief Decodes the additional PDR resource data
 *
 * @param[in] record_data - PDR record data
 * @param[in] resource_index - Additional resource index
 * @param[out] resource_id - Memory to store resource ID
 * @param[out] resource_uri_length - Memory to store first resource uri length
 * @param[out] resource_uri - Memory to store the resource uri
 *
 * @return pldm_platform_requester_rc_t (errno may be set)
 */
pldm_platform_requester_rc_t pldm_platform_decode_additional_pdr_resource(
    uint8_t *record_data, uint16_t resource_index, uint32_t *resource_id,
    uint16_t *resource_uri_length, char **resource_uri);

#ifdef __cplusplus
}
#endif

#endif /* PLDM_RDE_REQUESTER_H */
