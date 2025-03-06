#include <endian.h>

#include "libpldm/pldm_rde.h"

#include <gmock/gmock-matchers.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
TEST(NegotiateRedfishParametersTest, EncodeResponseSuccess)
{
    uint8_t completionCode = 0;
    uint8_t instanceId = 11;
    uint8_t deviceConcurrencySupport = 1;
    bitfield8_t deviceCapabilitiesFlags = {.byte = 0x3F};
    bitfield16_t deviceFeatureSupport = {.value = 0x7389};
    uint32_t deviceConfigurationSignature = 0xABCDEF12;
    constexpr const char* device = "This is a test";
    // Already has the space for the null character in sizeof(struct
    // pldm_rde_negotiate_redfish_parameters_resp).
    std::array<uint8_t,
               sizeof(struct pldm_msg_hdr) +
                   sizeof(struct pldm_rde_negotiate_redfish_parameters_resp) +
                   14>
        responseMsg{};
    auto response = reinterpret_cast<pldm_msg*>(responseMsg.data());
    EXPECT_EQ(encode_negotiate_redfish_parameters_resp(
                  instanceId, completionCode, deviceConcurrencySupport,
                  deviceCapabilitiesFlags, deviceFeatureSupport,
                  deviceConfigurationSignature, device,
                  PLDM_RDE_VARSTRING_ASCII, response),
              PLDM_SUCCESS);
    // verify header.
    EXPECT_EQ(response->hdr.instance_id, instanceId);
    EXPECT_EQ(response->hdr.request, 0);
    EXPECT_EQ(response->hdr.type, PLDM_RDE);
    EXPECT_EQ(response->hdr.command, PLDM_NEGOTIATE_REDFISH_PARAMETERS);
    // verify payload.
    auto resp_payload =
        reinterpret_cast<pldm_rde_negotiate_redfish_parameters_resp*>(
            response->payload);
    EXPECT_EQ(resp_payload->completion_code, completionCode);
    EXPECT_EQ(resp_payload->device_concurrency_support,
              deviceConcurrencySupport);
    EXPECT_EQ(resp_payload->device_capabilities_flags.byte,
              deviceCapabilitiesFlags.byte);
    EXPECT_EQ(le16toh(resp_payload->device_feature_support.value),
              deviceFeatureSupport.value);
    EXPECT_EQ(le32toh(resp_payload->device_configuration_signature),
              deviceConfigurationSignature);
    EXPECT_EQ(resp_payload->device_provider_name.string_format,
              PLDM_RDE_VARSTRING_ASCII);
    EXPECT_EQ(resp_payload->device_provider_name.string_length_bytes,
              strlen(device) + 1);
    EXPECT_EQ(memcmp(resp_payload->device_provider_name.string_data, device,
                     strlen(device) + 1),
              0);
}
TEST(NegotiateRedfishParametersTest, EncodeRequestSuccess)
{
    uint8_t instanceId = 11;
    uint8_t mcConcurrencySupport = 13;
    bitfield16_t mcFeatureSupport = {.value = 0x7389};
    std::array<uint8_t,
               sizeof(struct pldm_msg_hdr) +
                   sizeof(struct pldm_rde_negotiate_redfish_parameters_req)>
        requestMsg{};
    auto request = reinterpret_cast<pldm_msg*>(requestMsg.data());
    auto req_payload =
        reinterpret_cast<pldm_rde_negotiate_redfish_parameters_req*>(
            request->payload);
    EXPECT_EQ(encode_negotiate_redfish_parameters_req(
                  instanceId, mcConcurrencySupport, &mcFeatureSupport, request),
              PLDM_SUCCESS);
    EXPECT_EQ(request->hdr.instance_id, instanceId);
    EXPECT_EQ(request->hdr.type, PLDM_RDE);
    EXPECT_EQ(request->hdr.request, 1);
    EXPECT_EQ(request->hdr.command, PLDM_NEGOTIATE_REDFISH_PARAMETERS);
    EXPECT_EQ(req_payload->mc_concurrency_support, mcConcurrencySupport);
    EXPECT_EQ(le16toh(req_payload->mc_feature_support.value),
              mcFeatureSupport.value);
}
TEST(NegotiateRedfishParametersTest, DecodeRequestSuccess)
{
    uint8_t mcConcurrencySupport = 1;
    bitfield16_t mcFeatureSupport = {.value = 0x7389};
    std::array<uint8_t,
               sizeof(struct pldm_msg_hdr) +
                   sizeof(struct pldm_rde_negotiate_redfish_parameters_req)>
        requestMsg{};
    auto request = reinterpret_cast<pldm_msg*>(requestMsg.data());
    auto req_payload =
        reinterpret_cast<pldm_rde_negotiate_redfish_parameters_req*>(
            request->payload);
    req_payload->mc_concurrency_support = mcConcurrencySupport;
    req_payload->mc_feature_support.value = htole16(mcFeatureSupport.value);
    uint8_t decodedMcConcurrencySupport;
    bitfield16_t decodedMcFeatureSupport;
    EXPECT_EQ(decode_negotiate_redfish_parameters_req(
                  request,
                  sizeof(struct pldm_rde_negotiate_redfish_parameters_req),
                  &decodedMcConcurrencySupport, &decodedMcFeatureSupport),
              PLDM_SUCCESS);
    EXPECT_EQ(decodedMcConcurrencySupport, mcConcurrencySupport);
    EXPECT_EQ(decodedMcFeatureSupport.value, mcFeatureSupport.value);
}
TEST(NegotiateMediumParametersTest, EncodeRequestSuccess)
{
    uint8_t instanceId = 11;
    uint32_t maxTranferSize = 0xABCDEF18;
    std::array<uint8_t,
               sizeof(struct pldm_msg_hdr) +
                   sizeof(struct pldm_rde_negotiate_medium_parameters_req)>
        requestMsg{};
    auto request = reinterpret_cast<pldm_msg*>(requestMsg.data());
    auto req_payload =
        reinterpret_cast<pldm_rde_negotiate_medium_parameters_req*>(
            request->payload);
    EXPECT_EQ(encode_negotiate_medium_parameters_req(instanceId, maxTranferSize,
                                                     request),
              PLDM_SUCCESS);
    EXPECT_EQ(request->hdr.instance_id, instanceId);
    EXPECT_EQ(request->hdr.type, PLDM_RDE);
    EXPECT_EQ(request->hdr.request, 1);
    EXPECT_EQ(request->hdr.command, PLDM_NEGOTIATE_MEDIUM_PARAMETERS);
    EXPECT_EQ(le32toh(req_payload->mc_maximum_transfer_chunk_size_bytes),
              maxTranferSize);
}
TEST(NegotiateMediumParametersTest, DecodeRequestSuccess)
{
    uint32_t mcSize = 0x10000000;
    std::array<uint8_t,
               sizeof(struct pldm_msg_hdr) +
                   sizeof(struct pldm_rde_negotiate_medium_parameters_req)>
        requestMsg{};
    auto request = reinterpret_cast<pldm_msg*>(requestMsg.data());
    auto req_payload =
        reinterpret_cast<pldm_rde_negotiate_medium_parameters_req*>(
            request->payload);
    req_payload->mc_maximum_transfer_chunk_size_bytes = htole32(mcSize);
    uint32_t decodedMcSize;
    EXPECT_EQ(decode_negotiate_medium_parameters_req(
                  request,
                  sizeof(struct pldm_rde_negotiate_medium_parameters_req),
                  &decodedMcSize),
              PLDM_SUCCESS);
    EXPECT_EQ(decodedMcSize, mcSize);
}
TEST(NegotiateMediumParametersTest, EncodeResponseSuccess)
{
    uint8_t completionCode = 0;
    uint8_t instanceId = 11;
    uint32_t deviceSize = 0x10000000;
    std::array<uint8_t,
               sizeof(struct pldm_msg_hdr) +
                   sizeof(struct pldm_rde_negotiate_medium_parameters_resp)>
        responseMsg{};
    auto response = reinterpret_cast<pldm_msg*>(responseMsg.data());
    EXPECT_EQ(encode_negotiate_medium_parameters_resp(
                  instanceId, completionCode, deviceSize, response),
              PLDM_SUCCESS);
    // verify header.
    EXPECT_EQ(response->hdr.instance_id, instanceId);
    EXPECT_EQ(response->hdr.request, 0);
    EXPECT_EQ(response->hdr.type, PLDM_RDE);
    EXPECT_EQ(response->hdr.command, PLDM_NEGOTIATE_MEDIUM_PARAMETERS);
    // verify payload.
    auto resp_payload =
        reinterpret_cast<pldm_rde_negotiate_medium_parameters_resp*>(
            response->payload);
    EXPECT_EQ(resp_payload->completion_code, completionCode);
    EXPECT_EQ(le32toh(resp_payload->device_maximum_transfer_chunk_size_bytes),
              deviceSize);
}
TEST(GetSchemaDictionaryTest, EncodeRequestSuccess)
{
    uint8_t instanceId = 11;
    uint8_t schema_class = 1;
    uint32_t resourceId = 0xABCDEF18;
    std::array<uint8_t, sizeof(struct pldm_msg_hdr) +
                            sizeof(struct pldm_rde_get_schema_dictionary_req)>
        requestMsg{};
    auto request = reinterpret_cast<pldm_msg*>(requestMsg.data());
    auto req_payload =
        reinterpret_cast<pldm_rde_get_schema_dictionary_req*>(request->payload);
    EXPECT_EQ(encode_get_schema_dictionary_req(instanceId, resourceId,
                                               schema_class, request),
              PLDM_SUCCESS);
    EXPECT_EQ(request->hdr.instance_id, instanceId);
    EXPECT_EQ(request->hdr.type, PLDM_RDE);
    EXPECT_EQ(request->hdr.request, 1);
    EXPECT_EQ(request->hdr.command, PLDM_GET_SCHEMA_DICTIONARY);
    EXPECT_EQ(le32toh(req_payload->resource_id), resourceId);
    EXPECT_EQ(req_payload->requested_schema_class, schema_class);
}
TEST(GetSchemaDictionaryTest, DecodeRequestSuccess)
{
    uint32_t resourceId = 0xABCDEF12;
    std::array<uint8_t, sizeof(struct pldm_msg_hdr) +
                            sizeof(struct pldm_rde_get_schema_dictionary_req)>
        requestMsg{};
    auto request = reinterpret_cast<pldm_msg*>(requestMsg.data());
    auto req_payload =
        reinterpret_cast<pldm_rde_get_schema_dictionary_req*>(request->payload);
    req_payload->resource_id = htole32(resourceId);
    req_payload->requested_schema_class = PLDM_RDE_SCHEMA_ANNOTATION;
    uint32_t decodedResourceId;
    uint8_t decodedSchemaClass;
    EXPECT_EQ(decode_get_schema_dictionary_req(
                  request, sizeof(struct pldm_rde_get_schema_dictionary_req),
                  &decodedResourceId, &decodedSchemaClass),
              PLDM_SUCCESS);
    EXPECT_EQ(decodedResourceId, resourceId);
    EXPECT_EQ(decodedSchemaClass, PLDM_RDE_SCHEMA_ANNOTATION);
}
TEST(GetSchemaDictionaryTest, EncodeResponseSuccess)
{
    uint8_t completionCode = 0;
    uint8_t instanceId = 11;
    uint8_t dictionaryFormat = 0x00;
    uint32_t transferHandle = 0xABCDEF12;
    std::array<uint8_t, sizeof(struct pldm_msg_hdr) +
                            sizeof(struct pldm_rde_get_schema_dictionary_resp)>
        responseMsg{};
    auto response = reinterpret_cast<pldm_msg*>(responseMsg.data());
    EXPECT_EQ(encode_get_schema_dictionary_resp(instanceId, completionCode,
                                                dictionaryFormat,
                                                transferHandle, response),
              PLDM_SUCCESS);
    // verify header.
    EXPECT_EQ(response->hdr.instance_id, instanceId);
    EXPECT_EQ(response->hdr.request, 0);
    EXPECT_EQ(response->hdr.type, PLDM_RDE);
    EXPECT_EQ(response->hdr.command, PLDM_GET_SCHEMA_DICTIONARY);
    // verify payload.
    auto resp_payload = reinterpret_cast<pldm_rde_get_schema_dictionary_resp*>(
        response->payload);
    EXPECT_EQ(resp_payload->completion_code, completionCode);
    EXPECT_EQ(resp_payload->dictionary_format, dictionaryFormat);
    EXPECT_EQ(le32toh(resp_payload->transfer_handle), transferHandle);
}
TEST(NegotiateRedfishParamsTest, DecodeResponseSuccess)
{
    uint8_t completionCode = 0;
    uint8_t instanceId = 11;
    uint8_t deviceConcurrencySupport = 1;
    bitfield8_t deviceCapabilitiesFlags = {.byte = 0x3F};
    bitfield16_t deviceFeatureSupport = {.value = 0x7389};
    uint32_t deviceConfigurationSignature = 0xABCDEF12;
    constexpr const char* device = "This is a test";

    std::array<uint8_t,
               sizeof(struct pldm_msg_hdr) +
                   sizeof(struct pldm_rde_negotiate_redfish_parameters_resp) +
                   14>
        responseMsg{};
    auto response = reinterpret_cast<pldm_msg*>(responseMsg.data());
    auto deviceInfo = std::make_unique<pldm_rde_device_info>();
    uint8_t cc;
    encode_negotiate_redfish_parameters_resp(
        instanceId, completionCode, deviceConcurrencySupport,
        deviceCapabilitiesFlags, deviceFeatureSupport,
        deviceConfigurationSignature, device, PLDM_RDE_VARSTRING_ASCII,
        response);
    EXPECT_EQ(
        decode_negotiate_redfish_parameters_resp(
            response,
            sizeof(struct pldm_rde_negotiate_redfish_parameters_resp) + 14, &cc,
            deviceInfo.get()),
        PLDM_SUCCESS);

    EXPECT_EQ(deviceInfo.get()->device_capabilities_flag.byte,
              deviceCapabilitiesFlags.byte);
    EXPECT_EQ(deviceInfo.get()->device_feature_support.value,
              deviceFeatureSupport.value);
    EXPECT_EQ(deviceInfo.get()->device_configuration_signature,
              deviceConfigurationSignature);
    EXPECT_EQ(cc, completionCode);
    EXPECT_EQ(deviceInfo.get()->device_concurrency, deviceConcurrencySupport);
}
TEST(NegotiateMediumParams, DecodeResponseSuccess)
{
    uint8_t completionCode = 0;
    uint8_t instanceId = 11;
    uint32_t deviceSize = 0x10000000;
    std::array<uint8_t,
               sizeof(struct pldm_msg_hdr) +
                   sizeof(struct pldm_rde_negotiate_medium_parameters_resp)>
        responseMsg{};
    auto response = reinterpret_cast<pldm_msg*>(responseMsg.data());
    EXPECT_EQ(encode_negotiate_medium_parameters_resp(
                  instanceId, completionCode, deviceSize, response),
              PLDM_SUCCESS);

    // verify payload.
    auto resp_payload =
        reinterpret_cast<pldm_rde_negotiate_medium_parameters_resp*>(
            response->payload);
    EXPECT_EQ(resp_payload->completion_code, completionCode);
    EXPECT_EQ(le32toh(resp_payload->device_maximum_transfer_chunk_size_bytes),
              deviceSize);

    uint8_t cc;
    uint32_t deviceMaxTransferBytes;
    EXPECT_EQ(decode_negotiate_medium_parameters_resp(
                  response,
                  sizeof(struct pldm_msg_hdr) +
                      sizeof(struct pldm_rde_negotiate_medium_parameters_resp),
                  &cc, &deviceMaxTransferBytes),
              PLDM_SUCCESS);
    EXPECT_EQ(deviceMaxTransferBytes, deviceSize);
}
TEST(GetSchemaDictionary, DecodeResponseSuccess)
{
    uint8_t completionCode = 0;
    uint8_t instanceId = 11;
    uint8_t dictionaryFormat = 0x00;
    uint32_t transferHandle = 0xABCDEF12;
    constexpr int payloadLength =
        sizeof(struct pldm_msg_hdr) +
        sizeof(struct pldm_rde_get_schema_dictionary_resp);
    std::array<uint8_t, payloadLength> responseMsg{};
    auto response = reinterpret_cast<pldm_msg*>(responseMsg.data());
    EXPECT_EQ(encode_get_schema_dictionary_resp(instanceId, completionCode,
                                                dictionaryFormat,
                                                transferHandle, response),
              PLDM_SUCCESS);
    uint8_t cc, returnDictionaryFormat;
    uint32_t returnTransferHandle;
    EXPECT_EQ(decode_get_schema_dictionary_resp(response, payloadLength, &cc,
                                                &returnDictionaryFormat,
                                                &returnTransferHandle),
              PLDM_SUCCESS);
    EXPECT_EQ(cc, completionCode);
    EXPECT_EQ(returnDictionaryFormat, dictionaryFormat);
    EXPECT_EQ(returnTransferHandle, transferHandle);
}
TEST(MultipartReceive, EncodeRequestSuccess)
{
    uint8_t instanceId = 11;
    uint32_t transferHandle = 0xABCDEF12;
    uint16_t operationId = 0x01;
    uint8_t transferOperation = PLDM_XFER_FIRST_PART;
    constexpr int reqLength =
        sizeof(struct pldm_msg_hdr) + PLDM_MULTIPART_RECEIVE_REQ_BYTES;
    std::array<uint8_t, reqLength> requestMsg{};
    auto request = reinterpret_cast<pldm_msg*>(requestMsg.data());
    auto reqPayload =
        reinterpret_cast<pldm_rde_multipart_receive_req*>(request->payload);
    EXPECT_EQ(encode_rde_multipart_receive_req(instanceId, transferHandle,
                                               operationId, transferOperation,
                                               request),
              PLDM_SUCCESS);
    EXPECT_EQ(request->hdr.instance_id, instanceId);
    EXPECT_EQ(request->hdr.type, PLDM_RDE);
    EXPECT_EQ(request->hdr.request, 1);
    EXPECT_EQ(request->hdr.command, PLDM_RDE_MULTIPART_RECEIVE);
    EXPECT_EQ(reqPayload->operation_id, operationId);
    EXPECT_EQ(reqPayload->data_transfer_handle, transferHandle);
    EXPECT_EQ(reqPayload->transfer_operation, transferOperation);
}
TEST(MultipartReceive, DecodeRequestSuccess)
{
    uint8_t instanceId = 11;
    uint32_t transferHandle = 0xABCDEF12;
    uint16_t operationId = 0x01;
    uint8_t transferOperation = PLDM_XFER_FIRST_PART;
    constexpr int reqLength =
        sizeof(struct pldm_msg_hdr) + PLDM_MULTIPART_RECEIVE_REQ_BYTES;
    std::array<uint8_t, reqLength> requestMsg{};
    auto request = reinterpret_cast<pldm_msg*>(requestMsg.data());
    EXPECT_EQ(encode_rde_multipart_receive_req(instanceId, transferHandle,
                                               operationId, transferOperation,
                                               request),
              PLDM_SUCCESS);

    uint32_t returnTransferHandle;
    uint16_t returnOperationId;
    uint8_t returnTransferOperation;
    EXPECT_EQ(decode_rde_multipart_receive_req(
                  request, sizeof(struct pldm_rde_multipart_receive_req),
                  &returnTransferHandle, &returnOperationId,
                  &returnTransferOperation),
              PLDM_SUCCESS);
    EXPECT_EQ(returnOperationId, operationId);
    EXPECT_EQ(returnTransferHandle, transferHandle);
    EXPECT_EQ(returnTransferOperation, transferOperation);
}
TEST(MultipartReceive, EncodeResponseSuccess)
{
    uint8_t completionCode = 0;
    uint8_t instanceId = 11;
    uint8_t transferOperation = PLDM_XFER_FIRST_PART;
    uint32_t transferHandle = 0xABCDEF12;

    uint8_t payload[] = {0x01, 0x02, 0x03};
    constexpr int payloadSize = sizeof(struct pldm_msg_hdr) +
                                sizeof(struct pldm_rde_multipart_receive_resp) +
                                /*payload array size*/ 3;
    std::array<uint8_t, payloadSize> responseMsg{};
    auto response = reinterpret_cast<pldm_msg*>(responseMsg.data());

    EXPECT_EQ(encode_rde_multipart_receive_resp(
                  instanceId, completionCode, transferOperation, transferHandle,
                  /*payload array size*/ 3,
                  /*addChecksum*/ false, /*checksum*/ 0x00, &payload[0],
                  response),
              PLDM_SUCCESS);

    // // verify header.
    EXPECT_EQ(response->hdr.instance_id, instanceId);
    EXPECT_EQ(response->hdr.request, 0);
    EXPECT_EQ(response->hdr.type, PLDM_RDE);
    EXPECT_EQ(response->hdr.command, PLDM_RDE_MULTIPART_RECEIVE);
    // // verify payload.
    auto resp_payload =
        reinterpret_cast<pldm_rde_multipart_receive_resp*>(response->payload);
    EXPECT_EQ(resp_payload->completion_code, completionCode);
    EXPECT_EQ(resp_payload->transfer_flag, transferOperation);
    EXPECT_EQ(resp_payload->next_data_transfer_handle, transferHandle);
    EXPECT_EQ(resp_payload->data_length_bytes, /*payload array size*/ 3);
}
TEST(MultipartReceive, DecodeResponseSuccess)
{
    uint8_t completionCode = 0;
    uint8_t instanceId = 11;
    uint8_t transferOperation = PLDM_XFER_FIRST_PART;
    uint32_t transferHandle = 0xABCDEF12;

    uint8_t payload[] = {0x01, 0x02, 0x03};
    constexpr size_t payloadSize = 3;
    constexpr int responseSize =
        sizeof(struct pldm_msg_hdr) +
        sizeof(struct pldm_rde_multipart_receive_resp) +
        /*payload array size*/ payloadSize;
    std::array<uint8_t, responseSize> responseMsg{};
    auto response = reinterpret_cast<pldm_msg*>(responseMsg.data());

    EXPECT_EQ(encode_rde_multipart_receive_resp(
                  instanceId, completionCode, transferOperation, transferHandle,
                  /*payload array size*/ payloadSize,
                  /*addChecksum*/ false, /*checksum*/ 0x00, &payload[0],
                  response),
              PLDM_SUCCESS);

    uint8_t cc;
    uint8_t returnTransferFlag;
    uint32_t returnTransferHandle;
    uint32_t returnDataLenBytes;

    uint8_t* returnPayload;
    EXPECT_EQ(decode_rde_multipart_receive_resp(
                  response, /*ASSUME_MAX_SIZE*/ 256, &cc, &returnTransferFlag,
                  &returnTransferHandle, &returnDataLenBytes, &returnPayload),
              PLDM_SUCCESS);
    EXPECT_EQ(cc, completionCode);
    EXPECT_EQ(returnTransferFlag, transferOperation);
    EXPECT_EQ(returnDataLenBytes, payloadSize);
}

