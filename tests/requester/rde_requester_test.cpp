#include <string.h>

#include <array>
#include <cstring>
#include <iostream>
#include <vector>

#include "libpldm/requester/pldm_rde_requester.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
struct pldm_rde_requester_context rde_contexts[256];
int rde_context_counter = 0;

std::map<uint8_t, int> rde_command_request_size = {
    {PLDM_NEGOTIATE_REDFISH_PARAMETERS, 3},
    {PLDM_NEGOTIATE_MEDIUM_PARAMETERS, 4},
    {PLDM_GET_SCHEMA_DICTIONARY, 5},
    {PLDM_RDE_MULTIPART_RECEIVE, 7}};

std::map<uint8_t, int> rde_op_command_request_size = {
    {PLDM_RDE_OPERATION_INIT, 18},
    {PLDM_RDE_OPERATION_STATUS, 18},
    {PLDM_RDE_OPERATION_COMPLETE, 6},
    {PLDM_RDE_MULTIPART_RECEIVE, 7}};

class TestRdeRequester : public ::testing::Test
{
  public:
    TestRdeRequester()
    {
        instanceId = 1;
        mcConcurrency = 3;
        mcTransferSize = 2056;
        mcFeatures.value = 102;

        numberOfResources = 2;
        resourceIds.emplace_back(0x00000000);
        resourceIds.emplace_back(0x00010000);

        netId = 9;
        devId = "rde_dev";

        // RDE operation test values
        requestId = 0x01;
        resourceId = 0x0001000;
        sendTransferHandle = 0x0001000;
        opId = 0x00;
        opLocLength = 0x00;
        payloadLength = 0x00;
        opLoc = 0x00;
        reqPtr = 0x00;
        flags = 0x00;
    }

    uint8_t mcConcurrency;
    uint32_t mcTransferSize;
    bitfield8_t devCapabilities_;
    bitfield16_t mcFeatures;
    uint8_t numberOfResources;
    std::string devId;
    int netId;
    int instanceId;

    // RDE Operation Test values
    uint8_t requestId;
    uint32_t resourceId;
    uint32_t sendTransferHandle;
    uint16_t opId;
    uint8_t opLocLength;
    uint32_t payloadLength;
    uint8_t opLoc;
    uint8_t reqPtr;
    uint8_t flags;

    std::vector<uint32_t> resourceIds;
};

void free_memory(void* context)
{
    free(context);
}
struct pldm_rde_requester_context*
    allocate_memory_to_contexts(uint8_t number_of_contexts)
{
    int rc;
    int end = rde_context_counter + number_of_contexts;
    for (rde_context_counter = 0; rde_context_counter < end;
         rde_context_counter++)
    {
        struct pldm_rde_requester_context* current_ctx =
            (struct pldm_rde_requester_context*)malloc(
                sizeof(struct pldm_rde_requester_context));
        IGNORE(rc);
        rc = pldm_rde_create_context(current_ctx);
        if (rc)
        {
            return NULL;
        }
        rde_contexts[rde_context_counter] = *current_ctx;
    }

    return &rde_contexts[0];
}

TEST_F(TestRdeRequester, ContextManagerInitializationSuccess)
{
    struct pldm_rde_requester_manager* manager =
        new pldm_rde_requester_manager();

    int rc = pldm_rde_init_context(devId.c_str(), netId, manager,
                                   mcConcurrency, mcTransferSize, &mcFeatures,
                                   numberOfResources, &resourceIds.front(),
                                   allocate_memory_to_contexts, free_memory);

    EXPECT_EQ(rc, PLDM_BASE_REQUESTER_SUCCESS);
    EXPECT_EQ(manager->mc_concurrency, mcConcurrency);
    EXPECT_EQ(manager->mc_transfer_size, mcTransferSize);
    EXPECT_EQ(manager->mc_feature_support, &mcFeatures);
    EXPECT_EQ(manager->device_name, devId);
    EXPECT_EQ(manager->net_id, netId);
    EXPECT_EQ(manager->number_of_resources, numberOfResources);
    EXPECT_EQ(manager->resource_ids[0], 0x00000000);
    EXPECT_EQ(manager->resource_ids[1], 0x00010000);
}

