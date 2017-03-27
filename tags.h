#ifndef _TAGSH_
#define _TAGSH_

// HTTP request
const u_char sGET[] = "GET /";
// HTTP response header
const u_char sRESPONSE_STATUS_OK[] = "HTTP/1.1 200 OK\r\n";
const u_char sRESPONSE_STATUS_BAD_REQ[] = "HTTP/1.1 400 Bad Request\r\n";
const u_char sRESPONSE_CONTENT_TYPE_XML[] = "Content-Type: text/xml\r\n";
const u_char sRESPONSE_CONTENT_LENGTH[] = "Content-Length:     "; // 4 spaces could be replaced in W5200's buffer with the length after all content has been generated, but not sure if it's possible
const u_char sNEW_LINE[] = "\r\n";
const u_char sREQUEST_GET[] = "GET ";
const u_char sREQUEST_HTTP[] = " HTTP/1.1";

// XML
const u_char sXML_DECLARATION[] = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>";
const u_char sCLOSE_TAG[] = "\">";
const u_char sCLOSE_ATTR[] = "\" ";
//
const u_char sSTATUS_OPEN[] = "<status>";
const u_char sSTATUS_CLOSE[] = "</status>";

const u_char sDMX_OPEN[] =
		"<dmx xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xsi:noNamespaceSchemaLocation=\"http://430things.com/dmx.xsd\">";
const u_char sDMX_CLOSE[] = "</dmx>";
const u_char sUNIVERSE_OPEN[] = "<universe number=\"";
const u_char sUNIVERSE_CLOSE[] = "</universe>";
const u_char sCHANNEL_OPEN[] = "<channel number=\"";
const u_char sCHANNEL_CLOSE[] = "</channel>";

const u_char sCHANNELS_ATTR[] = "channels=\"";
const u_char sFPMS_ATTR[] = "fpms=\"";
const u_char sSTART_BYTE_ATTR[] = "startbyte=\"";

#endif

