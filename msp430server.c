#include "msp.h"
#include "defines.h"
#include "msp430server.h"
#include "w5500.h"
#include "tags.h"
#include "driverlib.h"
#include <stdio.h>

#define _delay_cycles(x) __delay_cycles(x)

///////////////////////////////////////////////////////////////
// W5200 network configuration
///////////////////////////////////////////////////////////////
const u_char MAC[6] = { 0x00, 0x08, 0xDC, 0x05, 0xB4, 0x38 }; // WIZnet's base address: 00-08-DC (04-03-00, can you guess what that is?)
//const u_char IP[4] = { 192, 168, 168, 43 }; // local IP
//const u_char gatewayIP[4] = { 192, 168, 168, 2 }; // gateway IP
//const u_char subnetMask[4] = { 255, 255, 255, 0 }; // subnet mask
//
// additional network configuration for client mode
//const u_char destinationIP[4] = { 192, 168, 168, 5 }; // destination IP
//const u_int destinationPort = 80; // destination port

///////////////////////////////////////////////////////////////

u_char ch_status[MAX_SOCK_NUM]; /** 0:closed, 1:ready, 2:connected */
u_char txBuffer[TX_MAX_BUF_SIZE]; // TX Buffer for applications
u_char rxBuffer[RX_MAX_BUF_SIZE]; // RX Buffer for applications

u_char lastByte = 0;
u_char readBufferPointer = 0;
u_char writeBufferPointer = 0;
u_int bytesReceived = 0;

u_char dmx[2][64];
u_char universe = 0;
u_char channel = 0;

///////////////////////////////////////////////////////
// Response section
///////////////////////////////////////////////////////
void openDocument() {

	// xml declaration and dmx open tag
	addStringToBuffer(sXML_DECLARATION);
	addStringToBuffer(sDMX_OPEN);

}

void closeDocument(u_char success) {

	addStringToBuffer(sSTATUS_OPEN);
	addCharToBuffer(success);
	addStringToBuffer(sSTATUS_CLOSE);
	addStringToBuffer(sDMX_CLOSE);

}

/////////////////////////////////////////////////////////
// HTTP response headers
/////////////////////////////////////////////////////////
void addHTTP400ResponseToBuffer() {
	addStringToBuffer(sRESPONSE_STATUS_BAD_REQ);
}

void addHTTP200ResponseToBuffer() {
	addStringToBuffer(sRESPONSE_STATUS_OK);
	addStringToBuffer(sRESPONSE_CONTENT_TYPE_XML);
	addStringToBuffer(sNEW_LINE);
}

//////////////////////////////////////////////////
// Process request
//////////////////////////////////////////////////
void processRequest(Request *request) {
	u_char success = '1';
	openDocument();
	u_char u = 0;
	while (u < 1) {
		u_char c = 0;
		addStringToBuffer(sUNIVERSE_OPEN);
		addCharToBufferAsHex(u);
		addStringToBuffer(sCLOSE_TAG);
		while (c < 32) {
			addStringToBuffer(sCHANNEL_OPEN);
			addCharToBufferAsHex(c);
			addStringToBuffer(sCLOSE_TAG);
			//addStringToBuffer("Ian is the fucking worst");
			addCharToBufferAsHex(c);
			addStringToBuffer(sCHANNEL_CLOSE);
			c++;
		}
		addStringToBuffer(sUNIVERSE_CLOSE);
		u++;
	}
	closeDocument(success);
}