TEST_F(TestRdeRequester, ContextManagerInitializationFailureDueToNullManager)
{
    struct pldm_rde_requester_manager* manager = NULL;
    int rc = pldm_rde_init_context(devId.c_str(), netId, manager,
                                   mcConcurrency, mcTransferSize, &mcFeatures,
                                   numberOfResources, &resourceIds.front(),
                                   allocate_memory_to_contexts, free_memory);
    EXPECT_EQ(rc, PLDM_RDE_CONTEXT_INITIALIZATION_ERROR);
}

TEST_F(TestRdeRequester, ContextManagerInitializationFailureDueToWrongDevId)
{
    std::string incorrect_dev_id;
    struct pldm_rde_requester_manager* manager =
        new pldm_rde_requester_manager();
    int rc = pldm_rde_init_context(incorrect_dev_id.c_str(), netId, manager,
                                   mcConcurrency, mcTransferSize, &mcFeatures,
                                   numberOfResources, &resourceIds.front(),
                                   allocate_memory_to_contexts, free_memory);
    EXPECT_EQ(rc, PLDM_RDE_CONTEXT_INITIALIZATION_ERROR);

    incorrect_dev_id = "";
    rc = pldm_rde_init_context(incorrect_dev_id.c_str(), netId, manager,
                               mcConcurrency, mcTransferSize, &mcFeatures,
                               numberOfResources, &resourceIds.front(),
                               allocate_memory_to_contexts, free_memory);
    EXPECT_EQ(rc, PLDM_RDE_CONTEXT_INITIALIZATION_ERROR);

    incorrect_dev_id = "VERY_LONG_DEV_ID";
    rc = pldm_rde_init_context(incorrect_dev_id.c_str(), netId, manager,
                               mcConcurrency, mcTransferSize, &mcFeatures,
                               numberOfResources, &resourceIds.front(),
                               allocate_memory_to_contexts, free_memory);
    EXPECT_EQ(rc, PLDM_RDE_CONTEXT_INITIALIZATION_ERROR);
}

TEST_F(TestRdeRequester,
       ContextManagerInitializationFailureDueToNullAllocatorFunctions)
{
    struct pldm_rde_requester_manager* manager =
        new pldm_rde_requester_manager();
    int rc =
        pldm_rde_init_context(devId.c_str(), netId, manager, mcConcurrency,
                              mcTransferSize, &mcFeatures, numberOfResources,
                              &resourceIds.front(), NULL, free_memory);
    EXPECT_EQ(rc, PLDM_RDE_CONTEXT_INITIALIZATION_ERROR);

    rc = pldm_rde_init_context(devId.c_str(), netId, manager, mcConcurrency,
                               mcTransferSize, &mcFeatures, numberOfResources,
                               &resourceIds.front(),
                               allocate_memory_to_contexts, NULL);
    EXPECT_EQ(rc, PLDM_RDE_CONTEXT_INITIALIZATION_ERROR);
}

TEST_F(TestRdeRequester, StartRDEDiscoverySuccess)
{
    struct pldm_rde_requester_manager* manager =
        new pldm_rde_requester_manager();

    int rc = pldm_rde_init_context(devId.c_str(), netId, manager,
                                   mcConcurrency, mcTransferSize, &mcFeatures,
                                   numberOfResources, &resourceIds.front(),
                                   allocate_memory_to_contexts, free_memory);

    struct pldm_rde_requester_context base_context = rde_contexts[0];

    rc = pldm_rde_start_discovery(&base_context);

    EXPECT_EQ(rc, PLDM_BASE_REQUESTER_SUCCESS);
    EXPECT_EQ(base_context.next_command, PLDM_NEGOTIATE_REDFISH_PARAMETERS);
}

TEST_F(TestRdeRequester, StartRDEDiscoveryFailure)
{
    struct pldm_rde_requester_manager* manager =
        new pldm_rde_requester_manager();

    int rc = pldm_rde_init_context(devId.c_str(), netId, manager,
                                   mcConcurrency, mcTransferSize, &mcFeatures,
                                   numberOfResources, &resourceIds.front(),
                                   allocate_memory_to_contexts, free_memory);

    struct pldm_rde_requester_context base_context = rde_contexts[0];

    base_context.context_status = CONTEXT_BUSY;
    rc = pldm_rde_start_discovery(&base_context);
    EXPECT_EQ(rc, PLDM_RDE_CONTEXT_NOT_READY);
}

