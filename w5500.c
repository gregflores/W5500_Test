#include "defines.h"
#include "w5500.h"
#include <stdlib.h>
#include "driverlib.h"
#include <stdio.h>
volatile unsigned int dbg_txwr, dbg_rxrd;

u_int localPort = 4000;

const u_char _socket_reg_block[8] = { 0x08, 0x28, 0x48, 0x68, 0x88, 0xA8, 0xC8, 0xE8 };
const u_char _socket_txb_block[8] = { 0x10, 0x30, 0x50, 0x70, 0x90, 0xB0, 0xD0, 0xF0 };
const u_char _socket_rxb_block[8] = { 0x18, 0x38, 0x58, 0x78, 0x98, 0xB8, 0xD8, 0xF8 };

extern u_char sendReceiveByteSPI(u_char byte);

u_int _tx_wr_cache[8], _rx_rd_cache[8];  /* Used for "piecemeal" writes & reads when 
					  * we update TX_WR or RX_RD but the WizNet still
					  * gives us the old value until we perform a CR
					  * command action on the socket.
					  */

/**
 * wait until socket close_sd status
 */
void waitUntilSocketClosed(u_char s) {
	while (getSn_SR(s) != SOCK_CLOSED)
		;
}

/**
 * open socket and wait for initiated status
 */
void openSocketOnPort(u_char s, u_char port) {
	socket(s, Sn_MR_TCP, port, 0x00);
	while (getSn_SR(s) != SOCK_INIT)
		;
}

/**
 * listen and wait for listening status
 */
void startListening(u_char s) {
	listen(s);
	while (getSn_SR(s) != SOCK_LISTEN)
		;
}

/**
 * establish connection for the channel in passive (server) mode.
 */
void listen(u_char s) {
	setSn_CR(s, Sn_CR_LISTEN);
	while (getSn_CR(s))
		;
}

/**
 * disconnect
 */
void disconnect(u_char s) {
	setSn_CR(s, Sn_CR_DISCON);
	while (getSn_CR(s))
		;
}

/**
 * close_s the socket
 */
void close_s(u_char s) {
	setSn_CR(s, Sn_CR_CLOSE);
	while (getSn_CR(s))
		;
	setSn_IR(s, 0xFF);
}

/**
 * initialize the channel in particular mode, set the port, and wait for W5500 to finish.
 */
void socket(u_char s, u_char protocol, u_int port, u_char flag) {
	setSn_MR(s, protocol | flag);
	if (port != 0) {
		setSn_PORT(s, port);
	} else {
		localPort++; // if not provided, set the source port number to an arbitrary number
		setSn_PORT(s, localPort);
	}
	setSn_CR(s, Sn_CR_OPEN);
	while (getSn_CR(s))
		;
	refreshTXBufferCache(s);
	refreshRXBufferCache(s);
}

/**
 * establish connection for the channel in Active (client) mode.
 */
void connect(u_char s, u_char * addr, u_int port) {

	setSn_DIPR(s, addr);
	setSn_DPORT(s, port);
	setSn_CR(s, Sn_CR_CONNECT);
	while (getSn_CR(s))
		;
	refreshTXBufferCache(s);
	refreshRXBufferCache(s);

//	while (getSn_SR(s)) != SOCK_SYNSENT) {
//		if (getSn_SR(s)) == SOCK_ESTABLISHED) {
//			break;
//		}
//		if (getSn_IR(s) & Sn_IR_TIMEOUT) {
//			setSn_IR(s, Sn_IR_TIMEOUT); // clear TIMEOUT Interrupt
//			break;
//		}
//	}
}