TEST(RDEOperationInit, EncodeRequestSuccess)
{
    uint8_t instanceId = 11;
    uint32_t resourceId = 0x0001000;
    uint16_t operationId = 32770;
    uint8_t operationType = PLDM_RDE_OPERATION_READ;
    auto operationFlags = std::make_unique<union pldm_rde_operation_flags>();
    operationFlags.get()->byte = 0x00;
    uint32_t transferHandle = 0xABCDEF12;
    constexpr size_t requestSize = sizeof(struct pldm_msg_hdr) +
                                   sizeof(struct pldm_rde_operation_init_req);
    std::array<uint8_t, requestSize> requestMsg{};
    auto request = reinterpret_cast<pldm_msg*>(requestMsg.data());

    EXPECT_EQ(encode_rde_operation_init_req(instanceId, resourceId, operationId,
                                            operationType, operationFlags.get(),
                                            transferHandle, 0, 0, NULL, NULL,
                                            request),
              PLDM_SUCCESS);

    // Test Header values
    EXPECT_EQ(request->hdr.instance_id, instanceId);
    EXPECT_EQ(request->hdr.request, PLDM_REQUEST);
    EXPECT_EQ(request->hdr.type, PLDM_RDE);
    EXPECT_EQ(request->hdr.command, PLDM_RDE_OPERATION_INIT);

    // Test payload values
    auto reqPayload =
        reinterpret_cast<pldm_rde_operation_init_req*>(request->payload);
    EXPECT_EQ(reqPayload->resource_id, resourceId);
    EXPECT_EQ(reqPayload->operation_id, operationId);
    EXPECT_EQ(reqPayload->operation_type, operationType);
    EXPECT_EQ(reqPayload->operation_flags.byte, operationFlags.get()->byte);
    EXPECT_EQ(reqPayload->operation_locator_length, 0);
    EXPECT_EQ(reqPayload->request_payload_length, 0);
}

