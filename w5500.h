/*
 * w5500.h
 *
 *  Created on: Oct 23, 2013
 *      Author: RobG
 */

#ifndef W5500_H_
#define W5500_H_

#include "typedefs.h"

extern volatile unsigned int dbg_txwr, dbg_rxrd;

// block select
#define RWB_READ		0x00
#define RWB_WRITE	   	0x04

#define COMM_REG_BLOCK  0x00
#define REG_BLOCK		0x08
#define TXBUF_BLOCK		0x10
#define RXBUF_BLOCK		0x18

extern const u_char _socket_reg_block[];
extern const u_char _socket_txb_block[];
extern const u_char _socket_rxb_block[];

// SPI mode
#define OP_VDM			0x00
#define OP_FDM_1_BYTE	0x01
#define OP_FDM_2_BYTE	0x02
#define OP_FDM_4_BYTE	0x03

// common register block
#define MR				0x00	// Mode
#define GAR				0x01	// Gateway Address
#define SUBR			0x05	// Subnet Mask Address
#define SHAR			0x09	// Source  Hardware Address
#define SIPR			0x0F	// Source IP Address
#define INTLEVEL		0x13	// Interrupt Low Level Timer
#define IR				0x15	// Interrupt
#define IMR				0x16	// Interrupt Mask
#define SIR				0x17	// Socket Interrupt
#define SIMR			0x18	// Socket Interrupt Mask
#define RTR				0x19	// Retry Time
#define RCR				0x1B	// Retry Count
#define PTIMER			0x1C	// PPP LCP Request Timer
#define PMAGIC			0x1D	// PPP LCP Magic Number
#define PHAR			0x1E	// PPP Destination MAC Address
#define PSID			0x24	// PPP Session Identification
#define PMRU			0x26	// PPP Maximum Segment Size
#define UIPR			0x28	// Unreachable IP Address
#define UPORTR			0x2C	// Unreachable Port
#define PHYCFGR			0x2E	// PHY Configuration
#define VERSIONR		0x39	// Chip Version
// socket register block
#define Sn_MR			0x00	// Socket n Mode
#define Sn_CR			0x01	// Socket n Command
#define Sn_IR			0x02	// Socket n Interrupt
#define Sn_SR			0x03	// Socket n Status
#define Sn_PORT			0x04	// Socket n Source Port
#define Sn_DHAR			0x06
#define Sn_DIPR			0x0C
#define Sn_DPORT		0x10
#define Sn_MSSR			0x12
#define Sn_TOS			0x15
#define Sn_TTL			0x16
#define Sn_RXBUF_SIZE	0x1E
#define Sn_TXBUF_SIZE	0x1F
#define Sn_TX_FSR		0x20
#define Sn_TX_RD		0x22
#define Sn_TX_WR		0x24
#define Sn_RX_RSR		0x26
#define Sn_RX_RD		0x28
#define Sn_RX_WR		0x2A
#define Sn_IMR			0x2C
#define Sn_FRAG			0x2D
#define Sn_KPALVTR		0x2F

#define MR_RST                       0x80
#define MR_WOL                       0x20
#define MR_PB                        0x10
#define MR_PPPOE                     0x08
#define MR_FARP                      0x02

#define IR_CONFLICT                  0x80
#define IR_UNREACH                   0x40
#define IR_PPPoE                     0x20
#define IR_MP                        0x10

#define PHYCFGR_RST                  0x7F
#define PHYCFGR_OPMD                 0x40
#define PHYCFGR_OPMDC_ALLA           0x38
#define PHYCFGR_OPMDC_PDOWN          0x30
#define PHYCFGR_OPMDC_NA             0x28
#define PHYCFGR_OPMDC_100FA          0x20
#define PHYCFGR_OPMDC_100F           0x18
#define PHYCFGR_OPMDC_100H           0x10
#define PHYCFGR_OPMDC_10F            0x08
#define PHYCFGR_OPMDC_10H            0x00
#define PHYCFGR_DPX_FULL             0x04
#define PHYCFGR_DPX_HALF             0x00
#define PHYCFGR_SPD_100              0x02
#define PHYCFGR_SPD_10               0x00
#define PHYCFGR_LNK_ON               0x01
#define PHYCFGR_LNK_OFF              0x00

