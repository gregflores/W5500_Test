/* dhcplib.c
 * WizNet W5200 Ethernet Controller Driver for MSP430
 * High-level Support I/O Library
 * Dynamic Host Configuration Protocol
 *
 * Ported to RobG's W5500 driver
 *
 * Copyright (c) 2014, Eric Brundick <spirilis@linux.com>
 *
 * Permission to use, copy, modify, and/or distribute this software for any purpose
 * with or without fee is hereby granted, provided that the above copyright notice
 * and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT,
 * OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS
 * ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <msp.h>
#include "dhcplib.h"
#include <stdlib.h>
#include <string.h>
#include "w5500.h"
#include "wizdebug.h"
#include <stdio.h>

/* DHCPlib-specific errno & descriptions */
int dhcplib_errno;

const char *dhcplib_errno_descriptions[] = {
	"DHCPlib wiznet_socket() Fault",
	"DHCPlib wiznet_bind() UDP Fault",
	"DHCPDISCOVER Send Fault",
	"DHCPOFFER recvfrom Fault",
	"DHCPOFFER UDP Srcport Invalid",
	"DHCPOFFER DHCP Header Read Fault",
	"DHCPOFFER DHCP Header Incorrect XID",
	"DHCPOFFER Preamble OP Field Invalid",
	"DHCPOFFER Read First Option Fault",
	"DHCPREQUEST Send Fault",
	"DHCPACK recvfrom Fault",
	"DHCPACK UDP Srcport Invalid",
	"DHCPACK DHCP Header Read Fault",
	"DHCPACK DHCP Header Incorrect XID",
	"DHCPACK Preamble OP Field Invalid",
	"DHCPACK Read First Option Fault",
	"DHCPACK Addresses Do Not Match DHCPOFFER",
	"DHCPlib WizNet Link Down",
	"DHCPlib Timeout Waiting for Reply"
};

/* Useful default IPs */
const uint8_t _ipzero[4] = { 0x00, 0x00, 0x00, 0x00 };
#define ipzero ((uint8_t *)_ipzero)
const uint8_t _ipone[4] = { 0xFF, 0xFF, 0xFF, 0xFF };
#define ipone ((uint8_t *)_ipone)

#define SOFF_GIADDR 8+0
#define SOFF_SUBNET 8+4
#define SOFF_YIADDR 8+8
#define SOFF_SIADDR 8+12
#define SOFF_YIADDR_ACK 8+16
#define SOFF_SIADDR_ACK 8+20
#define SOFF_XID 8+20
#define SOFF_OPTCODE 32
#define SOFF_OPTLEN 33

/* Functions */

char *dhcp_strerror(int errno)
{
	if (errno < 1 || errno > DHCP_ERRNO_MAX)
		return (char *)" ";

	return (char *)dhcplib_errno_descriptions[errno-1];
}

// Helper; rewind "piecemeal" buffer cache and flush a custom # of bytes to advance past current packet.
void dhcp_helper_flush_packet(uint8_t sockfd, uint16_t udp_pkt_len)
{
	refreshRXBufferCache(sockfd);
	flushRXBufferPiecemeal(sockfd, 8+udp_pkt_len);  // 8 bytes account for the initial UDP preamble (IP+srcport+pktlen) information.
	setSn_IR(sockfd, Sn_IR_RECV);
	setSn_CR(sockfd, Sn_CR_RECV);
	while (getSn_CR(sockfd))
		;
	refreshRXBufferCache(sockfd);
}

// Write initial DHCP information
int dhcp_write_header(uint8_t sockfd, uint8_t *scratch, uint8_t *ciaddr, uint8_t *yiaddr, uint8_t *siaddr, uint8_t *giaddr, uint8_t *chaddr)
{
	scratch[0] = 0x01;  // DHCP client->server request
	scratch[1] = 0x01;  // HTYPE
	scratch[2] = 0x06;  // HLEN
	scratch[3] = 0x00;  // HOPS
	writeToTXBufferPiecemeal(sockfd, scratch, 4);
	scratch[0] = DHCP_XID_0;
	scratch[1] = DHCP_XID_1;
	scratch[2] = DHCP_XID_2;
	scratch[3] = DHCP_XID_3;
	writeToTXBufferPiecemeal(sockfd, scratch, 4);
	fillTXBufferPiecemeal(sockfd, 0x00, 4);  // SECS, FLAGS (0x0000 for each)

	writeToTXBufferPiecemeal(sockfd, ciaddr, 4);
	writeToTXBufferPiecemeal(sockfd, yiaddr, 4);
	writeToTXBufferPiecemeal(sockfd, siaddr, 4);
	writeToTXBufferPiecemeal(sockfd, giaddr, 4);
	writeToTXBufferPiecemeal(sockfd, chaddr, 6);

	fillTXBufferPiecemeal(sockfd, 0x00, 192+10);

	// Magic Cookie
	scratch[0] = DHCP_MAGIC_COOKIE_0;
	scratch[1] = DHCP_MAGIC_COOKIE_1;
	scratch[2] = DHCP_MAGIC_COOKIE_2;
	scratch[3] = DHCP_MAGIC_COOKIE_3;
	writeToTXBufferPiecemeal(sockfd, scratch, 4);

	return 0;
}