TEST(RDEOperationInit, DecodeRequestSuccess)
{
    uint8_t instanceId = 11;
    uint32_t resourceId = 0x0001000;
    uint16_t operationId = 32770;
    uint8_t operationType = PLDM_RDE_OPERATION_READ;
    auto operationFlags = std::make_unique<union pldm_rde_operation_flags>();
    operationFlags.get()->byte = 0x00;
    uint32_t transferHandle = 0xABCDEF12;
    constexpr size_t requestSize = sizeof(struct pldm_msg_hdr) +
                                   sizeof(struct pldm_rde_operation_init_req);
    std::array<uint8_t, requestSize> requestMsg{};
    auto request = reinterpret_cast<pldm_msg*>(requestMsg.data());

    encode_rde_operation_init_req(instanceId, resourceId, operationId,
                                  operationType, operationFlags.get(),
                                  transferHandle, 0, 0, NULL, NULL, request);

    uint32_t returnResourceId;
    uint16_t returnOperationId;
    uint8_t returnOperationType;
    uint32_t returnSendTransferHandle;
    uint8_t returnOperationLocatorLength;
    uint32_t returnRequestPayloadLength;
    uint8_t* returnOperationLocator = std::make_unique<uint8_t>().get();
    uint8_t* returnRequestPayload = std::make_unique<uint8_t>().get();
    union pldm_rde_operation_flags returnOperationFlags;

    EXPECT_EQ(decode_rde_operation_init_req(
                  request, sizeof(struct pldm_rde_operation_init_req),
                  &returnResourceId, &returnOperationId, &returnOperationType,
                  &returnOperationFlags, &returnSendTransferHandle,
                  &returnOperationLocatorLength, &returnRequestPayloadLength,
                  &returnOperationLocator, &returnRequestPayload),
              PLDM_SUCCESS);
    EXPECT_EQ(returnResourceId, resourceId);
    EXPECT_EQ(returnOperationId, operationId);
    EXPECT_EQ(returnOperationType, operationType);
    EXPECT_EQ(returnSendTransferHandle, transferHandle);
    EXPECT_EQ(returnOperationFlags.byte, operationFlags.get()->byte);
    EXPECT_EQ(returnOperationLocatorLength, 0);
    EXPECT_EQ(returnRequestPayloadLength, 0);
}

