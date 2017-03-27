/* wizdebug.c
 * WizNet W5200 Ethernet Controller Driver for MSP430
 * Debug printf routine for WizNet Library
 *
 * Packaged up for RobG's W5500 library
 *
 *
 * Parts derived from Kevin Timmerman's tiny printf from 43oh.com
 * Link: http://forum.43oh.com/topic/1289-tiny-printf-c-version/
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
#include <stdint.h>
#include <stdarg.h>
#include "wizdebug.h"

/* User-provided putc, puts functions go here */
void wiznet_debug_puts(const char *str)
{
	unsigned int c;

	while ( (c = (unsigned int)(*str++)) != '\0' ) {
		#ifdef __MSP430F5529
		UCA1TXBUF = c;
		while (UCA1STAT & UCBUSY)
			;
		#else
		UCA0TXBUF = c;
		//while (UCA0STAT & UCBUSY)
			;
		#endif
	}
}

void wiznet_debug_putc(unsigned int c)
{
#ifdef __MSP430F5529
	UCA1TXBUF = c;
	while (UCA1STAT & UCBUSY)
		;
#else
	UCA0TXBUF = c;
	//while (UCA0STAT & UCBUSY)
		;
#endif
}

#ifdef __MSP430F5529
void wiznet_debug_init()
{
	// F5529LP USCI_A1 Backchannel UART init
	UCA1IFG = 0;
	UCA1CTL0 = 0x00;
	UCA1CTL1 = UCSSEL_2 | UCSWRST;

	// 115200 @ 16MHz UCOS16=1
	UCA1BR0 = 8;
	UCA1BR1 = 0;
	UCA1MCTL = UCBRS_0 | UCBRF_11 | UCOS16;

	// Port sel
	P4SEL |= BIT4|BIT5;  // USCI_A1

	// Ready
	UCA1CTL1 &= ~UCSWRST;
}
#else
void wiznet_debug_init()
{
	// USCI_A0 Backchannel UART
	// G2xxx series USCI vs. F5xxx/F6xxx USCI
	#ifdef __MSP430_HAS_USCI__
	IFG2 &= ~(UCA0TXIFG | UCA0RXIFG);
	#elif defined(__MSP430_HAS_USCI_A0__)
	UCA0IFG = 0;
	#endif

	UCA0CTL0 = 0x00;
	UCA0CTL1 = UCSSEL_2 | UCSWRST;

	// 115200 @ 16MHz UCOS16=1
	//UCA0BR0 = 8;
	//UCA0BR1 = 0;
	//UCA0MCTL = UCBRS_0 | UCBRF_11 | UCOS16;

	// 9600 @ 16MHz UCOS16=1
	UCA0BR0 = 104;
	UCA0BR1 = 0;
	//UCA0MCTL = UCBRS_0 | UCBRF_3 | UCOS16;

	#ifdef __MSP430_HAS_USCI__
	// G2xxx series
		#ifdef __MSP430_HAS_TB3__  // G2xx4 & G2xx5 series
		P3SEL |= BIT4 | BIT5;
		P3SEL2 &= ~(BIT4 | BIT5);
		#else                      // G2xx3 series
		P1SEL |= BIT1|BIT2;  // USCIA
		P1SEL2 |= BIT1|BIT2; //
		#endif
	#endif

	#ifdef __MSP430_HAS_USCI_A0__
		// F5xxx series
		#ifdef __MSP430F5172
		P1SEL |= BIT1|BIT2;  // USCIA
		#endif
	#endif

	// Ready
	UCA0CTL1 &= ~UCSWRST;
}
#endif




void wiznet_debug_putc(unsigned int);
void wiznet_debug_puts(const char *);

static const unsigned long dv[] = {
//  4294967296      // 32 bit unsigned max
    1000000000,     // +0
     100000000,     // +1
      10000000,     // +2
       1000000,     // +3
        100000,     // +4
//       65535      // 16 bit unsigned max     
         10000,     // +5
          1000,     // +6
           100,     // +7
            10,     // +8
             1,     // +9
};

static void xtoa(unsigned long x, const unsigned long *dp)
{
    char c;
    unsigned long d;
    if(x) {
        while(x < *dp) ++dp;
        do {
            d = *dp++;
            c = '0';
            while(x >= d) ++c, x -= d;
            wiznet_debug_putc(c);
        } while(!(d & 1));
    } else
        wiznet_debug_putc('0');
}

static void puth(unsigned int n)
{
    static const char hex[16] = { '0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F' };
    wiznet_debug_putc(hex[n & 15]);
}
 
void wiznet_debug_printf(char *format, ...)
{
    char c;
    int i;
    long n;
    
    va_list a;
    va_start(a, format);
    while( (c = *format++) ) {
        if(c == '%') {
            switch(c = *format++) {
                case 's':                       // String
                    wiznet_debug_puts(va_arg(a, char*));
                    break;
                case 'c':                       // Char
                    wiznet_debug_putc(va_arg(a, int));   // Char gets promoted to Int in args, so it's an int we're looking for (GCC warning)
                    break;
                case 'i':                       // 16 bit Integer
                case 'd':                       // 16 bit Integer
                case 'u':                       // 16 bit Unsigned
                    i = va_arg(a, int);
                    if( (c == 'i' || c == 'd') && i < 0 ) i = -i, wiznet_debug_putc('-');
                    xtoa((unsigned)i, dv + 5);
                    break;
                case 'l':                       // 32 bit Long
                case 'n':                       // 32 bit uNsigned loNg
                    n = va_arg(a, long);
                    if(c == 'l' &&  n < 0) n = -n, wiznet_debug_putc('-');
                    xtoa((unsigned long)n, dv);
                    break;
                case 'x':                       // 16 bit heXadecimal
                    i = va_arg(a, int);
                    puth(i >> 12);
                    puth(i >> 8);
                    puth(i >> 4);
                    puth(i);
                    break;
                case 'h':			// 8 bit Hexadecimal
                    i = va_arg(a, int) & 0xFF;
                    puth(i >> 4);
                    puth(i);
                    break;
                case 0: return;
                default: goto bad_fmt;
            }
        } else
bad_fmt:    if (c != '\n') { wiznet_debug_putc(c); } else { wiznet_debug_putc('\r'); wiznet_debug_putc('\n'); }  // UART CR-LF support
    }
    va_end(a);
}

