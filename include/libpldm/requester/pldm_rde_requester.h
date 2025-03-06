#ifndef PLDM_RDE_REQUESTER_H
#define PLDM_RDE_REQUESTER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "libpldm/base.h"
#include "libpldm/requester/pldm_base_requester.h"
#include "libpldm/pldm_rde.h"

// Currently RDE supports a maximum of 50 dictionary resources
#define MAX_RESOURCE_IDS 50

#define DEFAULT_INIT 0

#define MAX_HEADERS 8
#define MAX_ETAGS 8

typedef enum rde_requester_return_codes {
	PLDM_RDE_REQUESTER_SUCCESS = 0,
	PLDM_RDE_REQUESTER_NOT_PLDM_RDE_MSG = -1,
	PLDM_RDE_REQUESTER_NOT_RESP_MSG = -2,
	PLDM_RDE_REQUESTER_SEND_FAIL = -3,
	PLDM_RDE_REQUESTER_RECV_FAIL = -4,
	PLDM_RDE_REQUESTER_NO_NEXT_COMMAND_FOUND = -5,
	PLDM_RDE_REQUESTER_ENCODING_REQUEST_FAILURE = -6,
	PLDM_RDE_CONTEXT_INITIALIZATION_ERROR = -7,
	PLDM_RDE_CONTEXT_NOT_READY = -8,
	PLDM_RDE_NO_PDR_RESOURCES_FOUND = -9
} pldm_rde_requester_rc_t;

typedef enum rde_requester_status {
	PLDM_RDE_REQUESTER_REQUEST_FAILED = -1,
	PLDM_RDE_REQUESTER_READY_TO_PICK_NEXT_REQUEST = 0,
	PLDM_RDE_REQUESTER_WAITING_FOR_RESPONSE = 1,
	PLDM_RDE_REQUESTER_NO_PENDING_ACTION = 2
} rde_req_status_t;

typedef enum rde_context_status {
	CONTEXT_FREE = 0,
	CONTEXT_BUSY = 1,
	CONTEXT_CONTINUE = 2
} rde_context_status;

/**
 * @brief This will hold the PDR Resource information
 * This could be modified when the P&M PLDM Type is implemented
 */
struct pdr_resource {
	uint8_t resource_id_index;
	uint32_t transfer_handle;
	uint8_t dictionary_format;
	uint8_t transfer_operation;
	uint8_t schema_class;
};
/**
 * @brief The entire RDE Update operation is captured by the following struct
 */
struct rde_update_operation {
	uint8_t request_id;
	uint32_t resource_id;
	uint16_t operation_id;
	uint8_t operation_type;
	uint8_t operation_status;
	uint8_t percentage_complete;
	uint32_t completion_time;
	uint32_t result_transfer_handle;

	// Request Data
	union pldm_rde_operation_flags operation_flags;
	uint32_t send_data_transfer_handle;
	uint8_t operation_locator_length;
	uint8_t *operation_locator;
	uint32_t request_payload_length;
	uint8_t *request_payload;

	// Response Data
	uint32_t resp_payload_length;
	uint8_t *response_data;
	union pldm_rde_op_execution_flags *resp_operation_flags;
	union pldm_rde_permission_flags *resp_permission_flags;
	struct pldm_rde_varstring *resp_etag;

	// op complete
	uint8_t completion_code;
};

/**
 * @brief The entire RDE Read operation is captured by the following struct
 */
struct rde_read_operation {
	uint8_t request_id;
	uint32_t resource_id;
	uint16_t operation_id;
	uint8_t operation_type;
	uint8_t operation_status;
	uint8_t percentage_complete;
	uint32_t completion_time;
	uint32_t result_transfer_handle;

	// Request Data
	union pldm_rde_operation_flags operation_flags;
	uint32_t send_data_transfer_handle;
	uint8_t operation_locator_length;
	uint8_t *operation_locator;
	uint32_t request_payload_length;
	uint8_t *request_payload;

	// Response Data
	uint32_t resp_payload_length;
	uint8_t *response_data;
	union pldm_rde_op_execution_flags *resp_operation_flags;
	union pldm_rde_permission_flags *resp_permission_flags;
	struct pldm_rde_varstring *resp_etag;

	// For multipart receive
	uint32_t transfer_handle;
	uint8_t transfer_operation;

	// op complete
	uint8_t completion_code;
};