TEST(RDEOperationInit, EncodeResponseSuccess)
{
    uint8_t completionCode = 0;
    uint8_t instanceId = 11;
    uint8_t operationStatus = PLDM_RDE_OPERATION_COMPLETED;
    uint8_t completionPercentage = 100;
    uint32_t completionTimeSeconds = 1;
    union pldm_rde_op_execution_flags operationExecutionFlags;
    operationExecutionFlags.byte = 0x01;
    uint32_t resultTransferHandle = 0x01;
    union pldm_rde_permission_flags permissionFlags;
    permissionFlags.byte = 0x02;
    enum pldm_rde_varstring_format etagFormat = PLDM_RDE_VARSTRING_UTF_8;
    const char etag[] = "etag";

    constexpr size_t responsePayloadLength =
        sizeof(struct pldm_msg_hdr) +
        sizeof(struct pldm_rde_operation_init_resp) + 4;
    std::array<uint8_t, responsePayloadLength> responseMsg{};
    auto response = reinterpret_cast<pldm_msg*>(responseMsg.data());
    EXPECT_EQ(encode_rde_operation_init_resp(
                  instanceId, completionCode, operationStatus,
                  completionPercentage, completionTimeSeconds,
                  &operationExecutionFlags, resultTransferHandle,
                  &permissionFlags, 0, etagFormat, etag, NULL, response),
              PLDM_SUCCESS);
    // verify header.
    EXPECT_EQ(response->hdr.instance_id, instanceId);
    EXPECT_EQ(response->hdr.request, 0);
    EXPECT_EQ(response->hdr.type, PLDM_RDE);
    EXPECT_EQ(response->hdr.command, PLDM_RDE_OPERATION_INIT);
    // verify payload.
    auto respPayload =
        reinterpret_cast<pldm_rde_operation_init_resp*>(response->payload);
    EXPECT_EQ(respPayload->completion_code, completionCode);
    EXPECT_EQ(respPayload->operation_status, operationStatus);
    EXPECT_EQ(respPayload->completion_percentage, completionPercentage);
    EXPECT_EQ(respPayload->completion_time_seconds, completionTimeSeconds);
    EXPECT_EQ(respPayload->operation_execution_flags.byte,
              operationExecutionFlags.byte);
    EXPECT_EQ(respPayload->permission_flags.byte, permissionFlags.byte);
}

TEST(RDEOperationInit, DecodeResponseSuccess)
{
    uint8_t completionCode = 0;
    uint8_t instanceId = 11;
    uint8_t operationStatus = PLDM_RDE_OPERATION_COMPLETED;
    uint8_t completionPercentage = 100;
    uint32_t completionTimeSeconds = 1;
    union pldm_rde_op_execution_flags operationExecutionFlags;
    operationExecutionFlags.byte = 0x01;
    uint32_t resultTransferHandle = 0x01;
    union pldm_rde_permission_flags permissionFlags;
    permissionFlags.byte = 0x02;
    enum pldm_rde_varstring_format etagFormat = PLDM_RDE_VARSTRING_UTF_8;
    const char etag[] = "etag";

    constexpr size_t responsePayloadLength =
        sizeof(struct pldm_msg_hdr) +
        sizeof(struct pldm_rde_operation_init_resp) + 4;
    std::array<uint8_t, responsePayloadLength> responseMsg{};
    auto response = reinterpret_cast<pldm_msg*>(responseMsg.data());
    EXPECT_EQ(encode_rde_operation_init_resp(
                  instanceId, completionCode, operationStatus,
                  completionPercentage, completionTimeSeconds,
                  &operationExecutionFlags, resultTransferHandle,
                  &permissionFlags, 0, etagFormat, etag, NULL, response),
              PLDM_SUCCESS);

    uint8_t returnCompletionCode;
    uint8_t returnOperationStatus;
    uint8_t returnCompletionPercentage;
    uint32_t returnCompletionTimeSeconds;
    uint32_t returnTransferHandle;
    uint32_t returnResponsePayloadLength;

    union pldm_rde_op_execution_flags* returnExecutionFlags =
        (union pldm_rde_op_execution_flags*)malloc(
            sizeof(union pldm_rde_op_execution_flags));
    union pldm_rde_permission_flags* returnPermissionFlags =
        (union pldm_rde_permission_flags*)malloc(
            sizeof(union pldm_rde_permission_flags));

    auto returnEtag = std::make_unique<pldm_rde_varstring>().get();
    auto responsePayload = std::make_unique<uint8_t>().get();
    EXPECT_EQ(decode_rde_operation_init_resp(
                  response, responsePayloadLength, &returnCompletionCode,
                  &returnCompletionPercentage, &returnOperationStatus,
                  &returnCompletionTimeSeconds, &returnTransferHandle,
                  &returnResponsePayloadLength, &returnPermissionFlags,
                  &returnExecutionFlags, &returnEtag, &responsePayload),
              PLDM_SUCCESS);

    EXPECT_EQ(returnCompletionCode, completionCode);
    EXPECT_EQ(returnCompletionPercentage, completionPercentage);
    EXPECT_EQ(returnOperationStatus, operationStatus);
    EXPECT_EQ(returnCompletionTimeSeconds, completionTimeSeconds);
    EXPECT_EQ(returnTransferHandle, resultTransferHandle);
    EXPECT_EQ(returnResponsePayloadLength, 0);
    EXPECT_EQ(returnPermissionFlags->byte, permissionFlags.byte);
    EXPECT_EQ(returnExecutionFlags->byte, operationExecutionFlags.byte);
    EXPECT_EQ(returnEtag->string_length_bytes,
              std::end(etag) - std::begin(etag));
}
TEST(RDEOperationComplete, EncodeRequestSuccess)
{
    uint8_t instanceId = 11;
    uint32_t resourceId = 0x0001000;
    uint16_t operationId = 32770;
    constexpr size_t requestSize =
        sizeof(struct pldm_msg_hdr) +
        sizeof(struct pldm_rde_operation_complete_req);
    std::array<uint8_t, requestSize> requestMsg{};
    auto request = reinterpret_cast<pldm_msg*>(requestMsg.data());

    EXPECT_EQ(encode_rde_operation_complete_req(instanceId, resourceId,
                                                operationId, request),
              PLDM_SUCCESS);
    // Test Headers
    EXPECT_EQ(request->hdr.instance_id, instanceId);
    EXPECT_EQ(request->hdr.type, PLDM_RDE);
    EXPECT_EQ(request->hdr.request, PLDM_REQUEST);
    EXPECT_EQ(request->hdr.command, PLDM_RDE_OPERATION_COMPLETE);

    auto reqPayload =
        reinterpret_cast<pldm_rde_operation_complete_req*>(request->payload);
    EXPECT_EQ(reqPayload->resource_id, resourceId);
    EXPECT_EQ(reqPayload->operation_id, operationId);
}