// register read & write
u_char readRegisterByte(u_char offset, u_char control) {
	uint8_t byte;

	wizSelect();;
	sendReceiveByteSPI(0);
	sendReceiveByteSPI(offset);
	sendReceiveByteSPI(control); // RWB_READ is 0, so we just skip it
	byte = sendReceiveByteSPI(0);
	wizDeselect();;
	//if(byte != 0x0a){
		printf("0x%x\n", byte);
	//}
	return byte;
}
void writeRegisterByte(u_char offset, u_char control, u_char byte) {
	wizSelect();;

	sendReceiveByteSPI(0);
	sendReceiveByteSPI(offset);
	sendReceiveByteSPI(control | RWB_WRITE);
	sendReceiveByteSPI(byte);

	wizDeselect();;
}
u_int readRegisterWord(u_char offset, u_char control) {
	u_int word;
	wizSelect();;

	sendReceiveByteSPI(0);
	sendReceiveByteSPI(offset);
	sendReceiveByteSPI(control); // RWB_READ is 0, so we just skip it
	word = sendReceiveByteSPI(0);
	word <<= 8;
	word |= sendReceiveByteSPI(0);

	wizDeselect();;
	return word;
}
void writeRegisterWord(u_char offset, u_char control, u_int word) {
	wizSelect();;

	sendReceiveByteSPI(0);
	sendReceiveByteSPI(offset);
	sendReceiveByteSPI(control | RWB_WRITE);
	sendReceiveByteSPI(word >> 8);
	sendReceiveByteSPI(word);

	wizDeselect();;
}
void readRegisterArray(u_char offset, u_char control, u_char* array,
		u_int length) {
	u_int c;

	wizSelect();;

	sendReceiveByteSPI(0);
	sendReceiveByteSPI(offset);
	sendReceiveByteSPI(control); // RWB_READ is 0, so we just skip it
	for (c = 0; c < length; c++) {
		*array = sendReceiveByteSPI(0);
		array++;
	}

	wizDeselect();;
}
void writeRegisterArray(u_char offset, u_char control, u_char* array,
		u_int length) {
	u_int c;

	wizSelect();;

	sendReceiveByteSPI(0);
	sendReceiveByteSPI(offset);
	sendReceiveByteSPI(control | RWB_WRITE);
	for (c = 0; c < length; c++) {
		sendReceiveByteSPI(*array++);
	}

	wizDeselect();;
}
// memory read & write
u_char readMemoryByte(u_int addr, u_char control) {
	u_char byte;
	wizSelect();;

	sendReceiveByteSPI(addr >> 8);
	sendReceiveByteSPI(addr);
	sendReceiveByteSPI(control); // RWB_READ is 0, so we just skip it
	byte = sendReceiveByteSPI(0);

	wizDeselect();;
	return byte;
}
void writeMemoryByte(u_int addr, u_char control, u_char byte) {
	wizSelect();;

	sendReceiveByteSPI(addr >> 8);
	sendReceiveByteSPI(addr);
	sendReceiveByteSPI(control | RWB_WRITE);
	sendReceiveByteSPI(byte);

	wizDeselect();;
}
void readMemoryArray(u_int addr, u_char control, u_char* array, u_int length) {
	u_int c;

	wizSelect();;

	sendReceiveByteSPI(addr >> 8);
	sendReceiveByteSPI(addr);
	sendReceiveByteSPI(control); // RWB_READ is 0, so we just skip it
	for (c = 0; c < length; c++) {
		*array = sendReceiveByteSPI(0);
		array++;
	}

	wizDeselect();;
}
void writeMemoryArray(u_int addr, u_char control, u_char* array, u_int length) {
	u_int c;

	wizSelect();;

	sendReceiveByteSPI(addr >> 8);
	sendReceiveByteSPI(addr);
	sendReceiveByteSPI(control | RWB_WRITE);
	for (c = 0; c < length; c++) {
		sendReceiveByteSPI(*array++);
	}

	wizDeselect();;
}
void fillMemoryArray(u_int addr, u_char control, u_char value, u_int length) {
	u_int c;

	wizSelect();;

	sendReceiveByteSPI(addr >> 8);
	sendReceiveByteSPI(addr);
	sendReceiveByteSPI(control | RWB_WRITE);
	for (c = 0; c < length; c++) {
		sendReceiveByteSPI(value);
	}

	wizDeselect();;
}

void writeToTXBuffer(u_char s, u_char *array, u_int length) {
	u_int addr;
	if (length == 0) {
		return;
	}
	addr = getSn_TX_WR(s);
	writeMemoryArray(addr, _socket_txb_block[s], array, length);
	addr += length;
	setSn_TX_WR(s, addr);
	_tx_wr_cache[s] = addr;
}

void writeToTXBufferPiecemeal(u_char s, u_char *array, u_int length) {
	u_int addr;
	if (length == 0) {
		return;
	}
	addr = _tx_wr_cache[s];
	writeMemoryArray(addr, _socket_txb_block[s], array, length);
	addr += length;
	setSn_TX_WR(s, addr);
	_tx_wr_cache[s] = addr;
}

void fillTXBufferPiecemeal(u_char s, u_char value, u_int length) {
	u_int addr;
	if (length == 0) {
		return;
	}
	addr = _tx_wr_cache[s];
	fillMemoryArray(addr, _socket_txb_block[s], value, length);
	addr += length;
	setSn_TX_WR(s, addr);
	_tx_wr_cache[s] = addr;
}