TEST_F(TestRdeRequester, CreateRequesterContextSuccess)
{
    struct pldm_rde_requester_manager* manager =
        new pldm_rde_requester_manager();

    int rc = pldm_rde_init_context(devId.c_str(), netId, manager,
                                   mcConcurrency, mcTransferSize, &mcFeatures,
                                   numberOfResources, &resourceIds.front(),
                                   allocate_memory_to_contexts, free_memory);

    struct pldm_rde_requester_context* current_ctx =
        new pldm_rde_requester_context();
    rc = pldm_rde_create_context(current_ctx);

    EXPECT_EQ(rc, PLDM_BASE_REQUESTER_SUCCESS);
    EXPECT_EQ(current_ctx->context_status, CONTEXT_FREE);
    EXPECT_EQ(current_ctx->requester_status,
              PLDM_RDE_REQUESTER_READY_TO_PICK_NEXT_REQUEST);
    EXPECT_EQ(current_ctx->next_command,
              PLDM_RDE_REQUESTER_NO_NEXT_COMMAND_FOUND);
}

TEST_F(TestRdeRequester, CreateRequesterContextFailure)
{
    struct pldm_rde_requester_manager* manager =
        new pldm_rde_requester_manager();

    int rc = pldm_rde_init_context(devId.c_str(), netId, manager,
                                   mcConcurrency, mcTransferSize, &mcFeatures,
                                   numberOfResources, &resourceIds.front(),
                                   allocate_memory_to_contexts, free_memory);

    struct pldm_rde_requester_context* current_ctx = NULL;
    rc = pldm_rde_create_context(current_ctx);

    EXPECT_EQ(rc, PLDM_RDE_CONTEXT_INITIALIZATION_ERROR);
}

int test_get_next_request_seq(pldm_rde_requester_manager** manager,
                              struct pldm_rde_requester_context** ctx,
                              uint8_t next_command, int instanceId)
{
    (*ctx)->next_command = next_command;
    int requestBytes = 0;
    if (rde_command_request_size.find((*ctx)->next_command) !=
        rde_command_request_size.end())
    {
        requestBytes = rde_command_request_size[(*ctx)->next_command];
    }
    std::vector<uint8_t> msg(sizeof(pldm_msg_hdr) + requestBytes);
    auto request = reinterpret_cast<pldm_msg*>(msg.data());
    return pldm_rde_get_next_discovery_command(instanceId, *manager, *ctx,
                                               request);
}

TEST_F(TestRdeRequester, GetNextRequestInSequenceSuccess)
{
    struct pldm_rde_requester_manager* manager =
        new pldm_rde_requester_manager();

    int rc = pldm_rde_init_context(devId.c_str(), netId, manager,
                                   mcConcurrency, mcTransferSize, &mcFeatures,
                                   numberOfResources, &resourceIds.front(),
                                   allocate_memory_to_contexts, free_memory);

    struct pldm_rde_requester_context* base_context =
        new pldm_rde_requester_context();
    rc = pldm_rde_create_context(base_context);

    rc = test_get_next_request_seq(&manager, &base_context,
                                   PLDM_NEGOTIATE_REDFISH_PARAMETERS,
                                   instanceId);
    EXPECT_EQ(rc, PLDM_BASE_REQUESTER_SUCCESS);

    rc = test_get_next_request_seq(&manager, &base_context,
                                   PLDM_NEGOTIATE_MEDIUM_PARAMETERS,
                                   instanceId);
    EXPECT_EQ(rc, PLDM_BASE_REQUESTER_SUCCESS);
}

TEST_F(TestRdeRequester, GetNextRequestInSequenceFailure)
{
    struct pldm_rde_requester_manager* manager =
        new pldm_rde_requester_manager();

    int rc = pldm_rde_init_context(devId.c_str(), netId, manager,
                                   mcConcurrency, mcTransferSize, &mcFeatures,
                                   numberOfResources, &resourceIds.front(),
                                   allocate_memory_to_contexts, free_memory);

    struct pldm_rde_requester_context* base_context =
        new pldm_rde_requester_context();
    rc = pldm_rde_create_context(base_context);
    rc = test_get_next_request_seq(
        &manager, &base_context, 0x0023,
        instanceId); // Unknown request code to encode
    EXPECT_EQ(rc, PLDM_RDE_REQUESTER_ENCODING_REQUEST_FAILURE);
}