TEST(RDEOperationComplete, DecodeRequestSuccess)
{
    uint8_t instanceId = 11;
    uint32_t resourceId = 0x0001000;
    uint16_t operationId = 32770;
    constexpr size_t requestSize =
        sizeof(struct pldm_msg_hdr) +
        sizeof(struct pldm_rde_operation_complete_req);
    std::array<uint8_t, requestSize> requestMsg{};
    auto request = reinterpret_cast<pldm_msg*>(requestMsg.data());

    EXPECT_EQ(encode_rde_operation_complete_req(instanceId, resourceId,
                                                operationId, request),
              PLDM_SUCCESS);

    uint16_t returnOperationId;
    uint32_t returnResourceId;
    EXPECT_EQ(decode_rde_operation_status_req(
                  request, requestSize, &returnResourceId, &returnOperationId),
              PLDM_SUCCESS);

    EXPECT_EQ(returnOperationId, operationId);
    EXPECT_EQ(returnResourceId, resourceId);
}

TEST(RDEOperationComplete, EncodeResponseSuccess)
{
    uint8_t instanceId = 11;
    uint32_t completionCode = 0;
    constexpr size_t responseSize =
        sizeof(struct pldm_msg_hdr) +
        sizeof(struct pldm_rde_operation_complete_resp);
    std::array<uint8_t, responseSize> responseMsg{};
    auto response = reinterpret_cast<pldm_msg*>(responseMsg.data());

    EXPECT_EQ(encode_rde_operation_complete_resp(instanceId, completionCode,
                                                 response),
              PLDM_SUCCESS);

    EXPECT_EQ(response->hdr.instance_id, instanceId);
    EXPECT_EQ(response->hdr.request, PLDM_RESPONSE);
    EXPECT_EQ(response->hdr.type, PLDM_RDE);
    EXPECT_EQ(response->hdr.command, PLDM_RDE_OPERATION_COMPLETE);

    auto respPayload =
        reinterpret_cast<pldm_rde_operation_init_resp*>(response->payload);

    EXPECT_EQ(respPayload->completion_code, completionCode);
}

