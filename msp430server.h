#ifndef _MSP430SERVER_H_
#define _MSP430SERVER_H_
//
#include "typedefs.h"
//
void configureW5500(const u_char *sourceIP, const u_char *gatewayIP, const u_char *subnetMask);
void configureMSP430();
void resetW5500(void);
//
void addStringToBuffer(const u_char *string);
void addCharToBuffer(u_char character);
void addIntToBufferAsHex(u_int i);
void addCharToBufferAsHex(u_char c);
//
void startClient(u_char s, u_char *destinationIP, u_char port);
void stopClient(u_char s);
void startServer(u_char s, u_char port);
void stopServer(u_char s);
//
void flushBuffer();
void sendRequest();
//
void addHTTP400ResponseToBuffer();
void addHTTP200ResponseToBuffer();
//
void openDocument();
void closeDocument(u_char success);
//
void waitForData(u_char s);
void waitForConnection(u_char s);
u_char isConnected(u_char s);
u_char parseRequest(Request *request);
void processRequest(Request *request);
//
u_char sendReceiveByteSPI(u_char byte);
u_char getByteFromBuffer(u_char *byte);
u_char toHex(u_char);
u_char asciiToHex(u_char byte);
//
void delay_us(u_char time_us);
void delay_ms(u_int time_ms);

#endif