// Read initial DHCP information
int dhcp_read_header(uint8_t sockfd, uint8_t *scratch, uint8_t *ciaddr, uint8_t *yiaddr, uint8_t *siaddr, uint8_t *giaddr, uint8_t *chaddr)
{
	uint8_t *xid = scratch+SOFF_XID;

	#if WIZNET_DEBUG > 2
	const char *funcname = "dhcp_read_header()";
	#endif

	if (getVirtualRXReceived(sockfd) < 12) {
		printf("%s: Insufficient bytes (%d) to read a header\n", funcname, getVirtualRXReceived(sockfd));
		return -1;
	}
	readFromRXBufferPiecemeal(sockfd, scratch, 4);
	printf("%s: Initial header: OP=%d, HTYPE=%d, HLEN=%d, HOPS=%d\n", funcname, scratch[0], scratch[1], scratch[2], scratch[3]);
	if (scratch[0] != 0x02) {  // Preamble DHCP Server->Client
		printf("%s: Failed HTYPE\n", funcname);
		return -1;
	}
	readFromRXBufferPiecemeal(sockfd, xid, 4);  // Store XID for caller
	printf("%s: Read XID: %x%x%x%x\n", funcname, xid[0], xid[1], xid[2], xid[3]);
	readFromRXBufferPiecemeal(sockfd, scratch, 4);
	printf("%s: SECS=%x, FLAGS=%x\n", funcname, ntohs(scratch), ntohs(scratch+2));

	readFromRXBufferPiecemeal(sockfd, ciaddr, 4);
	readFromRXBufferPiecemeal(sockfd, yiaddr, 4);
	readFromRXBufferPiecemeal(sockfd, siaddr, 4);
	readFromRXBufferPiecemeal(sockfd, giaddr, 4);
	readFromRXBufferPiecemeal(sockfd, chaddr, 6);
	flushRXBufferPiecemeal(sockfd, 192+10);
	readFromRXBufferPiecemeal(sockfd, scratch, 4);
	if (scratch[0] != DHCP_MAGIC_COOKIE_0 || scratch[1] != DHCP_MAGIC_COOKIE_1 ||
	    scratch[2] != DHCP_MAGIC_COOKIE_2 || scratch[3] != DHCP_MAGIC_COOKIE_3) {
		printf("%s: Read header with invalid magic cookie (%x%x%x%x)\n", funcname, scratch[0], scratch[1], scratch[2], scratch[3]);
		return -1;
	}

	return 0;
}

int dhcp_write_option(uint8_t sockfd, uint8_t option, uint8_t len, void *buf)
{
	uint8_t opthdr[2];

	opthdr[0] = option;
	opthdr[1] = len;
	if (getTXVirtualFreeSize(sockfd) < 2+len)
		return -1;

	writeToTXBufferPiecemeal(sockfd, opthdr, 2);
	writeToTXBufferPiecemeal(sockfd, buf, len);
	
	return 0;
}

int dhcp_read_option(uint8_t sockfd, uint8_t *option, uint8_t *len, uint8_t maxlen, void *buf)
{
	uint8_t opthdr[2];

	#if WIZNET_DEBUG > 2
	const char *funcname = "dhcp_read_option()";
	#endif

	if (getVirtualRXReceived(sockfd) < 2) {
		printf("%s: Insufficient bytes (%d) to read a DHCP option field\n", funcname, getVirtualRXReceived(sockfd));
		return -1;
	}

	readFromRXBufferPiecemeal(sockfd, opthdr, 2);
	*option = opthdr[0];

	if (opthdr[1] > maxlen)
		*len = maxlen;
	else
		*len = opthdr[1];
	printf("%s: optcode=%u, optlen=%u, maxlen=%u\n", funcname, opthdr[0], opthdr[1], maxlen);

	readFromRXBufferPiecemeal(sockfd, (uint8_t *)buf, *len);  // Potential buffer overflow here!
	if (*len < opthdr[1])
		flushRXBufferPiecemeal(sockfd, opthdr[1] - *len);

	return 0;
}