TEST_F(TestRdeRequester, PushDiscoveryResponseRedfishParamSuccess)
{
    struct pldm_rde_requester_manager* manager =
        new pldm_rde_requester_manager();

    int rc = pldm_rde_init_context(devId.c_str(), netId, manager,
                                   mcConcurrency, mcTransferSize, &mcFeatures,
                                   numberOfResources, &resourceIds.front(),
                                   allocate_memory_to_contexts, free_memory);

    struct pldm_rde_requester_context* base_context =
        new pldm_rde_requester_context();
    rc = pldm_rde_create_context(base_context);

    std::vector<uint8_t> response(sizeof(pldm_msg_hdr) + 12, 0);
    uint8_t* responseMsg = response.data();
    size_t responseMsgSize = sizeof(pldm_msg_hdr) + 12;
    auto responsePtr = reinterpret_cast<struct pldm_msg*>(responseMsg);

    rc = test_get_next_request_seq(&manager, &base_context,
                                   PLDM_NEGOTIATE_REDFISH_PARAMETERS,
                                   instanceId);
    EXPECT_EQ(rc, PLDM_BASE_REQUESTER_SUCCESS);
    rc = encode_negotiate_redfish_parameters_resp(
        instanceId, /*completion_code=*/0,
        /*device_concurrency=*/mcConcurrency, devCapabilities_,
        /*device_capabilities_flags=*/mcFeatures,
        /*dev provider name*/ 0x00f,
        /*Example*/ devId.c_str(),
        /*device_configuration_signature*/ PLDM_RDE_VARSTRING_UTF_16,
        responsePtr);
    EXPECT_EQ(rc, 0);

    rc = pldm_rde_discovery_push_response(manager, base_context, responsePtr,
                                          responseMsgSize);

    EXPECT_EQ(rc, PLDM_RDE_REQUESTER_SUCCESS);
    EXPECT_EQ(manager->device.device_concurrency, mcConcurrency);
    EXPECT_EQ(manager->device.device_capabilities_flag.byte,
              devCapabilities_.byte);
    EXPECT_EQ(manager->device.device_feature_support.value,
              mcFeatures.value);
    EXPECT_EQ(base_context->next_command, PLDM_NEGOTIATE_MEDIUM_PARAMETERS);

    rc = test_get_next_request_seq(
        &manager, &base_context, PLDM_NEGOTIATE_MEDIUM_PARAMETERS, instanceId);
    EXPECT_EQ(rc, PLDM_BASE_REQUESTER_SUCCESS);

    rc = encode_negotiate_medium_parameters_resp(
        instanceId, /*completion_code=*/0,
        /*device_maximum_transfer_bytes*/ 256, responsePtr);
    EXPECT_EQ(rc, 0);
}

TEST_F(TestRdeRequester, PushDiscoveryResponseRedfishMediumParamSuccess)
{
    struct pldm_rde_requester_manager* manager =
        new pldm_rde_requester_manager();

    int rc = pldm_rde_init_context(devId.c_str(), netId, manager,
                                   mcConcurrency, mcTransferSize, &mcFeatures,
                                   numberOfResources, &resourceIds.front(),
                                   allocate_memory_to_contexts, free_memory);

    struct pldm_rde_requester_context* base_context =
        new pldm_rde_requester_context();
    rc = pldm_rde_create_context(base_context);

    std::vector<uint8_t> response(sizeof(pldm_msg_hdr) + 6, 0);
    uint8_t* responseMsg = response.data();
    size_t responseMsgSize = sizeof(pldm_msg_hdr) + 6;
    auto responsePtr = reinterpret_cast<struct pldm_msg*>(responseMsg);

    rc = test_get_next_request_seq(
        &manager, &base_context, PLDM_NEGOTIATE_MEDIUM_PARAMETERS, instanceId);
    EXPECT_EQ(rc, PLDM_BASE_REQUESTER_SUCCESS);

    rc = encode_negotiate_medium_parameters_resp(
        instanceId, /*completion_code=*/0,
        /*device_maximum_transfer_bytes*/ 256, responsePtr);
    EXPECT_EQ(rc, 0);

    manager->device.device_maximum_transfer_chunk_size = 256;
    manager->mc_transfer_size = 256;
    rc = pldm_rde_discovery_push_response(manager, base_context, responsePtr,
                                          responseMsgSize);

    EXPECT_EQ(rc, PLDM_RDE_REQUESTER_SUCCESS);
    EXPECT_EQ(manager->negotiated_transfer_size, 256);
}

