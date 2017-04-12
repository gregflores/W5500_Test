#include <msp.h>
#include <stdlib.h>
#include "w5500.h"
#include "msp430server.h"
#include "dhcplib.h"
#include "wizdebug.h"
#include <stdio.h>
#include "driverlib.h"

#ifdef __GNUC__
#define _delay_cycles(x) __delay_cycles(x)
#endif

void runAsServer();
void runAsClient();
// used for client example
void waitForEvent();
//
// network configuration
const u_char sourceIP[4] = { 192, 168, 1, 10 }; // local IP
const u_char gatewayIP[4] = { 192, 168, 1, 1 }; // gateway IP
const u_char subnetMask[4] = { 255, 255, 255, 0 }; // subnet mask
// network configuration for client mode
const u_char destinationIP[4] = { 192, 168, 1, 1 }; // destination IP
const u_int destinationPort = 80; // destination port

/*
 * main.c
 */
int main(void) {
	int ret;
	struct DHCPrenew dhcplease;
	MAP_WDT_A_holdTimer();
    MAP_Interrupt_disableMaster();
    FPU_enableModule();
    FlashCtl_setWaitState(FLASH_BANK0, 2);
    FlashCtl_setWaitState(FLASH_BANK1, 2);
    PCM_setPowerState(PCM_AM_DCDC_VCORE1);
    CS_setDCOCenteredFrequency(CS_DCO_FREQUENCY_48);
    CS_setDCOFrequency(48000000);
    MAP_CS_initClockSignal(CS_MCLK, CS_DCOCLK_SELECT, CS_CLOCK_DIVIDER_1);
    MAP_CS_initClockSignal(CS_SMCLK, CS_DCOCLK_SELECT, CS_CLOCK_DIVIDER_1);
    MAP_CS_initClockSignal(CS_HSMCLK, CS_DCOCLK_SELECT, CS_CLOCK_DIVIDER_1);
    MAP_CS_initClockSignal(CS_ACLK, CS_REFOCLK_SELECT, CS_CLOCK_DIVIDER_1);
    _delay_cycles(1000000);
	//wiznet_debug_init();

	// Wait for P1.3 button to be pressed before writing to UART; helps with MSP430G2 LaunchPad serial bug
	//P1SEL &= ~BIT3; P1SEL2 &= ~BIT3; P1DIR &= ~BIT3; P1REN |= BIT3; P1OUT |= BIT3; _delay_cycles(800000);
	//while (P1IN & BIT3) ;
	//wiznet_debug_printf("Beginning:\n");
	printf("Beginning:\n");
	configureMSP430();
	//resetW5500();
	configureW5500(sourceIP, gatewayIP, subnetMask);

	// DHCP stuff
	printf("Waiting for PHY:");
	while ( !(getPHYCFGR() & PHYCFGR_LNK_ON) ){
	;
	}
	printf(" PHY up\n");

	// Configure DHCP; don't care what the DNS server is
	// Note that the currently-configured IP, gateway and subnetmask get nuked at the beginning of this function.
	dhcplease.do_renew = 0;
	ret = dhcp_loop_configure(NULL, &dhcplease);
	if (ret != 0) {
		printf("dhcp_loop_configure returned error: %s\n", dhcp_strerror(dhcplib_errno));
		
		// If DHCP failed, endlessly blink the red LED.
		//P1DIR |= BIT0;
		//P1REN &= ~BIT0;
		//P1OUT |= BIT0;
		MAP_GPIO_setAsOutputPin(GPIO_PORT_P1, GPIO_PIN0);
		while(1) {
			MAP_GPIO_toggleOutputOnPin(GPIO_PORT_P1,GPIO_PIN0);
			_delay_cycles(12000000);
		}
	}

	printf("DHCP lease information: %u seconds, DHCPserver = %d.%d.%d.%d\n", dhcplease.seconds, dhcplease.dhcpserver[0], dhcplease.dhcpserver[1], dhcplease.dhcpserver[2], dhcplease.dhcpserver[3]);

	_delay_cycles(5*48000000);  // 5 seconds
	// Perform DHCP renew
	printf("PERFORMING DHCP RENEWAL\n");
	dhcplease.do_renew = 1;
	ret = dhcp_loop_configure(NULL, &dhcplease);
	if (ret != 0)
		printf("dhcp_loop_configure returned error: %s\n", dhcp_strerror(dhcplib_errno));


	while (1) {
		//runAsServer();
		runAsClient();
	}
}

void runAsServer() {
	//while (1) {
	startServer(0, 80);
	// wait for connection
	// TODO in the future, we will put MCU to sleep and use interrupt to wake it up
	waitForConnection(0);
	// connection was established, now wait for data
	waitForData(0);
	// we've got data, process it
	Request request = { 0, 0, 0, 0 };
	if (parseRequest(&request)) {
		// request is OK, process request
		addHTTP200ResponseToBuffer();
		processRequest(&request);
	} else {
		// cannot parse request
		addHTTP400ResponseToBuffer();
	}
	// flush buffer, disconnect, & close
	stopServer(0);
	//}
}

void runAsClient() {

	while (1) {
		//
		waitForEvent();
		//
		startClient(0, (u_char *) destinationIP, (u_char) destinationPort);

		// send request
		sendRequest();
		// wait for response
		waitForData(0);
		// we've got data, process it
		//TODO we could parse the response and verify status is 200, but for now, let's assume it is OK.
		//processResponse();
		// disconnect & close
		stopClient(0);
	}
}

// client example, wait for button press or some other event
void waitForEvent() {
	// test delay
	_delay_cycles(2*48000000);
}