TEST(RDEOperationComplete, DecodeResponseSuccess)
{
    uint8_t instanceId = 11;
    uint32_t completionCode = 0;
    constexpr size_t responseSize =
        sizeof(struct pldm_msg_hdr) +
        sizeof(struct pldm_rde_operation_complete_resp);
    std::array<uint8_t, responseSize> responseMsg{};
    auto response = reinterpret_cast<pldm_msg*>(responseMsg.data());

    encode_rde_operation_complete_resp(instanceId, completionCode, response);

    uint8_t cc;
    EXPECT_EQ(decode_rde_operation_complete_resp(response, responseSize, &cc),
              PLDM_SUCCESS);
    EXPECT_EQ(cc, PLDM_SUCCESS);
}
TEST(RDEOperationStatus, EncodeRequestSuccess)
{
    uint8_t instanceId = 11;
    uint32_t resourceId = 0x0001000;
    uint16_t operationId = 32770;

    constexpr size_t requestSize = sizeof(struct pldm_msg_hdr) +
                                   sizeof(struct pldm_rde_operation_status_req);
    std::array<uint8_t, requestSize> requestMsg{};
    auto request = reinterpret_cast<pldm_msg*>(requestMsg.data());

    EXPECT_EQ(encode_rde_operation_status_req(instanceId, resourceId,
                                              operationId, request),
              PLDM_SUCCESS);

    // Test Header values
    EXPECT_EQ(request->hdr.instance_id, instanceId);
    EXPECT_EQ(request->hdr.request, PLDM_REQUEST);
    EXPECT_EQ(request->hdr.type, PLDM_RDE);
    EXPECT_EQ(request->hdr.command, PLDM_RDE_OPERATION_STATUS);

    // Test payload values
    auto reqPayload =
        reinterpret_cast<pldm_rde_operation_status_req*>(request->payload);
    EXPECT_EQ(reqPayload->resource_id, resourceId);
    EXPECT_EQ(reqPayload->operation_id, operationId);
}
TEST(RDEOperationStatus, DecodeRequestSuccess)
{
    uint8_t instanceId = 11;
    uint32_t resourceId = 0x0001000;
    uint16_t operationId = 32770;

    constexpr size_t requestSize = sizeof(struct pldm_msg_hdr) +
                                   sizeof(struct pldm_rde_operation_status_req);
    std::array<uint8_t, requestSize> requestMsg{};
    auto request = reinterpret_cast<pldm_msg*>(requestMsg.data());

    encode_rde_operation_status_req(instanceId, resourceId, operationId,
                                    request);
    uint32_t returnResourceId;
    uint16_t returnOperationId;
    EXPECT_EQ(decode_rde_operation_status_req(
                  request, requestSize, &returnResourceId, &returnOperationId),
              PLDM_SUCCESS);
    EXPECT_EQ(returnResourceId, resourceId);
    EXPECT_EQ(returnOperationId, operationId);
}
TEST(RDEOperationStatus, EncodeResponseSuccess)
{
    uint8_t completionCode = 0;
    uint8_t instanceId = 11;
    uint8_t operationStatus = PLDM_RDE_OPERATION_COMPLETED;
    uint8_t completionPercentage = 100;
    uint32_t completionTimeSeconds = 1;
    union pldm_rde_op_execution_flags operationExecutionFlags;
    operationExecutionFlags.byte = 0x01;
    uint32_t resultTransferHandle = 0x01;
    union pldm_rde_permission_flags permissionFlags;
    permissionFlags.byte = 0x02;
    enum pldm_rde_varstring_format etagFormat = PLDM_RDE_VARSTRING_UTF_8;
    const char etag[] = "etag";

    constexpr size_t responsePayloadLength =
        sizeof(struct pldm_msg_hdr) +
        sizeof(struct pldm_rde_operation_status_resp) + 4;
    std::array<uint8_t, responsePayloadLength> responseMsg{};
    auto response = reinterpret_cast<pldm_msg*>(responseMsg.data());
    EXPECT_EQ(encode_rde_operation_status_resp(
                  instanceId, completionCode, operationStatus,
                  completionPercentage, completionTimeSeconds,
                  &operationExecutionFlags, resultTransferHandle,
                  &permissionFlags, 0, etagFormat, etag, NULL, response),
              PLDM_SUCCESS);
    // verify header.
    EXPECT_EQ(response->hdr.instance_id, instanceId);
    EXPECT_EQ(response->hdr.request, 0);
    EXPECT_EQ(response->hdr.type, PLDM_RDE);
    EXPECT_EQ(response->hdr.command, PLDM_RDE_OPERATION_STATUS);
    // verify payload.
    auto respPayload =
        reinterpret_cast<pldm_rde_operation_status_resp*>(response->payload);
    EXPECT_EQ(respPayload->completion_code, completionCode);
    EXPECT_EQ(respPayload->operation_status, operationStatus);
    EXPECT_EQ(respPayload->completion_percentage, completionPercentage);
    EXPECT_EQ(respPayload->completion_time_seconds, completionTimeSeconds);
    EXPECT_EQ(respPayload->operation_execution_flags.byte,
              operationExecutionFlags.byte);
    EXPECT_EQ(respPayload->permission_flags.byte, permissionFlags.byte);
}
TEST(RDEOperationStatus, DecodeResponseSuccess)
{
    uint8_t completionCode = 0;
    uint8_t instanceId = 11;
    uint8_t operationStatus = PLDM_RDE_OPERATION_COMPLETED;
    uint8_t completionPercentage = 100;
    uint32_t completionTimeSeconds = 1;
    union pldm_rde_op_execution_flags operationExecutionFlags;
    operationExecutionFlags.byte = 0x01;
    uint32_t resultTransferHandle = 0x01;
    union pldm_rde_permission_flags permissionFlags;
    permissionFlags.byte = 0x02;
    enum pldm_rde_varstring_format etagFormat = PLDM_RDE_VARSTRING_UTF_8;
    const char etag[] = "etag";

    constexpr size_t responsePayloadLength =
        sizeof(struct pldm_msg_hdr) +
        sizeof(struct pldm_rde_operation_status_resp) + 4;
    std::array<uint8_t, responsePayloadLength> responseMsg{};
    auto response = reinterpret_cast<pldm_msg*>(responseMsg.data());
    EXPECT_EQ(encode_rde_operation_status_resp(
                  instanceId, completionCode, operationStatus,
                  completionPercentage, completionTimeSeconds,
                  &operationExecutionFlags, resultTransferHandle,
                  &permissionFlags, 0, etagFormat, etag, NULL, response),
              PLDM_SUCCESS);

    uint8_t returnCompletionCode;
    uint8_t returnOperationStatus;
    uint8_t returnCompletionPercentage;
    uint32_t returnCompletionTimeSeconds;
    uint32_t returnTransferHandle;
    uint32_t returnResponsePayloadLength;

    union pldm_rde_op_execution_flags* returnExecutionFlags =
        (union pldm_rde_op_execution_flags*)malloc(
            sizeof(union pldm_rde_op_execution_flags));
    union pldm_rde_permission_flags* returnPermissionFlags =
        (union pldm_rde_permission_flags*)malloc(
            sizeof(union pldm_rde_permission_flags));

    auto returnEtag = std::make_unique<pldm_rde_varstring>().get();
    auto responsePayload = std::make_unique<uint8_t>().get();
    EXPECT_EQ(decode_rde_operation_status_resp(
                  response, responsePayloadLength, &returnCompletionCode,
                  &returnCompletionPercentage, &returnOperationStatus,
                  &returnCompletionTimeSeconds, &returnTransferHandle,
                  &returnResponsePayloadLength, &returnPermissionFlags,
                  &returnExecutionFlags, &returnEtag, &responsePayload),
              PLDM_SUCCESS);

    EXPECT_EQ(returnCompletionCode, completionCode);
    EXPECT_EQ(returnCompletionPercentage, completionPercentage);
    EXPECT_EQ(returnOperationStatus, operationStatus);
    EXPECT_EQ(returnCompletionTimeSeconds, completionTimeSeconds);
    EXPECT_EQ(returnTransferHandle, resultTransferHandle);
    EXPECT_EQ(returnResponsePayloadLength, 0);
    EXPECT_EQ(returnPermissionFlags->byte, permissionFlags.byte);
    EXPECT_EQ(returnExecutionFlags->byte, operationExecutionFlags.byte);
    EXPECT_EQ(returnEtag->string_length_bytes,
              std::end(etag) - std::begin(etag));
}

TEST(RDEOperationEnumerate, EncodeRequest)
{
    uint8_t instanceId = 23;

    constexpr size_t requestSize = sizeof(struct pldm_msg_hdr);
    std::array<uint8_t, requestSize> requestMsg{};
    auto request = reinterpret_cast<pldm_msg*>(requestMsg.data());

    // When input msg of type struct pldm_msg * is NULL
    EXPECT_EQ(encode_rde_operation_enumerate_req(instanceId, NULL),
              PLDM_ERROR_INVALID_DATA);

    // When all inputs are valid
    EXPECT_EQ(encode_rde_operation_enumerate_req(instanceId, request),
              PLDM_SUCCESS);

    // Test Header values
    EXPECT_EQ(request->hdr.instance_id, instanceId);
    EXPECT_EQ(request->hdr.request, PLDM_REQUEST);
    EXPECT_EQ(request->hdr.type, PLDM_RDE);
    EXPECT_EQ(request->hdr.command, PLDM_RDE_OPERATION_ENUMERATE);
}

TEST(RDEOperationEnumerate, DecodeRequest)
{
    uint8_t instanceId = 25;

    constexpr size_t requestSize = sizeof(struct pldm_msg_hdr);
    std::array<uint8_t, requestSize> requestMsg{};
    auto request = reinterpret_cast<pldm_msg*>(requestMsg.data());

    encode_rde_operation_enumerate_req(instanceId, request);

    // When input msg of type struct pldm_msg * is NULL
    EXPECT_EQ(decode_rde_operation_enumerate_req(
                  NULL, requestSize - sizeof(struct pldm_msg_hdr)),
              PLDM_ERROR_INVALID_DATA);

    // When when the request size in non-zero indicating a request payload is
    // present
    EXPECT_EQ(decode_rde_operation_enumerate_req(request, 10),
              PLDM_ERROR_INVALID_LENGTH);

    // When all inputs are valid
    EXPECT_EQ(decode_rde_operation_enumerate_req(
                  request, requestSize - sizeof(struct pldm_msg_hdr)),
              PLDM_SUCCESS);
}

TEST(RDEOperationEnumerate, EncodeResponse)
{
    uint8_t instanceId = 29;
    uint8_t completionCode = 0;

    constexpr size_t responsePayloadLength =
        sizeof(struct pldm_msg_hdr) +
        PLDM_RDE_OPERATION_ENUMERATE_RESP_HDR_SIZE;
    std::array<uint8_t, responsePayloadLength> responseMsg{};
    auto response = reinterpret_cast<pldm_msg*>(responseMsg.data());

    // When input msg of type struct pldm_msg* is NULL
    EXPECT_EQ(
        encode_rde_operation_enumerate_resp(instanceId, completionCode, NULL),
        PLDM_ERROR_INVALID_DATA);

    // When all the inputs are valid
    EXPECT_EQ(encode_rde_operation_enumerate_resp(instanceId, completionCode,
                                                  response),
              PLDM_SUCCESS);

    // verify header.
    EXPECT_EQ(response->hdr.instance_id, instanceId);
    EXPECT_EQ(response->hdr.request, 0);
    EXPECT_EQ(response->hdr.type, PLDM_RDE);
    EXPECT_EQ(response->hdr.command, PLDM_RDE_OPERATION_ENUMERATE);

    // verify payload.
    auto respPayload =
        reinterpret_cast<pldm_rde_operation_enumerate_resp*>(response->payload);
    EXPECT_EQ(respPayload->completion_code, completionCode);
    EXPECT_EQ(respPayload->operation_count, 0);
}