////////////////////////////////////////////////////////////
// Parse request
////////////////////////////////////////////////////////////
u_char parseRequest(Request *request) {

	u_char done = 0;
	u_char byte = 0;
	u_char counter = 0;
	u_char state = 0;
	u_char param_value = 0;

	// find GET
	while (!done && getByteFromBuffer(&byte)) {
		if (sGET[counter] == byte) {
			counter++;
			if (counter == 5) {
				done = 1;
			}
		} else if (counter > 0) {
			counter = 0;
			if (sGET[counter] == byte) {
				counter++;
			}
		}
	}
	// found GET, get the rest
	if (done) {
		done = 0;
		char hex = 0;
		u_char property = 0;
		state = REQ_ACTION; // first byte after sGET string is action
		while (!done && getByteFromBuffer(&byte)) {
			if (byte == ' ') { // we are done with the request
				done = 1;
			} else if (state == REQ_ACTION) {
				request->action = byte;
				state = 0;
			} else if (byte == '&' || byte == '?') { // we expect parameter's name after action or previous parameter's value
				state = PARAM_NAME;
			} else if (byte == '=') { // parameter's value is next
				state = PARAM_VALUE;
				hex = 0;
				param_value = 0;
			} else if (state == PARAM_NAME) {
				state = 0;
				switch (byte) {
				case 'u':
					property = REQ_UNIVERSE;
					break;
				case 'c':
					property = REQ_CHANNEL;
					break;
				case 'v':
					property = REQ_VALUE;
					hex = 0;
					break;
				default:
					property = REQ_IGNORE;
				}
			} else if (property == REQ_VALUE) { //this is used for the stream of hex value pairs: v=00FF010F...
				if (hex == 0) { // MSB nibble
					hex = 1;
					byte = asciiToHex(byte);
					if (byte == 0xFF) { // bad char, exit
						done = 1;
					} else {
						param_value = byte << 4;
					}
				} else { // LSB nibble
					hex = 0;
					byte = asciiToHex(byte);
					if (byte == 0xFF) { // bad char, exit
						done = 1;
					} else {
						param_value += byte;
						dmx[universe][channel] = param_value;
						channel++;
					}
					if (channel == 64) // reached max, ignore the rest
						done = 1;
				}

			} else { // state = PARAM_VALUE
				if (byte == 0x30 && hex == 0) { // this might be hex
					hex = 1;
				} else if (byte == 0x78 && hex == 1) { // yes, it is hex
					hex = 2;
					param_value = 0;
					continue;
				}

				if (hex == 2) {
					byte = asciiToHex(byte);
					if (byte == 0xFF) { // bad value, reset all
						hex = 0;
						byte = 0;
						param_value = 0;
					} else {
						param_value = (param_value << 4) + byte; // 8 bit hex
					}
				} else if (byte > 0x2F && byte < 0x3A) { // digit? (numeric)
					param_value = (param_value * 10) + (byte - 0x30); // parameters can have multiple digits
				} else {
					param_value = byte;
				}

				switch (property) { // assign value
				case REQ_UNIVERSE:
					universe = param_value;
					break;
				case REQ_CHANNEL:
					channel = param_value;
					break;
				case REQ_IGNORE:
					break;
				}
			}
		}
		return done;
	} else {
		return 0;
	}
}

///////////////////////////////////////////
//
///////////////////////////////////////////
void sendRequest() {
	addStringToBuffer(sREQUEST_GET);
	addStringToBuffer((const u_char*) "/mypage?button=");
	char someCondition = 0;
	if (someCondition) {
		addStringToBuffer((const u_char*) "ON");
	} else {
		addStringToBuffer((const u_char*) "OFF");
	}
	addStringToBuffer(sREQUEST_HTTP);
	addStringToBuffer(sNEW_LINE);
	addStringToBuffer(sNEW_LINE);
	flushBuffer();
}

//////////////////////////////////////////////
//
//////////////////////////////////////////////
void configureW5500(const u_char *sourceIP, const u_char *gatewayIP,
		const u_char *subnetMask) {

	setSHAR((u_char *) MAC);
	setSUBR((u_char *) subnetMask);
	setGAR((u_char *) gatewayIP);
	setSIPR((u_char *) sourceIP);

	// set PTR and RCR register
	setRTR(6000);
	setRCR(3);

}

