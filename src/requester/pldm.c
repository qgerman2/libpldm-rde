/* SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later */
#include "libpldm/mctp.h"
#include <libpldm/base.h>
#include <libpldm/pldm.h>
#include <libpldm/transport.h>

#include <bits/types/struct_iovec.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <stdio.h>
#include "mctp-defines.h"
#include <errno.h>

/* Temporary for old api */
#include <libpldm/transport/mctp-demux.h>
extern int
pldm_transport_mctp_demux_get_socket_fd(struct pldm_transport_mctp_demux *ctx);
extern struct pldm_transport_mctp_demux *
pldm_transport_mctp_demux_init_with_fd(int mctp_fd);

/* ---  old APIS written in terms of the new API -- */
/*
 * pldm_open returns the file descriptor to the MCTP socket, which needs to
 * persist over api calls (so a consumer can poll it for incoming messages).
 * So we need a global variable to store the transport struct
 */
static struct pldm_transport_mctp_demux *open_transport;

LIBPLDM_ABI_DEPRECATED
pldm_requester_rc_t pldm_open(void)
{
	int fd = PLDM_REQUESTER_OPEN_FAIL;

	if (open_transport) {
		fd = pldm_transport_mctp_demux_get_socket_fd(open_transport);

		/* If someone has externally issued close() on fd then we need to start again. Use
		 * `fcntl(..., F_GETFD)` to test whether fd is valid. */
		if (fd < 0 || fcntl(fd, F_GETFD) < 0) {
			pldm_close();
		}
	}

	/* We retest open_transport as it may have been set to NULL by pldm_close() above. */
	if (!open_transport) {
		struct pldm_transport_mctp_demux *demux = NULL;

		if (pldm_transport_mctp_demux_init(&demux) < 0) {
			return PLDM_REQUESTER_OPEN_FAIL;
		}

		open_transport = demux;

		fd = pldm_transport_mctp_demux_get_socket_fd(open_transport);
	}

	return fd;
}

/* This macro does the setup and teardown required for the old API to use the
 * new API. Since the setup/teardown logic is the same for all four send/recv
 * functions, it makes sense to only define it once. */
#define PLDM_REQ_FN(eid, fd, fn, rc, ...)                                        \
	do {                                                                     \
		struct pldm_transport_mctp_demux *demux;                         \
		bool using_open_transport = false;                               \
		pldm_tid_t tid = eid;                                            \
		struct pldm_transport *ctx;                                      \
		/* The fd can be for a socket we opened or one the consumer    \
		 * opened. */ \
		if (open_transport &&                                            \
		    mctp_fd == pldm_transport_mctp_demux_get_socket_fd(          \
				       open_transport)) {                        \
			using_open_transport = true;                             \
			demux = open_transport;                                  \
		} else {                                                         \
			demux = pldm_transport_mctp_demux_init_with_fd(fd);      \
			if (!demux) {                                            \
				rc = PLDM_REQUESTER_OPEN_FAIL;                   \
				goto transport_out;                              \
			}                                                        \
		}                                                                \
		ctx = pldm_transport_mctp_demux_core(demux);                     \
		rc = pldm_transport_mctp_demux_map_tid(demux, tid, eid);         \
		if (rc) {                                                        \
			rc = PLDM_REQUESTER_OPEN_FAIL;                           \
			goto transport_out;                                      \
		}                                                                \
		rc = fn(ctx, tid, __VA_ARGS__);                                  \
	transport_out:                                                           \
		if (!using_open_transport) {                                     \
			pldm_transport_mctp_demux_destroy(demux);                \
		}                                                                \
		break;                                                           \
	} while (0)

