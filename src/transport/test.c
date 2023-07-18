#include "container-of.h"
#include "transport.h"
#include "libpldm/transport/test.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

struct pldm_transport_test {
	struct pldm_transport transport;
	const struct pldm_transport_test_descriptor *seq;
	size_t count;
	size_t cursor;
	int timerfd;
};

#define transport_to_test(ptr)                                                 \
	container_of(ptr, struct pldm_transport_test, transport)

LIBPLDM_ABI_TESTING
struct pldm_transport *pldm_transport_test_core(struct pldm_transport_test *ctx)
{
	return &ctx->transport;
}

#ifdef PLDM_HAS_POLL
#include <poll.h>
LIBPLDM_ABI_TESTING
int pldm_transport_test_init_pollfd(struct pldm_transport *t,
				    struct pollfd *pollfd)
{
	static const struct itimerspec disable = {
		.it_value = { 0, 0 },
		.it_interval = { 0, 0 },
	};
	struct pldm_transport_test *ctx = transport_to_test(t);
	const struct pldm_transport_test_descriptor *desc;
	int rc;

	rc = timerfd_settime(ctx->timerfd, 0, &disable, NULL);
	if (rc < 0) {
		return PLDM_REQUESTER_POLL_FAIL;
	}

	if (ctx->cursor >= ctx->count) {
		return PLDM_REQUESTER_POLL_FAIL;
	}

	desc = &ctx->seq[ctx->cursor];

	if (desc->type != PLDM_TRANSPORT_TEST_ELEMENT_LATENCY) {
		return PLDM_REQUESTER_POLL_FAIL;
	}

	rc = timerfd_settime(ctx->timerfd, 0, &desc->latency, NULL);
	if (rc < 0) {
		return PLDM_REQUESTER_POLL_FAIL;
	}

	pollfd->fd = ctx->timerfd;
	pollfd->events = POLLIN;

	ctx->cursor++;

	return 0;
}
#endif

static pldm_requester_rc_t pldm_transport_test_recv(struct pldm_transport *t,
						    pldm_tid_t tid,
						    void **pldm_resp_msg,
						    size_t *resp_msg_len)
{
	struct pldm_transport_test *ctx = transport_to_test(t);
	const struct pldm_transport_test_descriptor *desc;
	void *msg;

	(void)tid;

	if (ctx->cursor >= ctx->count) {
		return PLDM_REQUESTER_RECV_FAIL;
	}

	desc = &ctx->seq[ctx->cursor];

	if (desc->type != PLDM_TRANSPORT_TEST_ELEMENT_MSG_RECV) {
		return PLDM_REQUESTER_RECV_FAIL;
	}

	msg = malloc(desc->recv_msg.len);
	if (!msg) {
		return PLDM_REQUESTER_RECV_FAIL;
	}

	memcpy(msg, desc->recv_msg.msg, desc->recv_msg.len);
	*pldm_resp_msg = msg;
	*resp_msg_len = desc->recv_msg.len;

	ctx->cursor++;

	return PLDM_REQUESTER_SUCCESS;
}

static pldm_requester_rc_t pldm_transport_test_send(struct pldm_transport *t,
						    pldm_tid_t tid,
						    const void *pldm_req_msg,
						    size_t req_msg_len)
{
	struct pldm_transport_test *ctx = transport_to_test(t);
	const struct pldm_transport_test_descriptor *desc;

	if (ctx->cursor > ctx->count) {
		return PLDM_REQUESTER_SEND_FAIL;
	}

	desc = &ctx->seq[ctx->cursor];

	if (desc->type != PLDM_TRANSPORT_TEST_ELEMENT_MSG_SEND) {
		return PLDM_REQUESTER_SEND_FAIL;
	}

	if (desc->send_msg.dst != tid) {
		return PLDM_REQUESTER_SEND_FAIL;
	}

	if (desc->send_msg.len != req_msg_len) {
		return PLDM_REQUESTER_SEND_FAIL;
	}

	if (memcmp(desc->send_msg.msg, pldm_req_msg, req_msg_len)) {
		return PLDM_REQUESTER_SEND_FAIL;
	}

	ctx->cursor++;

	return PLDM_REQUESTER_SUCCESS;
}

LIBPLDM_ABI_TESTING
int pldm_transport_test_init(struct pldm_transport_test **ctx,
			     const struct pldm_transport_test_descriptor *seq,
			     size_t count)
{
	int rc;

	if (!ctx || *ctx) {
		return -EINVAL;
	}

	struct pldm_transport_test *test = malloc(sizeof(*test));
	if (!test) {
		return -ENOMEM;
	}

	test->transport.name = "TEST";
	test->transport.version = 1;
	test->transport.recv = pldm_transport_test_recv;
	test->transport.send = pldm_transport_test_send;
	test->transport.init_pollfd = pldm_transport_test_init_pollfd;
	test->seq = seq;
	test->count = count;
	test->cursor = 0;
	test->timerfd = timerfd_create(CLOCK_MONOTONIC, 0);
	if (test->timerfd < 0) {
		rc = -errno;
		goto cleanup_test;
	}

	*ctx = test;

	return 0;

cleanup_test:
	free(test);
	return rc;
}

LIBPLDM_ABI_TESTING
void pldm_transport_test_destroy(struct pldm_transport_test *ctx)
{
	close(ctx->timerfd);
	free(ctx);
}