void configureMSP430() {

	/*
		BCSCTL1 = CALBC1_16MHZ; // 16MHz clock
		DCOCTL = CALDCO_16MHZ;

		//configure pins
		P2OUT |= WIZ_CS_PIN + WIZ_RST_PIN;
		P2DIR |= WIZ_CS_PIN + WIZ_RST_PIN;

		//configure USCI
		// setup UCB0 or UCB1
		P1SEL |= WIZ_SCLK_PIN + WIZ_MOSI_PIN + WIZ_MISO_PIN;
		P1SEL2 |= WIZ_SCLK_PIN + WIZ_MOSI_PIN + WIZ_MISO_PIN;

		UCB0CTL0 = UCCKPH + UCMSB + UCMST + UCSYNC; // 3-pin, 8-bit SPI master
		UCB0CTL1 |= UCSSEL_2; // SMCLK
		UCB0BR0 |= 0x02; // 1:2 because @ 1:1, we see MISO errors
		UCB0BR1 = 0;
		UCB0CTL1 &= ~UCSWRST; // clear SW

		u_char c = 0;
		while (c < 64) {
			dmx[0][c] = 0;
			dmx[1][c] = 0;
			c++;
		}
		*/
	    MAP_GPIO_setAsOutputPin(ETH_CS_PORT,
	                        ETH_CS_PIN);

	    MAP_GPIO_setOutputLowOnPin(ETH_CS_PORT,
	                           ETH_CS_PIN);
	    //
	    // Configure SPI peripheral.
	    //


	    eUSCI_SPI_MasterConfig spiMasterConfig =
	    {
	        EUSCI_A_SPI_CLOCKSOURCE_SMCLK,                      		// SMCLK Clock Source
	        MAP_CS_getSMCLK(),                                  			// Get SMCLK frequency
	        8000000,                                                	// SPICLK = 20 MHz
	        EUSCI_A_SPI_MSB_FIRST,                             			// MSB First
			EUSCI_A_SPI_PHASE_DATA_CAPTURED_ONFIRST_CHANGED_ON_NEXT, 	// Phase //  EUSCI_SPI_PHASE_DATA_CHANGED_ONFIRST_CAPTURED_ON_NEXT EUSCI_SPI_PHASE_DATA_CAPTURED_ONFIRST_CHANGED_ON_NEXT
	        EUSCI_A_SPI_CLOCKPOLARITY_INACTIVITY_LOW,         			// Low polarity
			EUSCI_A_SPI_3PIN                                   			// SPI Mode
	    };


	    MAP_GPIO_setAsPeripheralModuleFunctionOutputPin(ETH_MOSI_PORT,
														ETH_MOSI_PIN,
														GPIO_PRIMARY_MODULE_FUNCTION); //MOSI

		MAP_GPIO_setAsPeripheralModuleFunctionInputPin(ETH_MISO_PORT,
													ETH_MISO_PIN,
													GPIO_PRIMARY_MODULE_FUNCTION); //MISO

		MAP_GPIO_setAsPeripheralModuleFunctionOutputPin(ETH_SCLK_PORT,
													ETH_SCLK_PIN,
													GPIO_PRIMARY_MODULE_FUNCTION); //SCLK
		//UCB0BR0 |= 0x02;
		SPI_initMaster(ETH_EUSCI_MODULE, &spiMasterConfig);
	    SPI_enableModule(ETH_EUSCI_MODULE);

	    SPI_enableInterrupt(ETH_EUSCI_MODULE, ETH_EUSCI_REC_INT);
	    Interrupt_enableInterrupt(ETH_INT_ENABLE);
	    SPI_clearInterruptFlag(ETH_EUSCI_MODULE,  ETH_EUSCI_REC_INT);


}

void startClient(u_char s, u_char *destinationIP, u_char port) {
	// make sure socket is closed
	waitUntilSocketClosed(s);
	// open socket on arbitrary port
	openSocketOnPort(s, 0);
	// connect
	connect(s, destinationIP, port);
	// verify connection was established
	// TODO add timer/counter to prevent endless loop when connection cannot be established
	while (!isConnected(s))
		;
}

void stopClient(u_char s) {
	disconnect(s);
	close_s(s);
}

void startServer(u_char s, u_char port) {
	// make sure socket is closed
	waitUntilSocketClosed(s);
	// open socket on port 80
	openSocketOnPort(s, 80);
	// start listening
	startListening(s);
}

void stopServer(u_char s) {
	// flush buffer
	addStringToBuffer(sNEW_LINE);
	//TODO check return status and length, status should be 1 and length = 0;
	flushBuffer();
	// disconnect & close socket
	disconnect(s);
	close_s(s);
}

void flushBuffer() {
	//TODO check return status and length, status should be 1 and length = 0;
	u_int length = writeBufferPointer & 0x00FF;
	send(0, txBuffer, &length, 0);
	writeBufferPointer = 0;
}