#define IM_IR7                  	 0x80
#define IM_IR6                  	 0x40
#define IM_IR5                  	 0x20
#define IM_IR4                  	 0x10

#define Sn_MR_CLOSE                  0x00
#define Sn_MR_TCP                    0x01
#define Sn_MR_UDP                    0x02
#define Sn_MR_MACRAW                 0x04
#define Sn_MR_UCASTB                 0x10
#define Sn_MR_ND                     0x20
#define Sn_MR_BCASTB                 0x40
#define Sn_MR_MULTI                  0x80

#define Sn_MR_MFEN                   0x80
#define Sn_MR_MMB                    0x20
#define Sn_MR_MIP6B                  0x10
#define Sn_MR_MC                     0x20

#define Sn_CR_OPEN                   0x01
#define Sn_CR_LISTEN                 0x02
#define Sn_CR_CONNECT                0x04
#define Sn_CR_DISCON                 0x08
#define Sn_CR_CLOSE                  0x10
#define Sn_CR_SEND                   0x20
#define Sn_CR_SEND_MAC               0x21
#define Sn_CR_SEND_KEEP              0x22
#define Sn_CR_RECV                   0x40

#define Sn_IR_SEND_OK                0x10
#define Sn_IR_TIMEOUT                0x08
#define Sn_IR_RECV                   0x04
#define Sn_IR_DISCON                 0x02
#define Sn_IR_CON                    0x01

#define SOCK_CLOSED                  0x00
#define SOCK_STREAM                  0x01
#define SOCK_DGRAM                   0x02
#define SOCK_INIT                    0x13
#define SOCK_LISTEN                  0x14
#define SOCK_SYNSENT                 0x15
#define SOCK_SYNRECV                 0x16
#define SOCK_ESTABLISHED             0x17
#define SOCK_FIN_WAIT                0x18
#define SOCK_CLOSING                 0x1A
#define SOCK_TIME_WAIT               0x1B
#define SOCK_CLOSE_WAIT              0x1C
#define SOCK_LAST_ACK                0x1D
#define SOCK_UDP                     0x22
#define SOCK_IPRAW                   0x32
#define SOCK_MACRAW                  0x42
#define SOCK_PPPOE                   0x5F

#define IPPROTO_IP                   0x00
#define IPPROTO_ICMP                 0x01
#define IPPROTO_IGMP                 0x02
#define IPPROTO_GGP                  0x03
#define IPPROTO_TCP                  0x06
#define IPPROTO_PUP                  12
#define IPPROTO_UDP                  17
#define IPPROTO_IDP                  22
#define IPPROTO_ND                   77
#define IPPROTO_RAW                  0xFF

void waitUntilSocketClosed(u_char s);			// loop until socket is closed
void openSocketOnPort(u_char s, u_char port);// same as socket but waits until socket is opened
void startListening(u_char s);		// same as listen but waits until listening
void socket(u_char s, u_char protocol, u_int port, u_char flag);// open socket
void close_s(u_char s);											// close socket
void connect(u_char s, u_char * addr, u_int port);// establish TCP connection in client mode (Active connection)
void disconnect(u_char s);										// disconnect
void listen(u_char s);
void receive(u_char s, u_char * buffer, u_int length);
u_int send(u_char s, const u_char * buffer, u_int * length, u_char retry);

// register read & write
u_char readRegisterByte(u_char offset, u_char control);
void writeRegisterByte(u_char offset, u_char control, u_char byte);
u_int readRegisterWord(u_char offset, u_char control);
void writeRegisterWord(u_char offset, u_char control, u_int word);
void readRegisterArray(u_char offset, u_char control, u_char* array,
		u_int length);
void writeRegisterArray(u_char offset, u_char control, u_char* array,
		u_int length);