void readFromRXBuffer(u_char s, u_char *array, u_int length) {
	u_int addr = 0;
	if (length == 0) {
		return;
	}
	addr = getSn_RX_RD(s);
	readMemoryArray(addr, _socket_rxb_block[s], array, length);
	addr += length;
	setSn_RX_RD(s, addr);
	_rx_rd_cache[s] = addr;
}

void readFromRXBufferPiecemeal(u_char s, u_char *array, u_int length) {
	u_int addr = 0;
	if (length == 0) {
		return;
	}
	addr = _rx_rd_cache[s];
	readMemoryArray(addr, _socket_rxb_block[s], array, length);
	addr += length;
	setSn_RX_RD(s, addr);
	_rx_rd_cache[s] = addr;
}

void flushRXBufferPiecemeal(u_char s, u_int length) {
	u_int addr = 0;
	if (length == 0) {
		return;
	}
	addr = _rx_rd_cache[s];
	// No need to read a thing... just skip RX_RD past it
	addr += length;
	setSn_RX_RD(s, addr);
	_rx_rd_cache[s] = addr;
}

void refreshTXBufferCache(u_char s) {
	_tx_wr_cache[s] = getSn_TX_WR(s);
}

void refreshRXBufferCache(u_char s) {
	_rx_rd_cache[s] = getSn_RX_RD(s);
}

/**
 * copy local buffer to W5500 and send data
 */
u_int send(u_char s, const u_char * buffer, u_int * length, u_char retry) {
	u_char status = 0; // TODO define statuses
	u_int txPointerBefore, txPointerAfter;

	if (!retry) {
		do {
			status = getSn_SR(s);
			if ((status != SOCK_ESTABLISHED) && (status != SOCK_CLOSE_WAIT)) {
				return 2;
			}
		} while (getTXFreeSize(s) < *length);

		writeToTXBuffer(s, (u_char *) buffer, *length);
	}

	txPointerBefore = getSn_TX_RD(s);

	setSn_CR(s, Sn_CR_SEND);
	while (getSn_CR(s))
		;

	while ((getSn_IR(s) & Sn_IR_SEND_OK) != Sn_IR_SEND_OK) {
		if (getSn_IR(s) == SOCK_CLOSED) {
			close_s(s);
			return 3;
		}
	}
	setSn_IR(s, Sn_IR_SEND_OK); // wasn't SEND_OK set already internally?

	txPointerAfter = getSn_TX_RD(s);
	refreshTXBufferCache(s);

	*length = txPointerAfter - txPointerBefore;
	if (txPointerAfter > txPointerBefore) {
		return 1;
	} else {
		return 0;
	}
}


/**
 * copy received data from W5500's RX buffer to the local buffer
 */
void receive(u_char s, u_char * buffer, u_int length) {
	readFromRXBuffer(s, buffer, length);
	setSn_CR(s, Sn_CR_RECV);
	while (getSn_CR(s))
		;
}

void clearBuffer(u_char* array, u_int length) {
	while ( length != 0) {
		*array++ = 0;
		length--;
	}
}

u_int getTXFreeSize(u_char s) {
	u_int firstRead = 0, secondRead = 0;
	do {
		secondRead = getSn_TX_FSR(s);
		if (secondRead != 0) {
			firstRead = getSn_TX_FSR(s);
		}
	} while (firstRead != secondRead);
	return firstRead;
}

u_int getTXVirtualFreeSize(u_char s) {
	u_int firstRead = 0, secondRead = 0;
	do {
		secondRead = getSn_TX_RD(s);
		if (secondRead != 0) {
			firstRead = getSn_TX_RD(s);
		}
	} while (firstRead != secondRead);

	if (firstRead == _tx_wr_cache[s])
		return 65535;
	return firstRead - _tx_wr_cache[s];
}

u_int getRXReceived(u_char s) {
	u_int firstRead = 0, secondRead = 0;
	do {
		secondRead = getSn_RX_RSR(s);
		if (secondRead != 0) {
			firstRead = getSn_RX_RSR(s);
		}
	} while (firstRead != secondRead);
	return firstRead;
}

u_int getVirtualRXReceived(u_char s) {
	u_int firstRead = 0, secondRead = 0;
	do {
		secondRead = getSn_RX_WR(s);
		if (secondRead != 0) {
			firstRead = getSn_RX_WR(s);
		}
	} while (firstRead != secondRead);
	return firstRead - _rx_rd_cache[s];
}

u_int ntohs(u_char *array) {
	u_int val;

	val = array[1];
	val |= array[0] << 8;
	return val;
}

void htons(u_int val, u_char *array)
{
	array[1] = val & 0x00FF;
	array[0] = val >> 8;
}