struct rde_query_options {
	bool skip;
	bool top;
	bool expand;
	uint16_t skip_param;
	uint16_t top_param;
	uint16_t expand_levels;
	uint8_t header_count;
	enum pldm_rde_varstring_format hdrname_formats[MAX_HEADERS];
	char *hdrnames[MAX_HEADERS];
	enum pldm_rde_varstring_format hdrparam_formats[MAX_HEADERS];
	char *hdrparams[MAX_HEADERS];
	uint8_t etag_operation;
	uint8_t etag_count;
	enum pldm_rde_varstring_format etag_formats[MAX_ETAGS];
	char *etags[MAX_ETAGS];
};

/**
 * @brief RDE operation
 */
struct rde_operation {
	uint8_t request_id;
	uint32_t resource_id;
	uint16_t operation_id;
	uint8_t operation_type;
	uint8_t operation_status;
	uint8_t percentage_complete;
	uint32_t completion_time;
	uint32_t result_transfer_handle;

	// Request Data
	union pldm_rde_operation_flags operation_flags;
	uint32_t send_data_transfer_handle;
	uint8_t operation_locator_length;
	uint8_t *operation_locator;
	uint32_t request_payload_length;
	uint8_t *request_payload;
	struct rde_query_options *query_options;

	// Response Data
	uint32_t resp_payload_length;
	uint8_t *response_data;
	union pldm_rde_op_execution_flags *resp_operation_flags;
	union pldm_rde_permission_flags *resp_permission_flags;
	struct pldm_rde_varstring *resp_etag;

	// For multipart receive
	uint32_t transfer_handle;
	uint8_t transfer_operation;

	// op complete
	uint8_t completion_code;
};

/**
 * @brief RDE Requester context
 */
struct pldm_rde_requester_context {
	uint8_t context_status;
	int next_command;
	uint8_t requester_status;
	struct pdr_resource *current_pdr_resource;
	void *operation_ctx;
};

/**
 * @brief Context Manager- Manages all the contexts and common information per
 * rde device
 */
struct pldm_rde_requester_manager {
	bool initialized;
	uint8_t n_ctx;
	char device_name[8];
	int net_id;

	uint8_t mc_concurrency;
	uint32_t mc_transfer_size;
	bitfield16_t *mc_feature_support;
	uint32_t negotiated_transfer_size;

	uint32_t resource_ids[MAX_RESOURCE_IDS];
	uint8_t number_of_resources;

	struct pldm_rde_device_info device;
	// Pointer to an array of contexts of size n_ctx.
	struct pldm_rde_requester_context *ctx;
	// A callback to free the pldm_rde_requester_context memory.
	void (*free_requester_ctx)(void *ctx_memory);
};

/**
 * @brief Callback function for letting the requester handle response payload
 */
typedef void (*callback_funct)(struct pldm_rde_requester_manager *manager,
			       struct pldm_rde_requester_context *ctx,
			       /*payload_array*/ uint8_t **,
			       /*payload_length*/ uint32_t,
			       /*has_checksum*/ bool);

/**
 * @brief Initializes the context for PLDM RDE discovery commands
 *
 * @param[in] device_id - Device id of the RDE device
 * @param[in] net_id - Network ID to distinguish between RDE Devices
 * @param[in] manager - Pointer to Context Manager
 * @param[in] mc_concurrency - Concurrency supported by MC
 * @param[in] mc_transfer_size - Transfer Size of MC
 * @param[in] mc_features - Pointer to MC Features
 * @param[in] alloc_requester_ctx - Pointer to a function to allocated contexts
 * for a RDE device
 * @param[in] free_requester_ctx - Pointer to a function that frees allocated
 * memory to contexts
 *
 * @return pldm_requester_rc_t (errno may be set)
 */
pldm_rde_requester_rc_t
pldm_rde_init_context(const char *device_id, int net_id,
		      struct pldm_rde_requester_manager *manager,
		      uint8_t mc_concurrency, uint32_t mc_transfer_size,
		      bitfield16_t *mc_features,
		      struct pldm_rde_requester_context *(*alloc_requester_ctx)(
			      uint8_t number_of_ctx),

		      // Callback function to clean any context memory
		      void (*free_requester_ctx)(void *ctx_memory));

/**
 * @brief Sets the context with resource IDs discovered in the PDR
 *
 * @param[in] manager - Pointer to Context Manager
 * @param[in] number_of_resources - Number of resources supported (until PDR is
 * implemented)
 * @param[in] resource_id_address - The initial resource id index to begin
 * Discovery
 *
 * @return pldm_requester_rc_t (errno may be set)
*/
pldm_rde_requester_rc_t
pldm_rde_set_resources_in_context(struct pldm_rde_requester_manager *manager,
				  uint8_t number_of_resources,
				  uint32_t *resource_id_address);