// memory read & write
u_char readMemoryByte(u_int addr, u_char control);
void writeMemoryByte(u_int addr, u_char control, u_char byte);
void readMemoryArray(u_int addr, u_char control, u_char* array, u_int length);
void writeMemoryArray(u_int addr, u_char control, u_char* array, u_int length);
void fillMemoryArray(u_int addr, u_char control, u_char value, u_int length);
void writeToTXBuffer(u_char s, u_char* array, u_int length);
void writeToTXBufferPiecemeal(u_char s, u_char* array, u_int length);
void fillTXBufferPiecemeal(u_char s, u_char value, u_int length);
void readFromRXBuffer(u_char s, u_char* array, u_int length);
void readFromRXBufferPiecemeal(u_char s, u_char* array, u_int length);
void flushRXBufferPiecemeal(u_char s, u_int length);
void clearBuffer(u_char* array, u_int length);
//
u_int getTXFreeSize(u_char s);
u_int getTXVirtualFreeSize(u_char s);
u_int getRXReceived(u_char s);
u_int getVirtualRXReceived(u_char s);

u_int ntohs(u_char *array);
void htons(u_int val, u_char *array);

void refreshTXBufferCache(u_char s);
void refreshRXBufferCache(u_char s);

// common register functions
#define setMR(mr)				writeRegisterByte(MR, 0, mr)
#define getMR()					readRegisterByte(MR, 0)
#define setGAR(gar)				writeRegisterArray(GAR, 0, gar, 4)
#define getGAR(gar)				readRegisterArray(GAR, 0, gar, 4)
#define setSUBR(subr)			writeRegisterArray(SUBR, 0, subr, 4)
#define getSUBR(subr)			readRegisterArray(SUBR, 0, subr, 4)
#define setSHAR(shar)			writeRegisterArray(SHAR, 0, shar, 6)
#define getSHAR(shar)			readRegisterArray(SHAR, 0, shar, 6)
#define setSIPR(sipr)			writeRegisterArray(SIPR, 0, sipr, 4)
#define getSIPR(sipr)			readRegisterArray(SIPR, 0, sipr, 4)
#define setINTLEVEL(intlevel)	writeRegisterWord(INTLEVEL, 0, intlevel)
#define getINTLEVEL()			readRegisterWord(INTLEVEL, 0)
#define setIR(ir)				writeRegisterByte(IR, 0, (ir & 0xF0)) // mask reserved bits
#define getIR()					readRegisterByte(IR, 0) // lower bits are reserved
#define setIMR(imr)				writeRegisterByte(IMR, 0, (imr & 0xF0))  // mask reserved bits
#define getIMR()				readRegisterByte(IMR, 0)
#define setSIR(sir)				writeRegisterByte(SIR, 0, sir)
#define getSIR()				readRegisterByte(SIR, 0)
#define setSIMR(simr)			writeRegisterByte(SIMR, 0, simr)
#define getSIMR()				readRegisterByte(SIMR, 0)
#define setRTR(rtr)				writeRegisterWord(RTR, 0, rtr)
#define getRTR()				readRegisterWord(RTR, 0)
#define setRCR(rcr)				writeRegisterByte(RCR, 0, rcr)
#define getRCR()				readRegisterByte(RCR, 0)
#define getUIPR(uipr)			readRegisterArray(UIPR, 0, uipr, 6)
#define getUPORTR()				readRegisterWord(UPORTR, 0)
#define setPHYCFGR(phycfgr)		writeRegisterByte(PHYCFGR, 0, phycfgr)
#define getPHYCFGR()			readRegisterByte(PHYCFGR, 0)
#define getVERSIONR()			readRegisterByte(VERSIONR, 0)