void addCharToBuffer(u_char character) {
	txBuffer[writeBufferPointer++] = character;
	if (writeBufferPointer == TX_MAX_BUF_SIZE) {
		//TODO check return status and length, status should be 1 and length = 0;
		u_int length = writeBufferPointer & 0x00FF;
		send(0, txBuffer, &length, 0);
		writeBufferPointer = 0;
	}
}

u_char getByteFromBuffer(u_char *byte) {
	if (lastByte == readBufferPointer) { // first time or last byte was read from the local buffer
		if (bytesReceived > RX_MAX_BUF_SIZE) { // received more bytes than we can fit into our RX buffer
			lastByte = RX_MAX_BUF_SIZE;
			bytesReceived -= RX_MAX_BUF_SIZE;
		} else {
			lastByte = bytesReceived; // remaining bytes
		}
		receive(0, rxBuffer, lastByte); // get more from W5200
		readBufferPointer = 0;
	}

	if (lastByte == 0) { // if there are no more bytes in W5200's RX memory...
		return 0; // we are done, return failure
	}

	*byte = rxBuffer[readBufferPointer++]; // copy byte

	return 1; // we have more data, return success
}

void waitForData(u_char s) {
	while ((bytesReceived = getRXReceived(s)) == 0)
		;
	//TODO add logic to verify bytesReceived == getSn_RX_RSR(s), indicating all bytes were received
}

void waitForConnection(u_char s) {
	while (getSn_SR(s) != SOCK_ESTABLISHED)
		;
}

u_char isConnected(u_char s) {
	return (getSn_SR(s) == SOCK_ESTABLISHED);
}

void addStringToBuffer(const u_char *string) {
	while (*string) {
		addCharToBuffer(*string++);
	}
}

void addIntToBufferAsHex(u_int i) {
	addStringToBuffer((const u_char*) "0x");
	addCharToBuffer(toHex(i >> 12));
	addCharToBuffer(toHex(i >> 8));
	addCharToBuffer(toHex(i >> 4));
	addCharToBuffer(toHex(i));
}

void addCharToBufferAsHex(u_char c) {
	addStringToBuffer((const u_char*) "0x");
	addCharToBuffer(toHex(c >> 4));
	addCharToBuffer(toHex(c));
}

u_char toHex(u_char c) {
	return "0123456789ABCDEF"[c & 0x0F];
}
/*
 * Send and return byte via SPI
 */
u_char sendReceiveByteSPI(u_char byte) {
	uint8_t receivedByte;
	//UCB0TXBUF = byte;
	//while(!(UCB0IFG & UCTXIFG));
	//while(UCB0STAT & UCBUSY);
	//receivedByte = UCB0RXBUF;
	while (!(SPI_getInterruptStatus(ETH_EUSCI_MODULE,ETH_EUSCI_TRAN_INT)));

	SPI_transmitData(ETH_EUSCI_MODULE, byte);
	while (!(SPI_getInterruptStatus(ETH_EUSCI_MODULE,ETH_EUSCI_TRAN_INT)));
	receivedByte = SPI_receiveData(ETH_EUSCI_MODULE);
	//printf("0x%x\n", receivedByte);
	return receivedByte;
}

/*
 * Convert one ASCII character to hex (0x00-0x0F)
 * Return 0xFF when invalid
 */
u_char asciiToHex(u_char byte) {
	if (byte > 0x2F && byte < 0x3A) { // digit? (hex)
		byte -= 0x30;
	} else if (byte > 0x40 && byte < 0x47) { // upper case hex?
		byte -= 0x37;
	} else if (byte > 0x60 && byte < 0x67) { // lower case hex?
		byte -= 0x57;
	} else { // invalid hex char, reset all and return?
		byte = 0xFF;
	}
	return byte;
}

/*
 * W5500 hardware reset
 *
void resetW5500(void) {
	WIZ_RESET_0;
	delay_us(20);
	WIZ_RESET_1;
	delay_ms(200);
}
*/
void delay_us(u_char time_us) {
	u_char c = 0;
	while (c++ < time_us) {
		_delay_cycles(48);
	}
}

void delay_ms(u_int time_ms) {
	u_int c = 0;
	while (c++ < time_ms) {
		_delay_cycles(48000);
	}
}