TEST_F(TestRdeRequester, PushDiscoveryResponseFailure)
{
    struct pldm_rde_requester_manager* manager =
        new pldm_rde_requester_manager();

    int rc = pldm_rde_init_context(devId.c_str(), netId, manager,
                                   mcConcurrency, mcTransferSize, &mcFeatures,
                                   numberOfResources, &resourceIds.front(),
                                   allocate_memory_to_contexts, free_memory);

    struct pldm_rde_requester_context* base_context =
        new pldm_rde_requester_context();
    rc = pldm_rde_create_context(base_context);
    std::vector<uint8_t> response(sizeof(pldm_msg_hdr) + 12, 0);
    uint8_t* responseMsg = response.data();
    size_t responseMsgSize = sizeof(pldm_msg_hdr) + 12;
    auto responsePtr = reinterpret_cast<struct pldm_msg*>(responseMsg);
    rc = pldm_rde_discovery_push_response(manager, base_context, responsePtr,
                                          responseMsgSize);

    EXPECT_EQ(rc, PLDM_RDE_REQUESTER_NO_NEXT_COMMAND_FOUND);
}

TEST_F(TestRdeRequester, InitDictionarySchemaSuccess)
{
    struct pldm_rde_requester_manager* manager =
        new pldm_rde_requester_manager();

    int rc = pldm_rde_init_context(devId.c_str(), netId, manager,
                                   mcConcurrency, mcTransferSize, &mcFeatures,
                                   numberOfResources, &resourceIds.front(),
                                   allocate_memory_to_contexts, free_memory);
    EXPECT_EQ(rc, PLDM_BASE_REQUESTER_SUCCESS);
    struct pldm_rde_requester_context* base_context =
        new pldm_rde_requester_context();
    rc = pldm_rde_create_context(base_context);

    rc = pldm_rde_init_get_dictionary_schema(base_context);
    EXPECT_EQ(rc, PLDM_RDE_REQUESTER_SUCCESS);
    EXPECT_EQ(base_context->current_pdr_resource->schema_class,
              PLDM_RDE_SCHEMA_MAJOR);
}

TEST_F(TestRdeRequester, InitDictionarySchemaFailure)
{
    struct pldm_rde_requester_manager* manager =
        new pldm_rde_requester_manager();

    int rc = pldm_rde_init_context(devId.c_str(), netId, manager,
                                   mcConcurrency, mcTransferSize, &mcFeatures,
                                   numberOfResources, &resourceIds.front(),
                                   allocate_memory_to_contexts, free_memory);
    EXPECT_EQ(rc, PLDM_BASE_REQUESTER_SUCCESS);
    struct pldm_rde_requester_context* base_context =
        new pldm_rde_requester_context();
    base_context->context_status = CONTEXT_BUSY;

    rc = pldm_rde_init_get_dictionary_schema(base_context);
    EXPECT_EQ(rc, PLDM_RDE_CONTEXT_INITIALIZATION_ERROR);
}

TEST_F(TestRdeRequester, GetNextDictionarySchemaSuccess)
{
    struct pldm_rde_requester_manager* manager =
        new pldm_rde_requester_manager();

    int rc = pldm_rde_init_context(devId.c_str(), netId, manager,
                                   mcConcurrency, mcTransferSize, &mcFeatures,
                                   numberOfResources, &resourceIds.front(),
                                   allocate_memory_to_contexts, free_memory);
    EXPECT_EQ(rc, PLDM_BASE_REQUESTER_SUCCESS);
    struct pldm_rde_requester_context* base_context =
        new pldm_rde_requester_context();
    rc = pldm_rde_create_context(base_context);

    rc = pldm_rde_init_get_dictionary_schema(base_context);
    EXPECT_EQ(rc, PLDM_RDE_REQUESTER_SUCCESS);

    int requestBytes = 0;
    if (rde_command_request_size.find(base_context->next_command) !=
        rde_command_request_size.end())
    {
        requestBytes = rde_command_request_size[base_context->next_command];
    }
    std::vector<uint8_t> requestMsg(sizeof(pldm_msg_hdr) + requestBytes);
    auto request = reinterpret_cast<pldm_msg*>(requestMsg.data());
    rc = pldm_rde_get_next_dictionary_schema_command(instanceId, manager,
                                                     base_context, request);

    EXPECT_EQ(rc, PLDM_RDE_REQUESTER_SUCCESS);
}