LIBPLDM_ABI_DEPRECATED
pldm_requester_rc_t pldm_recv_any(mctp_eid_t eid, int mctp_fd,
				  uint8_t **pldm_resp_msg, size_t *resp_msg_len)
{
	pldm_requester_rc_t rc = 0;

	struct pldm_transport_mctp_demux *demux;
	bool using_open_transport = false;
	pldm_tid_t tid = eid;
	struct pldm_transport *ctx;
	/* The fd can be for a socket we opened or one the consumer
	 * opened. */
	if (open_transport &&
	    mctp_fd ==
		    pldm_transport_mctp_demux_get_socket_fd(open_transport)) {
		using_open_transport = true;
		demux = open_transport;
	} else {
		demux = pldm_transport_mctp_demux_init_with_fd(mctp_fd);
		if (!demux) {
			rc = PLDM_REQUESTER_OPEN_FAIL;
			goto transport_out;
		}
	}
	ctx = pldm_transport_mctp_demux_core(demux);
	rc = pldm_transport_mctp_demux_map_tid(demux, tid, eid);
	if (rc) {
		rc = PLDM_REQUESTER_OPEN_FAIL;
		goto transport_out;
	}
	/* TODO this is the only change, can we work this into the macro? */
	rc = pldm_transport_recv_msg(ctx, &tid, (void **)pldm_resp_msg,
				     resp_msg_len);

	struct pldm_msg_hdr *hdr = (struct pldm_msg_hdr *)(*pldm_resp_msg);
	if (rc != PLDM_REQUESTER_SUCCESS) {
		return rc;
	}
	if (hdr && (hdr->request || hdr->datagram)) {
		free(*pldm_resp_msg);
		*pldm_resp_msg = NULL;
		return PLDM_REQUESTER_NOT_RESP_MSG;
	}
	uint8_t pldm_cc = 0;
	if (*resp_msg_len < (sizeof(struct pldm_msg_hdr) + sizeof(pldm_cc))) {
		free(*pldm_resp_msg);
		*pldm_resp_msg = NULL;
		return PLDM_REQUESTER_RESP_MSG_TOO_SMALL;
	}

transport_out:
	if (!using_open_transport) {
		pldm_transport_mctp_demux_destroy(demux);
	}

	return rc;
}

LIBPLDM_ABI_DEPRECATED
pldm_requester_rc_t pldm_recv(mctp_eid_t eid, int mctp_fd,
			      __attribute__((unused)) uint8_t instance_id,
			      uint8_t **pldm_resp_msg, size_t *resp_msg_len)
{
	pldm_requester_rc_t rc =
		pldm_recv_any(eid, mctp_fd, pldm_resp_msg, resp_msg_len);
	struct pldm_msg_hdr *hdr = (struct pldm_msg_hdr *)(*pldm_resp_msg);
	if (rc == PLDM_REQUESTER_SUCCESS && hdr &&
	    hdr->instance_id != instance_id) {
		free(*pldm_resp_msg);
		*pldm_resp_msg = NULL;
		return PLDM_REQUESTER_INSTANCE_ID_MISMATCH;
	}
	return rc;
}

LIBPLDM_ABI_DEPRECATED
pldm_requester_rc_t pldm_send_recv(mctp_eid_t eid, int mctp_fd,
				   const uint8_t *pldm_req_msg,
				   size_t req_msg_len, uint8_t **pldm_resp_msg,
				   size_t *resp_msg_len)
{
	pldm_requester_rc_t rc = 0;
	struct pldm_msg_hdr *hdr = (struct pldm_msg_hdr *)pldm_req_msg;
	if (hdr && !hdr->request) {
		return PLDM_REQUESTER_NOT_REQ_MSG;
	}
	PLDM_REQ_FN(eid, mctp_fd, pldm_transport_send_recv_msg, rc,
		    pldm_req_msg, req_msg_len, (void **)pldm_resp_msg,
		    resp_msg_len);
	if (rc != PLDM_REQUESTER_SUCCESS) {
		return rc;
	}
	hdr = (struct pldm_msg_hdr *)(*pldm_resp_msg);
	if (hdr && (hdr->request || hdr->datagram)) {
		free(*pldm_resp_msg);
		*pldm_resp_msg = NULL;
		return PLDM_REQUESTER_NOT_RESP_MSG;
	}
	return rc;
}

LIBPLDM_ABI_DEPRECATED
pldm_requester_rc_t pldm_send(mctp_eid_t eid, int mctp_fd,
			      const uint8_t *pldm_req_msg, size_t req_msg_len)
{
	pldm_requester_rc_t rc = 0;
	struct pldm_msg_hdr *hdr = (struct pldm_msg_hdr *)pldm_req_msg;
	if (!hdr->request) {
		return PLDM_REQUESTER_NOT_REQ_MSG;
	}
	PLDM_REQ_FN(eid, mctp_fd, pldm_transport_send_msg, rc,
		    (void *)pldm_req_msg, req_msg_len);
	return rc;
}

/* Adding this here for completeness in the case we can't smoothly
 * transition apps over to the new api */
LIBPLDM_ABI_DEPRECATED
void pldm_close(void)
{
	if (open_transport) {
		pldm_transport_mctp_demux_destroy(open_transport);
	}
	open_transport = NULL;
}