/**
 * @brief Sets the first command to be triggered for base discovery and sets
 * the status of context to "Ready to PICK
 *
 * @param[in] ctx - pointer to a context which is to be initialized
 *
 * @return pldm_requester_rc_t (errno may be set)
 */
pldm_rde_requester_rc_t
pldm_rde_start_discovery(struct pldm_rde_requester_context *ctx);

/**
 * @brief Pushes the response values to the context of the PLDM_RDE type
 * command that was executed and updates the command status. It alse sets
 * the next_command attribute of the context based on the last executed
 * command.
 *
 * @param[in] manager - Context Manager
 * @param[in] ctx - a pointer to the context
 * @param[in] resp_msg - a pointer to the response message that the caller
 * received
 * @param[in] resp_size - size of the response message payload
 *
 * @return pldm_requester_rc_t (errno may be set)
 */
pldm_rde_requester_rc_t
pldm_rde_discovery_push_response(struct pldm_rde_requester_manager *manager,
				 struct pldm_rde_requester_context *ctx,
				 void *resp_msg, size_t resp_size);

/**
 * @brief Gets the next Discovery Command required for RDE
 *
 * @param[in] instance_id - Getting the instance_id
 * @param[in] manager - PLDM RDE Manager object
 * @param[in] current_ctx - PLDM RDE Requester context which would be
 * responsible for all actions of discovery commands
 * @param[out] request - Request byte stream
 *
 * @return pldm_requester_rc_t (errno may be set)
 */
pldm_rde_requester_rc_t pldm_rde_get_next_discovery_command(
	uint8_t instance_id, struct pldm_rde_requester_manager *manager,
	struct pldm_rde_requester_context *current_ctx,
	struct pldm_msg *request);

/**
 * @brief Creates the RDE context required for RDE operation. Sets the initial
 * state of the context
 *
 * @param[in] current_ctx - Context to be set with the inital state
 *
 * @return pldm_requester_rc_t (errno may be set)
 */
pldm_rde_requester_rc_t
pldm_rde_create_context(struct pldm_rde_requester_context *current_ctx);

/**
 * @brief Initializes the context to trigger dictionary schema RDE ops
 *
 * @param[in] ctx - RDE Requester context to be initialized and set the state to
 * trigger Dictionary Operations over RDE/PLDM
 *
 * @return pldm_requester_rc_t (errno may be set)
 */
pldm_rde_requester_rc_t
pldm_rde_init_get_dictionary_schema(struct pldm_rde_requester_context *ctx);

/**
 * @brief Gets the next command in sequence required to extract dictionaries
 * from the RDE Device for the given resources
 *
 * @param[in] instance_id - Instance ID corresponding to the request
 * @param[in] manager - Context Manager
 * @param[in] current_ctx - RDE Request Context
 * @param[out] request - Request object that would hold the encoded request
 * message for the requester to send
 *
 * @return pldm_requester_rc_t (errno may be set)
 */
pldm_rde_requester_rc_t pldm_rde_get_next_dictionary_schema_command(
	uint8_t instance_id, struct pldm_rde_requester_manager *manager,
	struct pldm_rde_requester_context *current_ctx,
	struct pldm_msg *request);

/**
 * @brief Pushes the response received into current context, updates the state
 * of the context and/or returns the response payload back to the requester
 *
 * @param[in] manager - Context Manager
 * @param[in] ctx - RDE Request Context
 * @param[in] resp_msg - Response received from the RDE Device
 * @param[in] resp_size - Size of the response message
 * @param[in] callback - Pointer to a function that would be executed over the
 * response payload as per requester's requirement
 *
 * @return pldm_requester_rc_t (errno may be set)
 */
pldm_rde_requester_rc_t pldm_rde_push_get_dictionary_response(
	struct pldm_rde_requester_manager *manager,
	struct pldm_rde_requester_context *ctx, void *resp_msg,
	size_t resp_size, callback_funct callback);