TEST_F(TestRdeRequester, GetNextDictionarySchemaFailure)
{
    struct pldm_rde_requester_manager* manager =
        new pldm_rde_requester_manager();

    int rc = pldm_rde_init_context(devId.c_str(), netId, manager,
                                   mcConcurrency, mcTransferSize, &mcFeatures,
                                   numberOfResources, &resourceIds.front(),
                                   allocate_memory_to_contexts, free_memory);
    EXPECT_EQ(rc, PLDM_BASE_REQUESTER_SUCCESS);
    struct pldm_rde_requester_context* base_context =
        new pldm_rde_requester_context();
    rc = pldm_rde_create_context(base_context);

    base_context->next_command = 0x23;

    std::vector<uint8_t> requestMsg(sizeof(pldm_msg_hdr));
    auto request = reinterpret_cast<pldm_msg*>(requestMsg.data());
    rc = pldm_rde_get_next_dictionary_schema_command(instanceId, manager,
                                                     base_context, request);
    EXPECT_EQ(rc, PLDM_RDE_REQUESTER_ENCODING_REQUEST_FAILURE);
}

TEST_F(TestRdeRequester, MultipartReceiveSuccess)
{
    struct pldm_rde_requester_manager* manager =
        new pldm_rde_requester_manager();

    int rc = pldm_rde_init_context(devId.c_str(), netId, manager,
                                   mcConcurrency, mcTransferSize, &mcFeatures,
                                   numberOfResources, &resourceIds.front(),
                                   allocate_memory_to_contexts, free_memory);
    EXPECT_EQ(rc, PLDM_BASE_REQUESTER_SUCCESS);
    struct pldm_rde_requester_context* base_context =
        new pldm_rde_requester_context();
    rc = pldm_rde_create_context(base_context);

    rc = pldm_rde_init_get_dictionary_schema(base_context);
    EXPECT_EQ(rc, PLDM_RDE_REQUESTER_SUCCESS);

    base_context->next_command = PLDM_RDE_MULTIPART_RECEIVE;
    base_context->current_pdr_resource->transfer_handle = 0x00;

    int requestBytes = 0;
    if (rde_command_request_size.find(base_context->next_command) !=
        rde_command_request_size.end())
    {
        requestBytes = rde_command_request_size[base_context->next_command];
    }
    std::vector<uint8_t> requestMsg(sizeof(pldm_msg_hdr) + requestBytes);
    auto request = reinterpret_cast<pldm_msg*>(requestMsg.data());

    rc = pldm_rde_get_next_dictionary_schema_command(instanceId, manager,
                                                     base_context, request);
    EXPECT_EQ(rc, PLDM_RDE_REQUESTER_SUCCESS);
}

void dummy_callback(struct pldm_rde_requester_manager* manager,
                    struct pldm_rde_requester_context* ctx, uint8_t** payload,
                    uint32_t payload_length, bool has_checksum)
{
    IGNORE(manager);
    IGNORE(ctx);
    IGNORE(payload);
    IGNORE(payload_length);
    IGNORE(has_checksum);
}

TEST_F(TestRdeRequester, PushResponseForGetDictionarySchema)
{
    struct pldm_rde_requester_manager* manager =
        new pldm_rde_requester_manager();

    int rc = pldm_rde_init_context(devId.c_str(), netId, manager,
                                   mcConcurrency, mcTransferSize, &mcFeatures,
                                   numberOfResources, &resourceIds.front(),
                                   allocate_memory_to_contexts, free_memory);
    EXPECT_EQ(rc, PLDM_BASE_REQUESTER_SUCCESS);
    struct pldm_rde_requester_context* base_context =
        new pldm_rde_requester_context();
    rc = pldm_rde_create_context(base_context);

    rc = pldm_rde_init_get_dictionary_schema(base_context);
    EXPECT_EQ(rc, PLDM_RDE_REQUESTER_SUCCESS);

    std::vector<uint8_t> response(sizeof(pldm_msg_hdr) + 6, 0);
    uint8_t* responseMsg = response.data();
    size_t responseMsgSize = sizeof(pldm_msg_hdr) + 6;
    auto responsePtr = reinterpret_cast<struct pldm_msg*>(responseMsg);

    rc = encode_get_schema_dictionary_resp(instanceId,
                                           /*cc*/ 0,
                                           /* dictionary_format*/ 0x00,
                                           /*transfer_handle*/ 0x00,
                                           responsePtr);

    EXPECT_EQ(rc, PLDM_RDE_REQUESTER_SUCCESS);
    rc = pldm_rde_push_get_dictionary_response(
        manager, base_context, responsePtr, responseMsgSize, dummy_callback);
    EXPECT_EQ(rc, PLDM_RDE_REQUESTER_SUCCESS);
}