TEST(RDEOperationEnumerate, UpdateEncodedResponse)
{
    uint8_t instanceId = 29;
    uint8_t completionCode = 0;
    const uint16_t operationCount = 3;

    constexpr size_t responsePayloadLength =
        sizeof(struct pldm_msg_hdr) +
        PLDM_RDE_OPERATION_ENUMERATE_RESP_HDR_SIZE +
        (operationCount *
         sizeof(struct pldm_rde_operation_enumerate_operation_data));
    std::array<uint8_t, responsePayloadLength> responseMsg{};
    auto response = reinterpret_cast<pldm_msg*>(responseMsg.data());

    EXPECT_EQ(encode_rde_operation_enumerate_resp(instanceId, completionCode,
                                                  response),
              PLDM_SUCCESS);

    struct pldm_rde_operation_enumerate_operation_data
        enumerateData[operationCount] = {
            {.resource_id = 54314,
             .operation_id = 2,
             .operation_type = PLDM_RDE_OPERATION_READ},
            {.resource_id = 32456,
             .operation_id = 1,
             .operation_type = PLDM_RDE_OPERATION_UPDATE},
            {.resource_id = 76543,
             .operation_id = 6,
             .operation_type = PLDM_RDE_OPERATION_ACTION}};

    // When input msg of type struct pldm_msg* is NULL
    EXPECT_EQ(update_encoded_rde_operation_enumerate_resp(
                  enumerateData[0].resource_id, enumerateData[0].operation_id,
                  enumerateData[0].operation_type, NULL),
              PLDM_ERROR_INVALID_DATA);

    // Update the encoded payload with enumerate_data one by one and verify that
    // response payload metadata gets updated accordingly
    for (int i = 0; i < operationCount; i++)
    {

        EXPECT_EQ(update_encoded_rde_operation_enumerate_resp(
                      enumerateData[i].resource_id,
                      enumerateData[i].operation_id,
                      enumerateData[i].operation_type, response),
                  PLDM_SUCCESS);
        auto respPayload2 =
            reinterpret_cast<pldm_rde_operation_enumerate_resp*>(
                response->payload);
        EXPECT_EQ(respPayload2->completion_code, completionCode);
        EXPECT_EQ(respPayload2->operation_count, i + 1);
    }

    // verify payload.
    auto respPayload3 =
        reinterpret_cast<pldm_rde_operation_enumerate_resp*>(response->payload);
    EXPECT_EQ(respPayload3->completion_code, completionCode);
    EXPECT_EQ(respPayload3->operation_count, operationCount);

    struct pldm_rde_operation_enumerate_operation_data* enumerateRespData =
        (struct pldm_rde_operation_enumerate_operation_data*)
            respPayload3->var_data;

    // Verify that the enumerate_data in the response is updated
    for (int i = 0; i < operationCount; i++)
    {
        EXPECT_EQ(enumerateRespData[i].resource_id,
                  enumerateData[i].resource_id);
        EXPECT_EQ(enumerateRespData[i].operation_id,
                  enumerateData[i].operation_id);
        EXPECT_EQ(enumerateRespData[i].operation_type,
                  enumerateData[i].operation_type);
    }
}

TEST(RDEOperationEnumerate, DecodeResponse)
{
    uint8_t instanceId = 29;
    uint8_t completionCodeInput = 0;
    const uint16_t operationCountInput = 3;

    uint8_t completionCodeOutput = 0;
    uint16_t operationCountOutput = 0;

    struct pldm_rde_operation_enumerate_operation_data
        enumerateRespData[operationCountInput];

    struct pldm_rde_operation_enumerate_operation_data
        enumerateData[operationCountInput] = {
            {.resource_id = 54314,
             .operation_id = 2,
             .operation_type = PLDM_RDE_OPERATION_READ},
            {.resource_id = 32456,
             .operation_id = 1,
             .operation_type = PLDM_RDE_OPERATION_UPDATE},
            {.resource_id = 76543,
             .operation_id = 6,
             .operation_type = PLDM_RDE_OPERATION_ACTION}};

    constexpr size_t responsePayloadLength =
        sizeof(struct pldm_msg_hdr) +
        PLDM_RDE_OPERATION_ENUMERATE_RESP_HDR_SIZE +
        (operationCountInput *
         sizeof(struct pldm_rde_operation_enumerate_operation_data));
    std::array<uint8_t, responsePayloadLength> responseMsg{};
    auto response = reinterpret_cast<pldm_msg*>(responseMsg.data());

    // When input msg of type struct pldm_msg* is NULL
    EXPECT_EQ(decode_rde_operation_enumerate_resp(
                  NULL, &completionCodeOutput, &operationCountOutput,
                  enumerateRespData, operationCountInput),
              PLDM_ERROR_INVALID_DATA);

    // When input completion_code is NULL
    EXPECT_EQ(decode_rde_operation_enumerate_resp(
                  response, NULL, &operationCountOutput, enumerateRespData,
                  operationCountInput),
              PLDM_ERROR_INVALID_DATA);

    // When input operation_count is NULL
    EXPECT_EQ(decode_rde_operation_enumerate_resp(
                  response, &completionCodeOutput, NULL, enumerateRespData,
                  operationCountInput),
              PLDM_ERROR_INVALID_DATA);

    // When input operation_data is NULL
    EXPECT_EQ(decode_rde_operation_enumerate_resp(
                  response, &completionCodeOutput, &operationCountOutput, NULL,
                  operationCountInput),
              PLDM_ERROR_INVALID_DATA);

    // When input array size is smaller that the actual operation count
    EXPECT_EQ(decode_rde_operation_enumerate_resp(
                  response, &completionCodeOutput, &operationCountOutput,
                  enumerateRespData, operationCountInput - 1),
              PLDM_SUCCESS);

    // When all the inputs are valid
    EXPECT_EQ(encode_rde_operation_enumerate_resp(
                  instanceId, completionCodeInput, response),
              PLDM_SUCCESS);

    // Update the encoded payload with enumerate_data one by one
    for (int i = 0; i < operationCountInput; i++)
    {
        EXPECT_EQ(update_encoded_rde_operation_enumerate_resp(
                      enumerateData[i].resource_id,
                      enumerateData[i].operation_id,
                      enumerateData[i].operation_type, response),
                  PLDM_SUCCESS);
    }

    EXPECT_EQ(decode_rde_operation_enumerate_resp(
                  response, &completionCodeOutput, &operationCountOutput,
                  enumerateRespData, operationCountInput),
              PLDM_SUCCESS);

    auto respPayload1 =
        reinterpret_cast<pldm_rde_operation_enumerate_resp*>(response->payload);
    EXPECT_EQ(respPayload1->completion_code, completionCodeInput);
    EXPECT_EQ(respPayload1->operation_count, operationCountInput);

    // Verify that the enumerate_data in the response is updated
    for (int i = 0; i < operationCountInput; i++)
    {
        EXPECT_EQ(enumerateRespData[i].resource_id,
                  enumerateData[i].resource_id);
        EXPECT_EQ(enumerateRespData[i].operation_id,
                  enumerateData[i].operation_id);
        EXPECT_EQ(enumerateRespData[i].operation_type,
                  enumerateData[i].operation_type);
    }

    // When encoder encounters error and sets error completion code
    EXPECT_EQ(
        encode_rde_operation_enumerate_resp(instanceId, PLDM_ERROR, response),
        PLDM_SUCCESS);
    EXPECT_EQ(decode_rde_operation_enumerate_resp(
                  response, &completionCodeOutput, &operationCountOutput,
                  enumerateRespData, operationCountInput),
              PLDM_SUCCESS);

    auto respPayload2 =
        reinterpret_cast<pldm_rde_operation_enumerate_resp*>(response->payload);
    EXPECT_EQ(respPayload2->completion_code, PLDM_ERROR);
    EXPECT_EQ(respPayload2->operation_count, 0);
}