int dhcp_send_packet(uint8_t sockfd, uint8_t *scratch, uint8_t dhcp_msgtype)
{
	uint8_t *ourmac = scratch+SOFF_SIADDR_ACK;  // Overloading SIADDR_ACK + optcode + optlen to hold the MAC address
	uint8_t *yiaddr = scratch+SOFF_YIADDR;
	uint8_t *siaddr = scratch+SOFF_SIADDR;

	#if WIZNET_DEBUG > 2
	const char *funcname = "dhcp_send_packet()";
	#endif

	getSHAR(ourmac);

	printf("%s: Our MAC = %x:%x:%x:%x:%x:%x\n", funcname, ourmac[0], ourmac[1], ourmac[2], ourmac[3], ourmac[4], ourmac[5]);


	// DHCP header
	if (dhcp_msgtype == DHCP_MSGTYPE_DHCPDISCOVER) {
		dhcp_write_header(sockfd, scratch, ipzero, ipzero, ipzero, ipzero, ourmac);
	} else {
		dhcp_write_header(sockfd, scratch, ipzero, ipzero, siaddr, ipzero, ourmac);
	}

	/* Write options */
	
	switch (dhcp_msgtype) {
		case DHCP_MSGTYPE_DHCPDISCOVER:
			// DHCP option 53: DHCPDISCOVER
			scratch[0] = DHCP_MSGTYPE_DHCPDISCOVER;
			dhcp_write_option(sockfd, DHCP_OPTCODE_DHCP_MSGTYPE, 1, scratch);

			// Option 55, parameter request list.
			// Subnetmask, Router, Domain Name Server
			scratch[0] = DHCP_OPTCODE_SUBNET_MASK;
			scratch[1] = DHCP_OPTCODE_ROUTER;
			scratch[2] = DHCP_OPTCODE_DOMAIN_SERVER;
			dhcp_write_option(sockfd, DHCP_OPTCODE_PARAM_LIST, 3, scratch);
			break;

		case DHCP_MSGTYPE_DHCPREQUEST:
			// DHCP Option 53: DHCP REQUEST
			scratch[0] = DHCP_MSGTYPE_DHCPREQUEST;
			dhcp_write_option(sockfd, DHCP_OPTCODE_DHCP_MSGTYPE, 1, scratch);

			// DHCP Option 50: Request IP address
			dhcp_write_option(sockfd, DHCP_OPTCODE_IP_ADDRESS_REQUEST, 4, yiaddr);

			// DHCP Option 54: DHCP server IP
			dhcp_write_option(sockfd, DHCP_OPTCODE_DHCP_SERVER_IP, 4, siaddr);
			break;
	}

	// Finished
	dhcp_write_option(sockfd, DHCP_OPTCODE_END, 0, NULL);

	send(sockfd, NULL, (u_int *)scratch, 1);  // Submit SEND command to commit the packet over the wire

	return 0;
}