TEST_F(TestRdeRequester, PushResponseForMultipartReceive)
{
    struct pldm_rde_requester_manager* manager =
        new pldm_rde_requester_manager();

    int rc = pldm_rde_init_context(devId.c_str(), netId, manager,
                                   mcConcurrency, mcTransferSize, &mcFeatures,
                                   numberOfResources, &resourceIds.front(),
                                   allocate_memory_to_contexts, free_memory);
    EXPECT_EQ(rc, PLDM_BASE_REQUESTER_SUCCESS);
    struct pldm_rde_requester_context* base_context =
        new pldm_rde_requester_context();
    rc = pldm_rde_create_context(base_context);

    rc = pldm_rde_init_get_dictionary_schema(base_context);
    EXPECT_EQ(rc, PLDM_RDE_REQUESTER_SUCCESS);

    base_context->next_command = PLDM_RDE_MULTIPART_RECEIVE;
    std::vector<uint8_t> response(sizeof(pldm_msg_hdr) + 20, 0);
    uint8_t* responseMsg = response.data();
    size_t responseMsgSize = sizeof(pldm_msg_hdr) + 20;
    auto responsePtr = reinterpret_cast<struct pldm_msg*>(responseMsg);

    uint8_t payload[] = {2, 3, 4};
    rc = encode_rde_multipart_receive_resp(
        instanceId, /*completion_code*/ 0, /*transfer_flag*/ 0x00,
        /*next_data_transfer_handle*/ 0x00, /*data_length_bytes*/ 8,
        /*add_checksum*/ false, /*checksum*/ 0x00, &payload[0], responsePtr);
    EXPECT_EQ(rc, PLDM_RDE_REQUESTER_SUCCESS);
    rc = pldm_rde_push_get_dictionary_response(
        manager, base_context, responsePtr, responseMsgSize, dummy_callback);
    EXPECT_EQ(rc, PLDM_RDE_REQUESTER_SUCCESS);
}


TEST_F(TestRdeRequester, InitRDEOperationContextSuccess)
{
    struct pldm_rde_requester_manager* manager =
        new pldm_rde_requester_manager();

    int rc = pldm_rde_init_context(devId.c_str(), netId, manager,
                                   mcConcurrency, mcTransferSize, &mcFeatures,
                                   numberOfResources, &resourceIds.front(),
                                   allocate_memory_to_contexts, free_memory);

    struct pldm_rde_requester_context* base_context =
        new pldm_rde_requester_context();
    rc = pldm_rde_create_context(base_context);

    struct rde_query_options *query_options = NULL;

    rc = pldm_rde_init_rde_operation_context(
        base_context, requestId, resourceId, opId, PLDM_RDE_OPERATION_READ,
        flags, query_options, sendTransferHandle, opLocLength, payloadLength, &opLoc,
        &reqPtr);

    EXPECT_EQ(rc, PLDM_RDE_REQUESTER_SUCCESS);
    struct rde_operation* operation =
        (struct rde_operation*)base_context->operation_ctx;
    EXPECT_EQ(operation->request_id, requestId);
    EXPECT_EQ(operation->resource_id, resourceId);
}