// socket register functions
#define setSn_MR(s, mr)			writeRegisterByte(Sn_MR, _socket_reg_block[s], mr)
#define getSn_MR(s)				readRegisterByte(Sn_MR, _socket_reg_block[s])
#define setSn_CR(s, cr)			writeRegisterByte(Sn_CR, _socket_reg_block[s], cr)
#define getSn_CR(s)				readRegisterByte(Sn_CR, _socket_reg_block[s])
#define setSn_IR(s, ir)			writeRegisterByte(Sn_IR, _socket_reg_block[s], (ir & 0x1F))  // mask reserved bits
#define getSn_IR(s)				readRegisterByte(Sn_IR, _socket_reg_block[s])
#define setSn_IMR(s, imr)		writeRegisterByte(Sn_IMR, _socket_reg_block[s], (imr & 0x1F)) // mask reserved bits
#define getSn_IMR(s)			readRegisterByte(Sn_IMR, _socket_reg_block[s]) // upper bits are reserved
//
#define getSn_SR(s)				readRegisterByte(Sn_SR, _socket_reg_block[s])
#define setSn_PORT(s, port)		writeRegisterWord(Sn_PORT, _socket_reg_block[s], port)
#define getSn_PORT(s)			readRegisterWord(Sn_PORT, _socket_reg_block[s])
#define setSn_DHAR(s, dhar)		writeRegisterArray(Sn_DHAR, _socket_reg_block[s], dhar, 6)
#define getSn_DHAR(s, dhar)		readRegisterArray(Sn_DHAR, _socket_reg_block[s], dhar, 6)
#define setSn_DIPR(s, dipr)		writeRegisterArray(Sn_DIPR, _socket_reg_block[s], dipr, 4)
#define getSn_DIPR(s, dipr)		readRegisterArray(Sn_DIPR, _socket_reg_block[s], dipr, 4)
#define setSn_DPORT(s, dport)	writeRegisterWord(Sn_DPORT, _socket_reg_block[s], dport)
#define getSn_DPORT(s)			readRegisterWord(Sn_DPORT, _socket_reg_block[s])
#define setSn_MSSR(s, mss)		writeRegisterWord(Sn_MSSR, _socket_reg_block[s], mss)
#define getSn_MSSR(s)			readRegisterWord(Sn_MSSR, _socket_reg_block[s])
#define setSn_TOS(s, tos)		writeRegisterByte(Sn_TOS, _socket_reg_block[s], tos)
#define getSn_TOS(s)			readRegisterByte(Sn_TOS, _socket_reg_block[s])
#define setSn_TTL(s, ttl)		writeRegisterByte(Sn_TTL, _socket_reg_block[s], ttl)
#define getSn_TTL(s)			readRegisterByte(Sn_TTL, _socket_reg_block[s])
#define setSn_RXBUF_SIZE(s, rxbufsize)	writeRegisterByte(Sn_RXBUF_SIZE, _socket_reg_block[s], rxbufsize)
#define getSn_RXBUF_SIZE(s)		readRegisterByte(Sn_RXBUF_SIZE, _socket_reg_block[s])
#define setSn_TXBUF_SIZE(s, txbufsize)	writeRegisterByte(Sn_TXBUF_SIZE, _socket_reg_block[s], txbufsize)
#define getSn_TXBUF_SIZE(s)		readRegisterByte(Sn_TXBUF_SIZE, _socket_reg_block[s])
#define setSn_RX_RD(s, rxrd)	writeRegisterWord(Sn_RX_RD, _socket_reg_block[s], rxrd)
#define getSn_RX_RD(s)			readRegisterWord(Sn_RX_RD, _socket_reg_block[s])
#define getSn_TX_FSR(s)			readRegisterWord(Sn_TX_FSR, _socket_reg_block[s])
#define getSn_RX_RSR(s)			readRegisterWord(Sn_RX_RSR, _socket_reg_block[s])
//
#define getSn_RX_WR(s)			readRegisterWord(Sn_RX_WR, _socket_reg_block[s])
#define setSn_FRAG(s, frag)		writeRegisterWord(Sn_FRAG, _socket_reg_block[s], frag)
#define getSn_FRAG(s)			readRegisterWord(Sn_FRAG, _socket_reg_block[s])
#define setSn_KPALVTR(s, kpalvt)	writeRegisterByte(Sn_KPALVTR, _socket_reg_block[s], kpalvt)
#define getSn_KPALVTR(s)		readRegisterByte(Sn_KPALVTR, _socket_reg_block[s])
#define getSn_RxMAX(s)			(getSn_RXBUF_SIZE(s) << 10)
#define getSn_TxMAX(s)			(getSn_TXBUF_SIZE(s) << 10)
#define getSn_TX_RD(s)			readRegisterWord(Sn_TX_RD, _socket_reg_block[s])
#define setSn_TX_WR(s, txwr)	writeRegisterWord(Sn_TX_WR, _socket_reg_block[s], txwr)
#define getSn_TX_WR(s)			readRegisterWord(Sn_TX_WR, _socket_reg_block[s])

#endif /* W5500_H_ */