// This returns the value of the first DHCP option, assuming it is option 53 (Message Type), or -1 on error.
int dhcp_validate_packet(uint8_t sockfd, uint8_t *scratch, uint16_t udp_pkt_len, uint8_t *yiaddr, uint8_t *siaddr)
{
	int ret;
	uint8_t *optcode = scratch+SOFF_OPTCODE, *optlen = scratch+SOFF_OPTLEN;
	uint8_t *xid = scratch+SOFF_XID;

	#if WIZNET_DEBUG > 1
	const char *funcname = "dhcp_validate_packet()";
	#endif

	// Read DHCP header
	if ( (ret = dhcp_read_header(DHCP_SOCKFD, scratch, scratch, yiaddr, siaddr, scratch, scratch)) < 0 ) {
		dhcplib_errno = DHCP_ERRNO_DHCPOFFER_READHEADER_FAULT;
		printf("%s: %s (%d)\n", funcname, dhcplib_errno_descriptions[dhcplib_errno-1], ret);
		return -1;
	}

	// Right transaction ID?
	if (xid[0] != DHCP_XID_0 || xid[1] != DHCP_XID_1 || xid[2] != DHCP_XID_2 || xid[3] != DHCP_XID_3) {
		dhcplib_errno = DHCP_ERRNO_DHCPOFFER_INVALID_XID;
		printf("%s: %s\n", funcname, dhcplib_errno_descriptions[dhcplib_errno-1]);
		return -1;
	}

	// Read first option; should be option 53
	if ( (ret = dhcp_read_option(DHCP_SOCKFD, optcode, optlen, 8, scratch)) < 0 ) {
		dhcplib_errno = DHCP_ERRNO_DHCPOFFER_READOPT_FAULT;
		printf("%s: %s (%d)\n", funcname, dhcplib_errno_descriptions[dhcplib_errno-1], ret);
		return -1;
	}

	if (*optcode != DHCP_OPTCODE_DHCP_MSGTYPE || *optlen != 1) {
		// No dhcplib_errno code for this one, it'll just have the caller flush this packet and move on to the next one.
		return -1;
	}

	return (int) scratch[0];
}

/* Main event loop: Initialize UDP socket, perform DHCP communication, configure Wiznet IP information to match.
 * Returns 0 if success, -1 if error.
 *
 * Optionally stash DNS server in dnsaddr if dnsaddr pointer is not NULL (0x0000).
 * Configure timeout in dhcplib.h via #define DHCP_LOOP_COUNT_TIMEOUT
 */
