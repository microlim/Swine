/*
***************************************************************************
*
* Author: Teunis van Beelen
*
* Copyright (C) 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013 Teunis van Beelen
*
* teuniz@gmail.com
*
***************************************************************************
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation version 2 of the License.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License along
* with this program; if not, write to the Free Software Foundation, Inc.,
* 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*
***************************************************************************
*
* This version of GPL is at http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt
*
***************************************************************************
*/

/* last revision: February 1, 2013 */

/* For more info and how to use this libray, visit: http://www.teuniz.net/RS-232/ */


#ifndef rs232_INCLUDED
#define rs232_INCLUDED



#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include <termios.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <limits.h>
#include <stdbool.h>

#include "common.h"

void RS485Ctrl_flow( char onoff ) ;

int RS232_OpenComport(int, int);
int RS232_PollComport(int, unsigned char *, int);
int RS232_SendByte(int, unsigned char);

int RS232_SendBuf(int, unsigned char *, int);
int RS485_SendBuf(int comport_number, unsigned char *buf, int size) ;

void RS232_CloseComport(int);
void RS232_cputs(int, const char *);
int RS232_IsCTSEnabled(int);
int RS232_IsDSREnabled(int);
void RS232_enableDTR(int);
void RS232_disableDTR(int);
void RS232_enableRTS(int);
void RS232_disableRTS(int);

bool USART1_RxQuePush( u8 data ) ;
bool USART1_RxQuePop( u8 *data ) ;
bool USART2_RxQuePush( u8 data ) ;
bool USART2_RxQuePop( u8 *data ) ;
bool USART3_RxQuePush( u8 data ) ;
bool USART3_RxQuePop( u8 *data ) ;
bool USART4_RxQuePush( u8 data ) ;
bool USART4_RxQuePop( u8 *data ) ;

bool UARTFifoPop( LPUART_DATAQUE pQue, u8 *data ) ;

bool UART1PacketStreaming( void ) ;
bool UART2PacketStreaming( void ) ;
bool UART3PacketStreaming( void ) ;
bool UART4PacketStreaming( void ) ;

void USART_SendPacket( int portnum, LPCOMMPACKET pPacket ) ;

extern COMMPACKET        USART1StreamingBuffer;
extern COMMPACKET        USART2StreamingBuffer;
extern COMMPACKET        USART3StreamingBuffer;
extern COMMPACKET        USART4StreamingBuffer;

extern UART_DATAQUE			USART1RxDataQue;      // Que for the Receive
extern UART_DATAQUE			USART1TxDataQue;    	// Que for the Transmit


extern UART_DATAQUE			USART2RxDataQue;      // Que for the Receive
extern UART_DATAQUE			USART2TxDataQue;    	// Que for the Transmit
extern u8								USART2Rxbuf[10];

extern UART_DATAQUE			USART3RxDataQue;      // Que for the Receive
extern UART_DATAQUE			USART3TxDataQue;    	// Que for the Transmit
extern u8								USART3Rxbuf[10];

extern UART_DATAQUE			USART4RxDataQue;      // Que for the Receive
extern UART_DATAQUE			USART4TxDataQue;    	// Que for the Transmit


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif


