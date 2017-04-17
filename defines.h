/*
 * defines.h
 */

#ifndef DEFINES_H_
#define DEFINES_H_

#include "msp.h"
//
//#define RESPONSE_TYPE_HTML
//
#define TX_MAX_BUF_SIZE			0x0F // we can go up to 256, but we would have to decrease RX down to 64, 256/64 (G2553 chip)
#define RX_MAX_BUF_SIZE			0x0F //
//
/* Ethernet controller pin and SPI definitions */
#define ETH_EUSCI_MODULE 		EUSCI_A3_BASE
#define ETH_EUSCI_REC_INT		EUSCI_A_SPI_RECEIVE_INTERRUPT
#define ETH_EUSCI_TRAN_INT		EUSCI_A_SPI_TRANSMIT_INTERRUPT
#define ETH_INT_ENABLE 			INT_EUSCIA3

#define ETH_CS_PIN 				GPIO_PIN4	//ETH CS P9.4
#define ETH_CS_PORT 			GPIO_PORT_P9

#define ETH_SCLK_PIN 			GPIO_PIN5	//ETH CLK P9.5
#define ETH_SCLK_PORT 			GPIO_PORT_P9

#define ETH_MISO_PIN 			GPIO_PIN6	//ETH MISO P9.6
#define ETH_MISO_PORT 			GPIO_PORT_P9

#define ETH_MOSI_PIN 			GPIO_PIN7	//ETH MOSI P9.7
#define ETH_MOSI_PORT 			GPIO_PORT_P9

#define WIZ_SELECT				GPIO_setOutputLowOnPin(ETH_CS_PORT, ETH_CS_PIN);
#define WIZ_DESELECT			GPIO_setOutputHighOnPin(ETH_CS_PORT, ETH_CS_PIN);

//
#define wizPowerUp()			WIZ_POWER_UP
#define wizPowerDown()			WIZ_POWER_DOWN
#define wizSelect()				WIZ_SELECT
#define wizDeselect()			WIZ_DESELECT
//#define wizEnableInterrupt()	WIZ_IE_ENABLE
//#define wizDisableInterrupt()	WIZ_IE_DISABLE
//
#define SOCK_CONFIG				2	// UDP
#define SOCK_DNS				2	// UDP
#define SOCK_DHCP				3	// UDP
#define MAX_BUF_SIZE			1460
#define KEEP_ALIVE_TIME			30	// 30 sec
#define	MAX_SOCK_NUM			8
//
#define WINDOWFULL_FLAG_ON 		1
#define WINDOWFULL_FLAG_OFF 	0
#define WINDOWFULL_MAX_RETRY_NUM 3
#define WINDOWFULL_WAIT_TIME 	1000
//
#define REQ_ACTION				0x01
#define REQ_UNIVERSE			0x02
#define REQ_CHANNEL				0x03
#define REQ_VALUE				0x04
#define REQ_IGNORE				0x05
#define PARAM_NAME				0x06
#define PARAM_VALUE				0x07
//

#endif /* DEFINES_H_ */