TEST(SupplyCustomRequestParameters, EncodeRequestSuccess)
{
    uint32_t resource_id = 0x0001001;
    uint8_t instance_id = 12;
    uint16_t operation_id = 32770;
    uint16_t link_expand = 0x01;
    uint16_t collection_skip = 0x02;
    uint16_t collection_top = 0x03;
    uint16_t pagination_offset = 0x4;


	pldm_rde_etag_operation etag_operation = PLDM_RDE_ETAG_IF_NONE_MATCH;
	uint8_t etag_count = 1;
	enum pldm_rde_varstring_format etag_formats[1] = { PLDM_RDE_VARSTRING_UTF_8};
    char etag1[6] = "etag1";
	char *etags[1] = { etag1 };

	uint8_t header_count = 1 ;
	enum pldm_rde_varstring_format hdrname_formats[1] = { PLDM_RDE_VARSTRING_UTF_8 };
	char *hdrnames[1] = { PLDM_RDE_EXPAND_TYPE };
	enum pldm_rde_varstring_format hdrparam_formats[1] = { PLDM_RDE_VARSTRING_UTF_8 };
	char *hdrparams[1] = { EXPAND_DOT };

    constexpr size_t requestSize =
        sizeof(struct pldm_msg_hdr) +
        sizeof(struct pldm_supply_custom_request_parameters_req) + 43;

    std::array<uint8_t, requestSize> requestMsg{};
    auto request = reinterpret_cast<pldm_msg*>(requestMsg.data());

    EXPECT_EQ(encode_supply_custom_request_parameters_req(
                  instance_id, resource_id, operation_id, link_expand,
                  collection_skip, collection_top, pagination_offset, request),
                  PLDM_SUCCESS);

    uint8_t encode_offset = 0;
    EXPECT_EQ(encode_etags_in_supply_custom_request_parameters_req(
                  etag_operation, etag_count, etag_formats, etags,
                  &encode_offset, request),
                  PLDM_SUCCESS);


    EXPECT_EQ(encode_headers_in_supply_custom_request_parameters_req(
                  header_count, hdrname_formats, hdrnames, hdrparam_formats,
                  hdrparams, &encode_offset, request),
                  PLDM_SUCCESS);
    // Test Header values
    EXPECT_EQ(request->hdr.instance_id, instance_id);
    EXPECT_EQ(request->hdr.request, PLDM_REQUEST);
    EXPECT_EQ(request->hdr.type, PLDM_RDE);
    EXPECT_EQ(request->hdr.command, PLDM_SUPPLY_CUSTOM_REQUEST_PARAMETERS);

    // Test payload values
    auto reqPayload =
        reinterpret_cast<pldm_supply_custom_request_parameters_req*>(
            request->payload);
    EXPECT_EQ(reqPayload->resource_id, resource_id);
    EXPECT_EQ(reqPayload->operation_id, operation_id);
    EXPECT_EQ(reqPayload->link_expand, link_expand);
    EXPECT_EQ(reqPayload->collection_skip, collection_skip);
    EXPECT_EQ(reqPayload->collection_top, collection_top);
    EXPECT_EQ(reqPayload->pagination_offset, pagination_offset);
    EXPECT_EQ(reqPayload->etag_operation, PLDM_RDE_ETAG_IF_NONE_MATCH);
    EXPECT_EQ(reqPayload->etag_count, etag_count);


    int offset = 0;
    for (uint8_t i = 0; i < etag_count; i++)
    {
        EXPECT_EQ(reqPayload->var_data[offset++], etag_formats[i]);
        size_t len = strlen(etags[i]) + 1;
        EXPECT_EQ(reqPayload->var_data[offset++], len);
        EXPECT_EQ(0, memcmp(reqPayload->var_data + offset,
                            etags[i], len));
        offset += len;
    }

    EXPECT_EQ(reqPayload->var_data[offset++], header_count);
    for (uint8_t i = 0; i < header_count; i++)
    {
        EXPECT_EQ(reqPayload->var_data[offset++], hdrname_formats[i]);
        size_t len = strlen(hdrnames[i]) + 1;
        EXPECT_EQ(reqPayload->var_data[offset++], len);
        EXPECT_EQ(0, memcmp(reqPayload->var_data + offset,
                            hdrnames[i], len));
        offset += len;
    }

    for (uint8_t i = 0; i < header_count; i++)
    {
        EXPECT_EQ(reqPayload->var_data[offset++], hdrparam_formats[i]);
        size_t len = strlen(hdrparams[i]) + 1;
        EXPECT_EQ(reqPayload->var_data[offset++], len);
        EXPECT_EQ(0, memcmp(reqPayload->var_data + offset,
                            hdrparams[i], len));
        offset += len;
    }
}

TEST(SupplyCustomRequestParameters, DecodeResponseSuccess)
{
    uint8_t completionCode = 0;
    uint8_t instanceId = 11;
    uint8_t operationStatus = PLDM_RDE_OPERATION_COMPLETED;
    uint8_t completionPercentage = 100;
    uint32_t completionTimeSeconds = 1;
    union pldm_rde_op_execution_flags operationExecutionFlags;
    operationExecutionFlags.byte = 0x04;
    uint32_t resultTransferHandle = 0x01;
    union pldm_rde_permission_flags permissionFlags;
    permissionFlags.byte = 0x02;
    const uint32_t responsePayloadLength = 12;
    const uint8_t* varPayload = (const uint8_t*)"TEST_PAYLOAD";

    // Encode
	enum pldm_rde_varstring_format etag_format = PLDM_RDE_VARSTRING_UTF_8;
    char etag1[6] = "etag1";

    constexpr size_t responseLength =
        sizeof(struct pldm_msg_hdr) +
        sizeof(struct pldm_supply_custom_request_parameters_resp) + 8 +
        responsePayloadLength;
    std::array<uint8_t, responseLength> responseMsg{};
    auto response = reinterpret_cast<pldm_msg*>(responseMsg.data());
    EXPECT_EQ(encode_supply_custom_request_parameters_resp(
                  instanceId, completionCode, operationStatus,
                  completionPercentage, completionTimeSeconds,
                  &operationExecutionFlags, resultTransferHandle,
                  &permissionFlags, responsePayloadLength, etag_format, etag1, varPayload,
                  response),
              PLDM_SUCCESS);

    // Decode
    uint8_t returnCompletionCode;
    uint8_t returnOperationStatus;
    uint8_t returnCompletionPercentage;
    uint32_t returnCompletionTimeSeconds;
    uint32_t returnTransferHandle;
    uint32_t returnResponsePayloadLength;

    union pldm_rde_op_execution_flags* returnExecutionFlags =
        (union pldm_rde_op_execution_flags*)malloc(
            sizeof(union pldm_rde_op_execution_flags));
    union pldm_rde_permission_flags* returnPermissionFlags =
        (union pldm_rde_permission_flags*)malloc(
            sizeof(union pldm_rde_permission_flags));

    struct pldm_rde_varstring* returnEtag =
        (pldm_rde_varstring*)malloc(sizeof(struct pldm_rde_varstring) + 6);
    // auto returnEtag = std::make_unique<pldm_rde_varstring>().get();
    auto responsePayload = std::make_unique<uint8_t>().get();
    EXPECT_EQ(decode_supply_custom_request_parameters_resp(
                  response, responseLength, &returnCompletionCode,
                  &returnCompletionPercentage, &returnOperationStatus,
                  &returnCompletionTimeSeconds, &returnTransferHandle,
                  &returnResponsePayloadLength, &returnPermissionFlags,
                  &returnExecutionFlags, &returnEtag, &responsePayload),
              PLDM_SUCCESS);

    EXPECT_EQ(returnCompletionCode, completionCode);
    EXPECT_EQ(returnCompletionPercentage, completionPercentage);
    EXPECT_EQ(returnOperationStatus, operationStatus);
    EXPECT_EQ(returnCompletionTimeSeconds, completionTimeSeconds);
    EXPECT_EQ(returnTransferHandle, resultTransferHandle);
    EXPECT_EQ(returnResponsePayloadLength, responsePayloadLength);
    EXPECT_EQ(returnPermissionFlags->byte, permissionFlags.byte);
    EXPECT_EQ(returnExecutionFlags->byte, operationExecutionFlags.byte);
    EXPECT_EQ(returnEtag->string_format, etag_format);
    EXPECT_EQ(returnEtag->string_length_bytes, strlen(etag1) + 1);
    EXPECT_EQ(0, memcmp(returnEtag->string_data, etag1,
                        strlen(etag1) + 1));
    EXPECT_EQ(0, memcmp(responsePayload, varPayload, responsePayloadLength));
}