/**
 * @brief Initialize RDE operation context for performing any RDE operation
 * More about the terms in the spec:
 * https://www.dmtf.org/sites/default/files/standards/documents/DSP0218_1.1.0.pdf
 *
 * @param[in] ctx - Current context to be initialized
 * @param[in] request_id - Unique request ID for the request
 * @param[in] resource_id - Resource id of the PDR resource RDE operation is to
 * be performed on
 * @param[in] operation_id - Each RDE operation has an operation ID
 * @param[in] operation_type - Operation type (READ, UPDATE, etc)
 * @param[in] operation_flags - Operation flags required by the RDE operation
 * @param[in] send_data_transfer_handle - Transfer handle (usually required if
 * there is a large request payload)
 * @param[in] operation_locator_length - Operation Locator length
 * @param[in] request_payload_length - Request payload length
 * @param[in] operation_locator - operation locator
 * @param[in] request_payload - pointer Request payload buffer
 *
 * @return pldm_requester_rc_t (errno may be set)
 */
pldm_rde_requester_rc_t pldm_rde_init_rde_operation_context(
	struct pldm_rde_requester_context *ctx, uint8_t request_id,
	uint32_t resource_id, uint16_t operation_id, uint8_t operation_type,
	uint8_t op_flags_byte, struct rde_query_options *query_options,
	uint32_t send_data_transfer_handle, uint8_t operation_locator_length,
	uint32_t request_payload_length, uint8_t *operation_locator,
	uint8_t *request_payload);

/**
 * @brief Get next RDE operation in sequence to cater to a RDE request
 *
 * @param[in] instance_id - Instance id corresponding to the request
 * @param[in] manager - Context Manager to perform RDE operation on the device
 * @param[in] current_ctx - Current context to perform RDE operation
 * @param[out] request - Pointer to request message where the encoded pldm msg
 * would be stored
 *
 * @return pldm_requester_rc_t (errno may be set)
 */
pldm_rde_requester_rc_t
pldm_rde_get_next_rde_operation(uint8_t instance_id,
				struct pldm_rde_requester_manager *manager,
				struct pldm_rde_requester_context *current_ctx,
				struct pldm_msg *request);

/**
 * @brief Pushes the RDE response received back to the requester via callback
 * function and also updates the context with the required state to complete the
 * RDE operation
 *
 * @param[in] manager - Context Manager
 * @param[in] ctx - Context for performing RDE operation
 * @param[in] resp_msg - Response received after performing RDE operation
 * @param[in] resp_size - Size of the response received
 * @param[in] callback - Callback function to be executed on the response
 *
 * @return pldm_requester_rc_t (errno may be set)
 */
pldm_rde_requester_rc_t pldm_rde_push_read_operation_response(
	struct pldm_rde_requester_manager *manager,
	struct pldm_rde_requester_context *ctx, void *resp_msg,
	size_t resp_size, callback_funct callback);

/**
 * @brief Cleanup memory after RDE Operation completes (Succeeds/Fails)
 * Needs to be called after every rde_operation_init once RDE Operation init is
 * completed
 *
 * @param[in] ctx - Requester context holds RDE operation context to be cleared
 *
 * @return pldm_requester_rc_t (errno may be set)
*/
pldm_rde_requester_rc_t
free_rde_op_init_context(struct pldm_rde_requester_context *ctx);

/**
 * @brief Cleanup PDR memory after dictioanry extraction
*/
pldm_rde_requester_rc_t free_op_context_after_dictionary_extraction(
	struct pldm_rde_requester_context *ctx);

/**
 * =============== Workaround begins for b/293742455 ===================
 */
/**
 * @brief Provides the RDE Op complete request encoded
 */
pldm_rde_requester_rc_t get_pldm_rde_operation_complete_request(
	uint8_t instance_id, uint32_t resource_id, uint16_t operation_id,
	struct pldm_msg *request);

/**
 * @brief Provides the encoded RDE Op Enumerate request
 */
pldm_rde_requester_rc_t
get_pldm_rde_operation_enumerate_request(uint8_t instance_id,
					 struct pldm_msg *request);

/**
 * @brief Provides the decoded RDE Op Enumerate response
 */
pldm_rde_requester_rc_t get_pldm_rde_operation_enumerate_response(
	struct pldm_msg *response, uint8_t *completion_code,
	uint16_t *operation_count,
	struct pldm_rde_operation_enumerate_operation_data *operation_data,
	uint16_t operation_data_array_size);

/**
 * @brief Provides the RDE Op Kill request encoded
 */
pldm_rde_requester_rc_t
get_pldm_rde_operation_kill_request(uint8_t instance_id, uint32_t resource_id,
				    uint16_t operation_id,
				    struct pldm_msg *request);
/**
 * =============== Workaround ends for b/293742455 ===================
 */

#ifdef __cplusplus
}
#endif

#endif /* PLDM_RDE_REQUESTER_H */