TEST_F(TestRdeRequester, InitRDEOperationContextFailure)
{
    struct pldm_rde_requester_manager* manager =
        new pldm_rde_requester_manager();

    int rc = pldm_rde_init_context(devId.c_str(), netId, manager,
                                   mcConcurrency, mcTransferSize, &mcFeatures,
                                   numberOfResources, &resourceIds.front(),
                                   allocate_memory_to_contexts, free_memory);

    struct pldm_rde_requester_context* base_context =
        new pldm_rde_requester_context();
    rc = pldm_rde_create_context(base_context);

    base_context->context_status = CONTEXT_BUSY;
    struct rde_query_options *query_options = NULL;
    rc = pldm_rde_init_rde_operation_context(
        base_context, requestId, resourceId, opId, PLDM_RDE_OPERATION_READ,
        flags, query_options, sendTransferHandle, opLocLength, payloadLength, &opLoc,
        &reqPtr);

    EXPECT_EQ(rc, PLDM_RDE_CONTEXT_INITIALIZATION_ERROR);
}

TEST_F(TestRdeRequester, GetNextRDEOperationSuccess)
{
    struct pldm_rde_requester_manager* manager =
        new pldm_rde_requester_manager();
    int rc = pldm_rde_init_context(devId.c_str(), netId, manager,
                                   mcConcurrency, mcTransferSize, &mcFeatures,
                                   numberOfResources, &resourceIds.front(),
                                   allocate_memory_to_contexts, free_memory);

    struct pldm_rde_requester_context* base_context =
        new pldm_rde_requester_context();
    rc = pldm_rde_create_context(base_context);
    struct rde_query_options *query_options = NULL;
    rc = pldm_rde_init_rde_operation_context(
        base_context, requestId, resourceId, opId, PLDM_RDE_OPERATION_READ,
        flags, query_options, sendTransferHandle, opLocLength, payloadLength, &opLoc,
        &reqPtr);

    size_t requestBytes = 256;
    std::vector<uint8_t> requestMsg(sizeof(pldm_msg_hdr) + requestBytes);
    auto request = reinterpret_cast<pldm_msg*>(requestMsg.data());
    rc = pldm_rde_get_next_rde_operation(instanceId, manager, base_context,
                                         request);

    EXPECT_EQ(rc, PLDM_RDE_REQUESTER_SUCCESS);

    base_context->next_command = PLDM_RDE_OPERATION_COMPLETE;
    rc = pldm_rde_get_next_rde_operation(instanceId, manager, base_context,
                                         request);

    EXPECT_EQ(rc, PLDM_RDE_REQUESTER_SUCCESS);

    base_context->next_command = PLDM_RDE_MULTIPART_RECEIVE;
    rc = pldm_rde_get_next_rde_operation(instanceId, manager, base_context,
                                         request);

    EXPECT_EQ(rc, PLDM_RDE_REQUESTER_SUCCESS);
}

TEST_F(TestRdeRequester, GetNextRDEOperationFailure)
{
    struct pldm_rde_requester_manager* manager =
        new pldm_rde_requester_manager();

    int rc = pldm_rde_init_context(devId.c_str(), netId, manager,
                                   mcConcurrency, mcTransferSize, &mcFeatures,
                                   numberOfResources, &resourceIds.front(),
                                   allocate_memory_to_contexts, free_memory);

    struct pldm_rde_requester_context* base_context =
        new pldm_rde_requester_context();
    rc = pldm_rde_create_context(base_context);
    struct rde_query_options *query_options = NULL;
    rc = pldm_rde_init_rde_operation_context(
        base_context, requestId, resourceId, opId, PLDM_RDE_OPERATION_READ,
        flags, query_options, sendTransferHandle, opLocLength, payloadLength, &opLoc,
        &reqPtr);

    size_t requestBytes = 256;
    std::vector<uint8_t> requestMsg(sizeof(pldm_msg_hdr) + requestBytes);
    auto request = reinterpret_cast<pldm_msg*>(requestMsg.data());
    manager->initialized = false;
    rc = pldm_rde_get_next_rde_operation(instanceId, manager, base_context,
                                         request);

    EXPECT_EQ(rc, PLDM_RDE_CONTEXT_INITIALIZATION_ERROR);

    manager->number_of_resources = 0;
    manager->initialized = true;
    rc = pldm_rde_get_next_rde_operation(instanceId, manager, base_context,
                                         request);

    EXPECT_EQ(rc, PLDM_RDE_NO_PDR_RESOURCES_FOUND);

    manager->number_of_resources = 1;
    manager->initialized = true;
    base_context->context_status = CONTEXT_BUSY;
    rc = pldm_rde_get_next_rde_operation(instanceId, manager, base_context,
                                         request);

    EXPECT_EQ(rc, PLDM_RDE_CONTEXT_NOT_READY);
}