int dhcp_loop_configure(uint8_t *dnsaddr, struct DHCPrenew *lease)
{
	uint8_t scratch[34];
	uint16_t loopcount=0, udp_pkt_len=0;
	/* ^ loopcount is an overloaded variable:
	 * +-----------------------------------------------+
	 * |15 14 13 12 11 10  9  8  7  6  5  4  3  2  1  0|
	 * +--+-----------+--------------------------------+
	 * | R|  STATE    |   LOOP COUNT                   |
	 * +--+-----------+--------------------------------+
	 * R = "Ready to Return" - a flag bit used to break out of the main while() loop, indicating that
	 *                         we're ready to exit for good or for bad.
	 *
	 * State = status of the state machine, i.e. which phase of the DHCP process we're in at the moment.
	 *
	 * Loop count = # of whirls around the while() loop we've gone without seeing any packet action.
	 *              Once this loop count has counted to DHCP_LOOP_COUNT_TIMEOUT, we abort the DHCP
	 *              process, close the socket and report back an error.
	 */
	int ret;
	uint8_t *giaddr = scratch+SOFF_GIADDR, *subnetmask = scratch+SOFF_SUBNET, *yiaddr = scratch+SOFF_YIADDR;
	uint8_t *siaddr = scratch+SOFF_SIADDR, *yiaddr_ack = scratch+SOFF_YIADDR_ACK, *siaddr_ack = scratch+SOFF_SIADDR_ACK;
	uint8_t *optcode = scratch+SOFF_OPTCODE, *optlen = scratch+SOFF_OPTLEN;

	#if WIZNET_DEBUG > 0
	const char *funcname = "dhcp_loop_configure()";
	uint8_t did_report_dhcpoffer=0, did_report_dhcpack=0;
	#endif

	// Variable prep
	memset(subnetmask, 0xFF, 4);
	dhcplib_errno = 0;

	// Clear IP information from WizNet - if we are not renewing
	if (lease == NULL || !lease->do_renew) {
		setSIPR(ipzero);  // 0.0.0.0
		setSUBR(ipone);   // 255.255.255.255
		setGAR(ipzero);   // 0.0.0.0
	}

	// Bail if we have no link.
	if ( (getPHYCFGR() & PHYCFGR_LNK_ON) == 0x00 ) {
		dhcplib_errno = DHCP_ERRNO_LINK_DOWN;
		return -1;
	}

	// Init socket - bind UDP on port 68
	socket(DHCP_SOCKFD, Sn_MR_UDP, 68, 0);

	// UDP destination for outbound packets
	setSn_DPORT(DHCP_SOCKFD, 67);    // Server listens on port 67, client listens on port 68
	setSn_DIPR(DHCP_SOCKFD, ipone);  // Dest IP = 255.255.255.255 (UDP broadcast)

	if (lease != NULL && lease->do_renew) {
		// DHCP renewal only requires a DHCPREQUEST + DHCPACK, so pre-set the 'state'
		loopcount = 2 << 11;
		memcpy(siaddr, lease->dhcpserver, 4);
		getSIPR(yiaddr);
		getGAR(giaddr);
		getSUBR(subnetmask);
		/* In theory, you should be able to request a DHCP renewal with a Unicast packet.
		 * In practice, though, I've found with an environment using "DHCP helper" features (e.g. in Cisco
		 * hardware) that this will not work; you still need to send your DHCPREQUEST packet to the broadcast
		 * address (255.255.255.255).
		 */
	}

	// Main event loop with timeout support
	while ( (loopcount & 0x07FF) < DHCP_LOOP_COUNT_TIMEOUT && !(loopcount & 0x8000) ) {  // loopcount bit 15 set means we're done
		switch ( (loopcount & 0x7800) >> 11 ) {
			case 0:
				// Send DHCPDISCOVER
				printf("%s: Sending DHCPDISCOVER\n", funcname);
				dhcp_send_packet(DHCP_SOCKFD, scratch, DHCP_MSGTYPE_DHCPDISCOVER);
				loopcount = (loopcount & 0x87FF) | (1 << 11);
				break;

			case 1:
				// Check for DHCPOFFER
				#if WIZNET_DEBUG > 0
				if (!did_report_dhcpoffer) {
					printf("%s: Waiting for DHCPOFFER\n", funcname);
					did_report_dhcpoffer = 1;
				}
				#endif

				if (getRXReceived(DHCP_SOCKFD)) {
					printf("%s: Packet received\n", funcname);

					// Read UDP preamble
					flushRXBufferPiecemeal(DHCP_SOCKFD, 4);  // Src IP; don't care
					readFromRXBufferPiecemeal(DHCP_SOCKFD, scratch, 4);  // Src Port & UDP Packet Length
					udp_pkt_len = ntohs(scratch+2);
					printf("%s: SrcPort = %d, PktLen = %d, RXrecv = %d\n", funcname, ntohs(scratch), udp_pkt_len, getRXReceived(DHCP_SOCKFD));

					if (ntohs(scratch) != 67) {
						dhcplib_errno = DHCP_ERRNO_DHCPOFFER_SRCPORT_INVALID;
						dhcp_helper_flush_packet(DHCP_SOCKFD, udp_pkt_len);
						printf("%s: %s (found %u)\n", funcname, dhcplib_errno_descriptions[dhcplib_errno-1], ntohs(scratch));
						continue;
					}

					ret = dhcp_validate_packet(DHCP_SOCKFD, scratch, udp_pkt_len, yiaddr, siaddr);
					if (ret == DHCP_MSGTYPE_DHCPOFFER) {
						printf("%s: Packet is DHCPOFFER\n", funcname);
						printf("%s: DHCP server gave us IP address %d.%d.%d.%d\n", funcname, yiaddr[0], yiaddr[1], yiaddr[2], yiaddr[3]);

						// Read all options to extract info
						do {
							dhcp_read_option(DHCP_SOCKFD, optcode, optlen, 8, scratch);
							switch (*optcode) {
								case DHCP_OPTCODE_SUBNET_MASK:
									if (*optlen == 4) {
										memcpy(subnetmask, scratch, 4);
										printf("%s: subnetmask: %u.%u.%u.%u\n", funcname, subnetmask[0], subnetmask[1], subnetmask[2], subnetmask[3]);
									} else {
										printf("%s: Option %u subnetmask found but optlen invalid; optlen = %d\n", funcname, *optcode, *optlen);
									}
									break;
								case DHCP_OPTCODE_ROUTER:
									if (*optlen == 4) {
										memcpy(giaddr, scratch, 4);
										printf("%s: gateway: %u.%u.%u.%u\n", funcname, giaddr[0], giaddr[1], giaddr[2], giaddr[3]);
									} else {
										printf("%s: Option %u router found but optlen invalid; optlen = %d\n", funcname, *optcode, *optlen);
									}
									break;
								case DHCP_OPTCODE_DHCP_SERVER_IP:
									if (*optlen == 4) {
										memcpy(siaddr, scratch, 4);
										printf("%s: DHCP server: %u.%u.%u.%u\n", funcname, siaddr[0], siaddr[1], siaddr[2], siaddr[3]);
									} else {
										printf("%s: Option %u DHCP Server IP found but optlen invalid; optlen = %d\n", funcname, *optcode, *optlen);
									}
									break;
								case DHCP_OPTCODE_DOMAIN_SERVER:
									if (*optlen >= 4) {  // Only extract the first DNS server.
										printf("%s: DNS server: %u.%u.%u.%u (%u in total)\n", funcname, scratch[0], scratch[1], scratch[2], scratch[3], *optlen/4);
										if (dnsaddr != NULL)
											memcpy(dnsaddr, scratch, 4);
									} else {
										printf("%s: Option %u DNS server found but optlen invalid; optlen = %d\n", funcname, *optcode, *optlen);
									}
									break;
								case DHCP_OPTCODE_RENEWAL_TIME:
									if (*optlen == 4) {
										if (lease != NULL) {
											if (ntohs(scratch) > 0)
												lease->seconds = 65535;
											else
												lease->seconds = ntohs(scratch+2);
											memcpy(lease->dhcpserver, siaddr, 4);
											printf("%s: Renewal time (T1) = %d seconds, reporting %d to application\n", funcname, ((uint32_t)ntohs(scratch) << 16) | (uint32_t)ntohs(scratch+2), lease->seconds);
										} else {
											printf("%s: Renewal time (T1) = %d seconds, application not requesting lease information.\n", funcname, ((uint32_t)ntohs(scratch) << 16) | (uint32_t)ntohs(scratch+2));
										}
									} else {
										printf("%s: Option %u Renewal time (T1) found but optlen invalid; optlen = %d\n", funcname, *optcode, *optlen);
									}

							}
						} while (*optcode != DHCP_OPTCODE_END);
						dhcp_helper_flush_packet(DHCP_SOCKFD, udp_pkt_len);  // Flush remainder of packet
						loopcount = (loopcount & 0x87FF) | (2 << 11);
						dhcplib_errno = 0;  // Reset dhcplib_errno in case it was set by a prior erroneous packet
					} else {
						// Not a DHCPOFFER; flush packet and signal more may come, then loop around.
						dhcp_helper_flush_packet(DHCP_SOCKFD, udp_pkt_len);  // Flush remainder of packet
						printf("%s: Packet is not DHCPOFFER; MSGTYPE=%d\n", funcname, ret);
					}
				}
				break;  // If there is no data waiting, loop will just continue another round.

			case 2:  // Send DHCPREQUEST (officially requesting the lease)
				printf("%s: Sending DHCPREQUEST to %u.%u.%u.%u requesting IP=%u.%u.%u.%u\n", funcname,
					siaddr[0], siaddr[1], siaddr[2], siaddr[3],
					yiaddr[0], yiaddr[1], yiaddr[2], yiaddr[3]);
				dhcp_send_packet(DHCP_SOCKFD, scratch, DHCP_MSGTYPE_DHCPREQUEST);
				loopcount = (loopcount & 0x87FF) | (3 << 11);
				break;

			case 3:  // Receive DHCPACK (officially acknowledgement that the lease is ours)
				#if WIZNET_DEBUG > 0
				if (!did_report_dhcpack) {
					printf("%s: Waiting for DHCPACK\n", funcname);
					did_report_dhcpack = 1;
				}
				#endif

				if (getRXReceived(DHCP_SOCKFD)) {
					printf("%s: Packet received\n", funcname);

					// Read UDP preamble
					flushRXBufferPiecemeal(DHCP_SOCKFD, 4);  // Src IP; don't care
					readFromRXBufferPiecemeal(DHCP_SOCKFD, scratch, 4);  // Src Port & UDP Packet Length
					udp_pkt_len = ntohs(scratch+2);
					printf("%s: SrcPort = %d, PktLen = %d, RXrecv = %d\n", funcname, ntohs(scratch), udp_pkt_len, getRXReceived(DHCP_SOCKFD));

					if (ntohs(scratch) != 67) {
						dhcplib_errno = DHCP_ERRNO_DHCPACK_SRCPORT_INVALID;
						dhcp_helper_flush_packet(DHCP_SOCKFD, udp_pkt_len);  // Flush remainder of packet
						printf("%s: %s (found %u)\n", funcname, dhcplib_errno_descriptions[dhcplib_errno-1], ntohs(scratch));
						continue;
					}

					ret = dhcp_validate_packet(DHCP_SOCKFD, scratch, udp_pkt_len, yiaddr_ack, scratch);
					if (ret == DHCP_MSGTYPE_DHCPACK) {
						printf("%s: Packet is DHCPACK\n", funcname);

						// Read all options to extract info
						do {
							dhcp_read_option(DHCP_SOCKFD, optcode, optlen, 8, scratch);

							switch (*optcode) {
								case DHCP_OPTCODE_DHCP_SERVER_IP:
									if (*optlen >= 4) {
										/* Using debug3 here because we're only reading the DHCP_SERVER_IP option
										 * from DHCPACK merely to validate that we received an ACK from the correct server.
										 */
										printf("%s: DHCP server: %u.%u.%u.%u\n", funcname, scratch[0], scratch[1], scratch[2], scratch[3]);
										memcpy(siaddr_ack, scratch, 4);
									} else {
										printf("%s: Option %u DHCP Server IP found but optlen invalid; optlen = %d\n", funcname, *optcode, *optlen);
									}
							}
						} while (*optcode != DHCP_OPTCODE_END);
						dhcp_helper_flush_packet(DHCP_SOCKFD, udp_pkt_len);  // Flush remainder of packet

						printf("%s: DHCPACK contents processed; validating\n", funcname);
						if (memcmp(siaddr, siaddr_ack, 4)) {
							printf("%s: DHCP server IP does not match!\n", funcname);
							dhcplib_errno = DHCP_ERRNO_DHCPACK_ADDRESSES_DO_NOT_MATCH;
							continue;
						}

						if (memcmp(yiaddr, yiaddr_ack, 4)) {
							printf("%s: Assigned IP does not match!\n", funcname);
							dhcplib_errno = DHCP_ERRNO_DHCPACK_ADDRESSES_DO_NOT_MATCH;
							continue;
						}

						// Set IP, gateway, router!
						if (dhcplib_errno != DHCP_ERRNO_DHCPACK_ADDRESSES_DO_NOT_MATCH && (lease == NULL || lease->do_renew == 0)) {
							printf("%s: Configuring WizNet IP settings\n", funcname);

							setGAR(giaddr);
							printf("%s: setGAR(%d.%d.%d.%d)\n", funcname, giaddr[0], giaddr[1], giaddr[2], giaddr[3]);
							setSUBR(subnetmask);
							printf("%s: setSUBR(%d.%d.%d.%d)\n", funcname, subnetmask[0], subnetmask[1], subnetmask[2], subnetmask[3]);
							setSIPR(yiaddr);
							printf("%s: setSIPR(%d.%d.%d.%d)\n", funcname, yiaddr[0], yiaddr[1], yiaddr[2], yiaddr[3]);
						}
						loopcount = (loopcount & 0x07FF) | 0x8000 | (4 << 11);  // All done!
						dhcplib_errno = 0;  // Reset dhcplib_errno in case it was set by a prior erroneous packet
						break;
					} else {
						// Not a DHCPACK; flush packet and signal more may come, then loop around.
						dhcp_helper_flush_packet(DHCP_SOCKFD, udp_pkt_len);  // Flush remainder of packet
						printf("%s: Packet is not DHCPACK; MSGTYPE=%u\n", funcname, scratch[0]);
					}
				}
				break;  // If there is no data waiting, loop will just continue another round.
		}
		_delay_cycles(500000);  // 1/100sec at 25MHz (1/64sec at 16MHz)
		loopcount++;
	}

	if ( (loopcount & 0x07FF) >= DHCP_LOOP_COUNT_TIMEOUT ) {
		printf("%s: RXrecv = %d, virtualRXrecv = %d\n", funcname, getRXReceived(DHCP_SOCKFD), getVirtualRXReceived(DHCP_SOCKFD));
		printf("%s: RX_RD = %d, RX_WR = %d, TX_RD = %d, TX_WR = %d\n", funcname, getSn_RX_RD(DHCP_SOCKFD), getSn_RX_WR(DHCP_SOCKFD), getSn_TX_RD(DHCP_SOCKFD), getSn_TX_WR(DHCP_SOCKFD));
		printf("%s: Loop count %u out of %u\n", funcname, loopcount & 0x07FF, DHCP_LOOP_COUNT_TIMEOUT);
		printf("%s: TIMEOUT\n", funcname);
		dhcplib_errno = DHCP_ERRNO_TIMEOUT;
	}
	
	close_s(DHCP_SOCKFD);

	if (dhcplib_errno != 0)
		return -1;
	return 0;
}
