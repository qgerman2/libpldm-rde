#include "libpldm/transport.h"

#include "libpldm/transport/test.h"

#include <gtest/gtest.h>

TEST(Transport, create)
{
    struct pldm_transport_test* test = NULL;

    EXPECT_EQ(pldm_transport_test_init(&test, NULL, 0), 0);
    EXPECT_NE(pldm_transport_test_core(test), nullptr);
    pldm_transport_test_destroy(test);
}

TEST(Transport, send_one)
{
    const uint8_t msg[] = {0x80, 0x00, 0x01, 0x01};
    const struct pldm_transport_test_descriptor seq[] = {
        {
            .type = PLDM_TRANSPORT_TEST_ELEMENT_MSG_SEND,
            .send_msg =
                {
                    .dst = 1,
                    .msg = msg,
                    .len = sizeof(msg),
                },
        },
    };
    struct pldm_transport_test* test = NULL;
    struct pldm_transport* ctx;
    int rc;

    EXPECT_EQ(pldm_transport_test_init(&test, seq, 1), 0);
    ctx = pldm_transport_test_core(test);
    rc = pldm_transport_send_msg(ctx, 1, msg, sizeof(msg));
    EXPECT_EQ(rc, PLDM_REQUESTER_SUCCESS);
    pldm_transport_test_destroy(test);
}

TEST(Transport, recv_one)
{
    uint8_t msg[] = {0x00, 0x00, 0x01, 0x00};
    const struct pldm_transport_test_descriptor seq[] = {
        {
            .type = PLDM_TRANSPORT_TEST_ELEMENT_MSG_RECV,
            .recv_msg =
                {
                    .src = 1,
                    .msg = msg,
                    .len = sizeof(msg),
                },
        },
    };
    struct pldm_transport_test* test = NULL;
    struct pldm_transport* ctx;
    void* recvd;
    size_t len;
    int rc;

    EXPECT_EQ(pldm_transport_test_init(&test, seq, 1), 0);
    ctx = pldm_transport_test_core(test);
    rc = pldm_transport_recv_msg(ctx, 1, &recvd, &len);
    ASSERT_EQ(rc, PLDM_REQUESTER_SUCCESS);
    EXPECT_EQ(len, sizeof(msg));
    EXPECT_EQ(memcmp(recvd, msg, len), 0);
    pldm_transport_test_destroy(test);
}

TEST(Transport, send_recv_one)
{
    uint8_t req[] = {0x80, 0x00, 0x01, 0x01};
    uint8_t resp[] = {0x00, 0x00, 0x01, 0x00};
    const struct pldm_transport_test_descriptor seq[] = {
        {
            .type = PLDM_TRANSPORT_TEST_ELEMENT_MSG_SEND,
            .send_msg =
                {
                    .dst = 1,
                    .msg = req,
                    .len = sizeof(req),
                },
        },
        {
            .type = PLDM_TRANSPORT_TEST_ELEMENT_LATENCY,
            .latency =
                {
                    .it_interval = {0, 0},
                    .it_value = {1, 0},
                },
        },
        {
            .type = PLDM_TRANSPORT_TEST_ELEMENT_MSG_RECV,
            .recv_msg =
                {
                    .src = 1,
                    .msg = resp,
                    .len = sizeof(resp),
                },
        },
    };
    struct pldm_transport_test* test = NULL;
    struct pldm_transport* ctx;
    size_t len;
    void* msg;
    int rc;

    EXPECT_EQ(pldm_transport_test_init(&test, seq, 3), 0);
    ctx = pldm_transport_test_core(test);
    rc = pldm_transport_send_recv_msg(ctx, 1, req, sizeof(req), &msg, &len);
    ASSERT_EQ(rc, PLDM_REQUESTER_SUCCESS);
    EXPECT_EQ(len, sizeof(resp));
    EXPECT_EQ(memcmp(msg, resp, len), 0);
    pldm_transport_test_destroy(test);
}