LIBPLDM_ABI_STABLE
pldm_requester_rc_t pldm_send_at_network(mctp_eid_t eid, int network_id,
                                         int mctp_fd,
                                         const uint8_t* pldm_req_msg,
                                         size_t req_msg_len)
{
    struct sockaddr_mctp addr = {0};
    addr.smctp_family = AF_MCTP;
    addr.smctp_network = network_id;
    addr.smctp_addr.s_addr = eid;
    addr.smctp_type = MCTP_MSG_TYPE_PLDM;
    addr.smctp_tag = MCTP_TAG_OWNER;

    int rc = sendto(mctp_fd, (uint8_t*)pldm_req_msg, req_msg_len, 0,
                    (struct sockaddr*)&addr, sizeof(addr));

    if (rc == -1)
    {
        perror("FAILED ON SEND LIBPLDM");
        return PLDM_REQUESTER_SEND_FAIL;
    }
    return PLDM_REQUESTER_SUCCESS;
}

static pldm_requester_rc_t mctp_recv_at_network(mctp_eid_t eid, int mctp_fd,
                                                uint8_t** pldm_resp_msg,
                                                size_t* resp_msg_len,
                                                int network_id)
{
    ssize_t min_len = sizeof(struct pldm_msg_hdr);
    struct sockaddr_mctp addr = {0};
    addr.smctp_family = AF_MCTP;
    addr.smctp_addr.s_addr = eid;
    addr.smctp_type = MCTP_MSG_TYPE_PLDM;
    addr.smctp_tag = MCTP_TAG_OWNER;
    addr.smctp_network = network_id;
    socklen_t addrlen = sizeof(addr);
    ssize_t length = recvfrom(mctp_fd, NULL, 0, MSG_PEEK | MSG_TRUNC,
                              (struct sockaddr*)&addr, &addrlen);
    if (length <= 0)
    {
		fprintf(stderr, "No length received at MCTP socket, errno: %d\n", errno);
        return PLDM_REQUESTER_RECV_FAIL;
    }
    else if (length < min_len)
    {
        /* read and discard */
        uint8_t buf[length];
        recv(mctp_fd, buf, length, 0);
        return PLDM_REQUESTER_INVALID_RECV_LEN;
    }
    else
    {
        ssize_t bytes = recvfrom(mctp_fd, *pldm_resp_msg, length, MSG_TRUNC,
                                 (struct sockaddr*)&addr, &addrlen);
        if (length != bytes)
        {
            fprintf(stderr, "Receive from socket failed, errno: %d\n", errno);
            return PLDM_REQUESTER_INVALID_RECV_LEN;
        }
        *resp_msg_len = length;
        return PLDM_REQUESTER_SUCCESS;
    }
}

pldm_requester_rc_t recv_at_network(mctp_eid_t eid, int mctp_fd,
                                         uint8_t** pldm_resp_msg,
                                         size_t* resp_msg_len, int network_id)
{
    pldm_requester_rc_t rc = mctp_recv_at_network(eid, mctp_fd, pldm_resp_msg,
                                                  resp_msg_len, network_id);
    if (rc != PLDM_REQUESTER_SUCCESS)
    {
        fprintf(stderr,
            "MCTP Data Failure: No data received on eid:%u, nid:%d\n",
            eid, network_id);
        return rc;
    }

    struct pldm_msg_hdr* hdr = (struct pldm_msg_hdr*)(*pldm_resp_msg);
    if (hdr->request != PLDM_RESPONSE)
    {
        fprintf(stderr, "MCTP Receive failed - Header is not a response\n");
        return PLDM_REQUESTER_NOT_RESP_MSG;
    }

    uint8_t pldm_rc = 0;
    if (*resp_msg_len < (sizeof(struct pldm_msg_hdr) + sizeof(pldm_rc)))
    {
        fprintf(stderr, "MCTP Receive failed - Receive message is too small\n");
        return PLDM_REQUESTER_RESP_MSG_TOO_SMALL;
    }

    return PLDM_REQUESTER_SUCCESS;
}

LIBPLDM_ABI_STABLE
pldm_requester_rc_t pldm_recv_at_network(mctp_eid_t eid, int mctp_fd,
                                         uint8_t instance_id,
                                         uint8_t** pldm_resp_msg,
                                         size_t* resp_msg_len, int network_id)
{
    pldm_requester_rc_t rc = recv_at_network(eid, mctp_fd, pldm_resp_msg,
                                                  resp_msg_len, network_id);

    if (rc != PLDM_REQUESTER_SUCCESS)
    {
        return rc;
    }

    struct pldm_msg_hdr* hdr = (struct pldm_msg_hdr*)(*pldm_resp_msg);
    if (hdr->instance_id != instance_id)
    {
        fprintf(stderr, "MCTP Receive failed - Instance Id mismatch\n");
        return PLDM_REQUESTER_INSTANCE_ID_MISMATCH;
    }

    return PLDM_REQUESTER_SUCCESS;
}
