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


#include "rs232.h"
#include "common.h"


UART_DATAQUE			USART1RxDataQue;      // Que for the Receive
UART_DATAQUE			USART1TxDataQue;    	// Que for the Transmit
COMMPACKET        USART1StreamingBuffer;

UART_DATAQUE			USART2RxDataQue;      // Que for the Receive
UART_DATAQUE			USART2TxDataQue;    	// Que for the Transmit
COMMPACKET        USART2StreamingBuffer;
u8								USART2Rxbuf[10];

UART_DATAQUE			USART3RxDataQue;      // Que for the Receive
UART_DATAQUE			USART3TxDataQue;    	// Que for the Transmit
COMMPACKET        USART3StreamingBuffer;
u8								USART3Rxbuf[10];


UART_DATAQUE			USART4RxDataQue;      // Que for the Receive
UART_DATAQUE			USART4TxDataQue;    	// Que for the Transmit
COMMPACKET        USART4StreamingBuffer;


#ifdef __linux__   /* Linux */


int Cport[30],
    error;

struct termios new_port_settings,
       old_port_settings[30];

char comports[30][16]={"/dev/ttyS0","/dev/ttyS1","/dev/ttyS2","/dev/ttyS3","/dev/ttyS4","/dev/ttyS5",
                       "/dev/ttyS6","/dev/ttyS7","/dev/ttyS8","/dev/ttyS9","/dev/ttyS10","/dev/ttyS11",
                       "/dev/ttyS12","/dev/ttyS13","/dev/ttyS14","/dev/ttyS15","/dev/ttyUSB0",
                       "/dev/ttyUSB1","/dev/ttyUSB2","/dev/ttyUSB3","/dev/ttyUSB4","/dev/ttyUSB5",
                       "/dev/ttyAMA0","/dev/ttyAMA1","/dev/ttyACM0","/dev/ttyACM1",
                       "/dev/rfcomm0","/dev/rfcomm1","/dev/ircomm0","/dev/ircomm1"};

#define OUT_ON "echo 255 > /sys/class/leds/led7/brightness"
#define OUT_OFF "echo 0 > /sys/class/leds/led7/brightness"

void RS485Ctrl_flow( char onoff )
{
	if (onoff)
	{
		system(OUT_ON);
	} else {
		system(OUT_OFF);
	}
	usleep(500*100) ;
}

int RS232_OpenComport(int comport_number, int baudrate)
{
  int baudr, status;

  if((comport_number>29)||(comport_number<0))
  {
    printf("illegal comport number\n");
    return(1);
  }

  switch(baudrate)
  {
    case      50 : baudr = B50;
                   break;
    case      75 : baudr = B75;
                   break;
    case     110 : baudr = B110;
                   break;
    case     134 : baudr = B134;
                   break;
    case     150 : baudr = B150;
                   break;
    case     200 : baudr = B200;
                   break;
    case     300 : baudr = B300;
                   break;
    case     600 : baudr = B600;
                   break;
    case    1200 : baudr = B1200;
                   break;
    case    1800 : baudr = B1800;
                   break;
    case    2400 : baudr = B2400;
                   break;
    case    4800 : baudr = B4800;
                   break;
    case    9600 : baudr = B9600;
                   break;
    case   19200 : baudr = B19200;
                   break;
    case   38400 : baudr = B38400;
                   break;
    case   57600 : baudr = B57600;
                   break;
    case  115200 : baudr = B115200;
                   break;
    case  230400 : baudr = B230400;
                   break;
    case  460800 : baudr = B460800;
                   break;
    case  500000 : baudr = B500000;
                   break;
    case  576000 : baudr = B576000;
                   break;
    case  921600 : baudr = B921600;
                   break;
    case 1000000 : baudr = B1000000;
                   break;
    default      : printf("invalid baudrate\n");
                   return(1);
                   break;
  }

  Cport[comport_number] = open(comports[comport_number], O_RDWR | O_NOCTTY | O_NDELAY);
  if(Cport[comport_number]==-1)
  {
    perror("unable to open comport ");
    return(1);
  }

  error = tcgetattr(Cport[comport_number], old_port_settings + comport_number);
  if(error==-1)
  {
    close(Cport[comport_number]);
    perror("unable to read portsettings ");
    return(1);
  }
  memset(&new_port_settings, 0, sizeof(new_port_settings));  /* clear the new struct */

  new_port_settings.c_cflag = baudr | CS8 | CLOCAL | CREAD;
  new_port_settings.c_iflag = IGNPAR;
  new_port_settings.c_oflag = 0;
  new_port_settings.c_lflag = 0;
  new_port_settings.c_cc[VMIN] = 0;      /* block untill n bytes are received */
  new_port_settings.c_cc[VTIME] = 0;     /* block untill a timer expires (n * 100 mSec.) */
  error = tcsetattr(Cport[comport_number], TCSANOW, &new_port_settings);
  if(error==-1)
  {
    close(Cport[comport_number]);
    perror("unable to adjust portsettings ");
    return(1);
  }

  if(ioctl(Cport[comport_number], TIOCMGET, &status) == -1)
  {
    perror("unable to get portstatus");
    return(1);
  }

  status |= TIOCM_DTR;    /* turn on DTR */
  status |= TIOCM_RTS;    /* turn on RTS */

  if(ioctl(Cport[comport_number], TIOCMSET, &status) == -1)
  {
    perror("unable to set portstatus");
    return(1);
  }

  return(0);
}


int RS232_PollComport(int comport_number, unsigned char *buf, int size)
{
  int n;

#ifndef __STRICT_ANSI__                       /* __STRICT_ANSI__ is defined when the -ansi option is used for gcc */
  if(size>SSIZE_MAX)  size = (int)SSIZE_MAX;  /* SSIZE_MAX is defined in limits.h */
#else
  if(size>512)  size = 512;
#endif

  n = read(Cport[comport_number], buf, size);

  return(n);
}


int RS232_SendByte(int comport_number, unsigned char byte)
{
  int n;

  n = write(Cport[comport_number], &byte, 1);
  if(n<0)  return(1);

  return(0);
}


int RS232_SendBuf(int comport_number, unsigned char *buf, int size)
{
  return(write(Cport[comport_number], buf, size));
}


int RS485_SendBuf(int comport_number, unsigned char *buf, int size)
{
	int flag=0;
	RS485Ctrl_flow(1) ;  // transfer mode
  flag=write(Cport[comport_number], buf, size) ;
  RS485Ctrl_flow(0) ;  // receive mode
	return(flag);
}

void RS232_CloseComport(int comport_number)
{
  int status;

  if(ioctl(Cport[comport_number], TIOCMGET, &status) == -1)
  {
    perror("unable to get portstatus");
  }

  status &= ~TIOCM_DTR;    /* turn off DTR */
  status &= ~TIOCM_RTS;    /* turn off RTS */

  if(ioctl(Cport[comport_number], TIOCMSET, &status) == -1)
  {
    perror("unable to set portstatus");
  }

  close(Cport[comport_number]);
  tcsetattr(Cport[comport_number], TCSANOW, old_port_settings + comport_number);
}

/*
Constant  Description
TIOCM_LE  DSR (data set ready/line enable)
TIOCM_DTR DTR (data terminal ready)
TIOCM_RTS RTS (request to send)
TIOCM_ST  Secondary TXD (transmit)
TIOCM_SR  Secondary RXD (receive)
TIOCM_CTS CTS (clear to send)
TIOCM_CAR DCD (data carrier detect)
TIOCM_CD  Synonym for TIOCM_CAR
TIOCM_RNG RNG (ring)
TIOCM_RI  Synonym for TIOCM_RNG
TIOCM_DSR DSR (data set ready)
*/

int RS232_IsCTSEnabled(int comport_number)
{
  int status;

  ioctl(Cport[comport_number], TIOCMGET, &status);

  if(status&TIOCM_CTS) return(1);
  else return(0);
}

int RS232_IsDSREnabled(int comport_number)
{
  int status;

  ioctl(Cport[comport_number], TIOCMGET, &status);

  if(status&TIOCM_DSR) return(1);
  else return(0);
}

void RS232_enableDTR(int comport_number)
{
  int status;

  if(ioctl(Cport[comport_number], TIOCMGET, &status) == -1)
  {
    perror("unable to get portstatus");
  }

  status |= TIOCM_DTR;    /* turn on DTR */

  if(ioctl(Cport[comport_number], TIOCMSET, &status) == -1)
  {
    perror("unable to set portstatus");
  }
}

void RS232_disableDTR(int comport_number)
{
  int status;

  if(ioctl(Cport[comport_number], TIOCMGET, &status) == -1)
  {
    perror("unable to get portstatus");
  }

  status &= ~TIOCM_DTR;    /* turn off DTR */

  if(ioctl(Cport[comport_number], TIOCMSET, &status) == -1)
  {
    perror("unable to set portstatus");
  }
}

void RS232_enableRTS(int comport_number)
{
  int status;

  if(ioctl(Cport[comport_number], TIOCMGET, &status) == -1)
  {
    perror("unable to get portstatus");
  }

  status |= TIOCM_RTS;    /* turn on RTS */

  if(ioctl(Cport[comport_number], TIOCMSET, &status) == -1)
  {
    perror("unable to set portstatus");
  }
}

void RS232_disableRTS(int comport_number)
{
  int status;

  if(ioctl(Cport[comport_number], TIOCMGET, &status) == -1)
  {
    perror("unable to get portstatus");
  }

  status &= ~TIOCM_RTS;    /* turn off RTS */

  if(ioctl(Cport[comport_number], TIOCMSET, &status) == -1)
  {
    perror("unable to set portstatus");
  }
}


#else         /* windows */


HANDLE Cport[16];


char comports[16][10]={"\\\\.\\COM1",  "\\\\.\\COM2",  "\\\\.\\COM3",  "\\\\.\\COM4",
                       "\\\\.\\COM5",  "\\\\.\\COM6",  "\\\\.\\COM7",  "\\\\.\\COM8",
                       "\\\\.\\COM9",  "\\\\.\\COM10", "\\\\.\\COM11", "\\\\.\\COM12",
                       "\\\\.\\COM13", "\\\\.\\COM14", "\\\\.\\COM15", "\\\\.\\COM16"};

char baudr[64];


int RS232_OpenComport(int comport_number, int baudrate)
{
  if((comport_number>15)||(comport_number<0))
  {
    printf("illegal comport number\n");
    return(1);
  }

  switch(baudrate)
  {
    case     110 : strcpy(baudr, "baud=110 data=8 parity=N stop=1 dtr=on rts=on");
                   break;
    case     300 : strcpy(baudr, "baud=300 data=8 parity=N stop=1 dtr=on rts=on");
                   break;
    case     600 : strcpy(baudr, "baud=600 data=8 parity=N stop=1 dtr=on rts=on");
                   break;
    case    1200 : strcpy(baudr, "baud=1200 data=8 parity=N stop=1 dtr=on rts=on");
                   break;
    case    2400 : strcpy(baudr, "baud=2400 data=8 parity=N stop=1 dtr=on rts=on");
                   break;
    case    4800 : strcpy(baudr, "baud=4800 data=8 parity=N stop=1 dtr=on rts=on");
                   break;
    case    9600 : strcpy(baudr, "baud=9600 data=8 parity=N stop=1 dtr=on rts=on");
                   break;
    case   19200 : strcpy(baudr, "baud=19200 data=8 parity=N stop=1 dtr=on rts=on");
                   break;
    case   38400 : strcpy(baudr, "baud=38400 data=8 parity=N stop=1 dtr=on rts=on");
                   break;
    case   57600 : strcpy(baudr, "baud=57600 data=8 parity=N stop=1 dtr=on rts=on");
                   break;
    case  115200 : strcpy(baudr, "baud=115200 data=8 parity=N stop=1 dtr=on rts=on");
                   break;
    case  128000 : strcpy(baudr, "baud=128000 data=8 parity=N stop=1 dtr=on rts=on");
                   break;
    case  256000 : strcpy(baudr, "baud=256000 data=8 parity=N stop=1 dtr=on rts=on");
                   break;
    case  500000 : strcpy(baudr, "baud=500000 data=8 parity=N stop=1 dtr=on rts=on");
                   break;
    case 1000000 : strcpy(baudr, "baud=1000000 data=8 parity=N stop=1 dtr=on rts=on");
                   break;
    default      : printf("invalid baudrate\n");
                   return(1);
                   break;
  }

  Cport[comport_number] = CreateFileA(comports[comport_number],
                      GENERIC_READ|GENERIC_WRITE,
                      0,                          /* no share  */
                      NULL,                       /* no security */
                      OPEN_EXISTING,
                      0,                          /* no threads */
                      NULL);                      /* no templates */

  if(Cport[comport_number]==INVALID_HANDLE_VALUE)
  {
    printf("unable to open comport\n");
    return(1);
  }

  DCB port_settings;
  memset(&port_settings, 0, sizeof(port_settings));  /* clear the new struct  */
  port_settings.DCBlength = sizeof(port_settings);

  if(!BuildCommDCBA(baudr, &port_settings))
  {
    printf("unable to set comport dcb settings\n");
    CloseHandle(Cport[comport_number]);
    return(1);
  }

  if(!SetCommState(Cport[comport_number], &port_settings))
  {
    printf("unable to set comport cfg settings\n");
    CloseHandle(Cport[comport_number]);
    return(1);
  }

  COMMTIMEOUTS Cptimeouts;

  Cptimeouts.ReadIntervalTimeout         = MAXDWORD;
  Cptimeouts.ReadTotalTimeoutMultiplier  = 0;
  Cptimeouts.ReadTotalTimeoutConstant    = 0;
  Cptimeouts.WriteTotalTimeoutMultiplier = 0;
  Cptimeouts.WriteTotalTimeoutConstant   = 0;

  if(!SetCommTimeouts(Cport[comport_number], &Cptimeouts))
  {
    printf("unable to set comport time-out settings\n");
    CloseHandle(Cport[comport_number]);
    return(1);
  }

  return(0);
}


int RS232_PollComport(int comport_number, unsigned char *buf, int size)
{
  int n;

  if(size>4096)  size = 4096;

/* added the void pointer cast, otherwise gcc will complain about */
/* "warning: dereferencing type-punned pointer will break strict aliasing rules" */

  ReadFile(Cport[comport_number], buf, size, (LPDWORD)((void *)&n), NULL);

  return(n);
}


int RS232_SendByte(int comport_number, unsigned char byte)
{
  int n;

  WriteFile(Cport[comport_number], &byte, 1, (LPDWORD)((void *)&n), NULL);

  if(n<0)  return(1);

  return(0);
}


int RS232_SendBuf(int comport_number, unsigned char *buf, int size)
{
  int n;

  if(WriteFile(Cport[comport_number], buf, size, (LPDWORD)((void *)&n), NULL))
  {
    return(n);
  }

  return(-1);
}


void RS232_CloseComport(int comport_number)
{
  CloseHandle(Cport[comport_number]);
}


int RS232_IsCTSEnabled(int comport_number)
{
  int status;

  GetCommModemStatus(Cport[comport_number], (LPDWORD)((void *)&status));

  if(status&MS_CTS_ON) return(1);
  else return(0);
}


int RS232_IsDSREnabled(int comport_number)
{
  int status;

  GetCommModemStatus(Cport[comport_number], (LPDWORD)((void *)&status));

  if(status&MS_DSR_ON) return(1);
  else return(0);
}


void RS232_enableDTR(int comport_number)
{
  EscapeCommFunction(Cport[comport_number], SETDTR);
}


void RS232_disableDTR(int comport_number)
{
  EscapeCommFunction(Cport[comport_number], CLRDTR);
}


void RS232_enableRTS(int comport_number)
{
  EscapeCommFunction(Cport[comport_number], SETRTS);
}


void RS232_disableRTS(int comport_number)
{
  EscapeCommFunction(Cport[comport_number], CLRRTS);
}


#endif


void RS232_cputs(int comport_number, const char *text)  /* sends a string to serial port */
{
  while(*text != 0)   RS232_SendByte(comport_number, *(text++));
}



bool USART1_RxQuePush( u8 data )
{
  u16 tmp = USART1RxDataQue.nPushIndex + 1;
  tmp %= UART_DATAQUE_SIZE;
  if( tmp == USART1RxDataQue.nPopIndex ) { return false; }  // overflow
  
  USART1RxDataQue.nBuffer[USART1RxDataQue.nPushIndex++] = data;
  USART1RxDataQue.nPushIndex %= UART_DATAQUE_SIZE;

  return true;
}

bool USART1_RxQuePop( u8 *data )
{
  // is empty
  if( USART1RxDataQue.nPushIndex == USART1RxDataQue.nPopIndex ) {
    return false;
  }
  
  *data = USART1RxDataQue.nBuffer[USART1RxDataQue.nPopIndex++];
  USART1RxDataQue.nPopIndex %= UART_DATAQUE_SIZE;

  return true;
}

bool USART2_RxQuePush( u8 data )
{
  u16 tmp = USART2RxDataQue.nPushIndex + 1;
  tmp %= UART_DATAQUE_SIZE;
  if( tmp == USART2RxDataQue.nPopIndex ) { return false; }  // overflow
  
  USART2RxDataQue.nBuffer[USART2RxDataQue.nPushIndex++] = data;
  USART2RxDataQue.nPushIndex %= UART_DATAQUE_SIZE;

  return true;
}

bool USART2_RxQuePop( u8 *data )
{
  // is empty
  if( USART2RxDataQue.nPushIndex == USART2RxDataQue.nPopIndex ) {
    return false;
  }
  
  *data = USART2RxDataQue.nBuffer[USART2RxDataQue.nPopIndex++];
  USART2RxDataQue.nPopIndex %= UART_DATAQUE_SIZE;

  return true;
}


bool USART3_RxQuePush( u8 data )
{
  u16 tmp = USART3RxDataQue.nPushIndex + 1;
  tmp %= UART_DATAQUE_SIZE;
  if( tmp == USART3RxDataQue.nPopIndex ) { return false; }  // overflow
  
  USART3RxDataQue.nBuffer[USART3RxDataQue.nPushIndex++] = data;
  USART3RxDataQue.nPushIndex %= UART_DATAQUE_SIZE;

  return true;
}

bool USART3_RxQuePop( u8 *data )
{
  // is empty
  if( USART3RxDataQue.nPushIndex == USART3RxDataQue.nPopIndex ) {
    return false;
  }
  
  *data = USART3RxDataQue.nBuffer[USART3RxDataQue.nPopIndex++];
  USART3RxDataQue.nPopIndex %= UART_DATAQUE_SIZE;

  return true;
}


bool USART4_RxQuePush( u8 data )
{
  u16 tmp = USART4RxDataQue.nPushIndex + 1;
  tmp %= UART_DATAQUE_SIZE;
  if( tmp == USART4RxDataQue.nPopIndex ) { return false; }  // overflow
  
  USART4RxDataQue.nBuffer[USART4RxDataQue.nPushIndex++] = data;
  USART4RxDataQue.nPushIndex %= UART_DATAQUE_SIZE;

  return true;
}

bool USART4_RxQuePop( u8 *data )
{
  // is empty
  if( USART4RxDataQue.nPushIndex == USART4RxDataQue.nPopIndex ) {
    return false;
  }
  
  *data = USART4RxDataQue.nBuffer[USART4RxDataQue.nPopIndex++];
  USART4RxDataQue.nPopIndex %= UART_DATAQUE_SIZE;

  return true;
}

bool UARTFifoPop( LPUART_DATAQUE pQue, u8 *data )
{
  if( pQue->nPushIndex == pQue->nPopIndex ) { return false; }   // is empty
  
  *data = pQue->nBuffer[pQue->nPopIndex++];
  pQue->nPopIndex %= UART_DATAQUE_SIZE;

  return true;
}

bool UART1PacketStreaming( void )
{
	u8					nch, i;
	u16					nchksum;
	u8					*buffer = (u8*)&USART1StreamingBuffer;
	
  while( UARTFifoPop(&USART1RxDataQue, &nch) ) {
		// packet streaming
		for( i=0; i<(COMMPACKET_SIZE-1); i++ ) { buffer[i] = buffer[i+1]; }
		buffer[COMMPACKET_SIZE-1] = nch;

		// check packet_header & checksum
		if( USART1StreamingBuffer.nHeader == PACKET_HEADER ) {
      nchksum = USART1StreamingBuffer.wCanID
              + USART1StreamingBuffer.wData[0]
              + USART1StreamingBuffer.wData[1]
              + USART1StreamingBuffer.wData[2]
              + USART1StreamingBuffer.wData[3];
			if( nchksum == USART1StreamingBuffer.nChksum ) {
       // return PushMessage( *((LPMESSAGE)(&USART2StreamingBuffer)) );
        return true;
      }
		}
  }
  
  return false;
}

bool UART2PacketStreaming( void )
{
	u8					nch, i;
	u16					nchksum;
	u8					*buffer = (u8*)&USART2StreamingBuffer;
	
  while( UARTFifoPop(&USART2RxDataQue, &nch) ) {
		// packet streaming
		for( i=0; i<(COMMPACKET_SIZE-1); i++ ) { buffer[i] = buffer[i+1]; }
		buffer[COMMPACKET_SIZE-1] = nch;

		// check packet_header & checksum
		if( USART2StreamingBuffer.nHeader == PACKET_HEADER ) {
      nchksum = USART2StreamingBuffer.wCanID
              + USART2StreamingBuffer.wData[0]
              + USART2StreamingBuffer.wData[1]
              + USART2StreamingBuffer.wData[2]
              + USART2StreamingBuffer.wData[3];
			if( nchksum == USART2StreamingBuffer.nChksum ) {
       // return PushMessage( *((LPMESSAGE)(&USART2StreamingBuffer)) );
        return true;
      }
		}
  }
  
  return false;
}

bool UART3PacketStreaming( void )
{
	u8					nch, i;
	u16					nchksum;
	u8					*buffer = (u8*)&USART3StreamingBuffer;
	
  while( UARTFifoPop(&USART3RxDataQue, &nch) ) {
		// packet streaming
		for( i=0; i<(COMMPACKET_SIZE-1); i++ ) { buffer[i] = buffer[i+1]; }
		buffer[COMMPACKET_SIZE-1] = nch;

		// check packet_header & checksum
		if( USART3StreamingBuffer.nHeader == PACKET_HEADER ) {
      nchksum = USART3StreamingBuffer.wCanID
              + USART3StreamingBuffer.wData[0]
              + USART3StreamingBuffer.wData[1]
              + USART3StreamingBuffer.wData[2]
              + USART3StreamingBuffer.wData[3];
			if( nchksum == USART3StreamingBuffer.nChksum ) {
       // return PushMessage( *((LPMESSAGE)(&USART2StreamingBuffer)) );
        return true;
      }
		}
  }
  
  return false;
}

bool UART4PacketStreaming( void )
{
	u8					nch, i;
	u16					nchksum;
	u8					*buffer = (u8*)&USART4StreamingBuffer;
	
  while( UARTFifoPop(&USART4RxDataQue, &nch) ) {
		// packet streaming
		for( i=0; i<(COMMPACKET_SIZE-1); i++ ) { buffer[i] = buffer[i+1]; }
		buffer[COMMPACKET_SIZE-1] = nch;

		// check packet_header & checksum
		if( USART4StreamingBuffer.nHeader == PACKET_HEADER ) {
      nchksum = USART4StreamingBuffer.wCanID
              + USART4StreamingBuffer.wData[0]
              + USART4StreamingBuffer.wData[1]
              + USART4StreamingBuffer.wData[2]
              + USART4StreamingBuffer.wData[3];
			if( nchksum == USART4StreamingBuffer.nChksum ) {
       // return PushMessage( *((LPMESSAGE)(&USART2StreamingBuffer)) );
        return true;
      }
		}
  }
  
  return false;
}

void USART_SendPacket( int portnum, LPCOMMPACKET pPacket )
{
	int nwrite=0;
	int i ;
	if( pPacket != NULL ) 
	{
		pPacket->nHeader  = PACKET_HEADER;
		pPacket->nChksum  = pPacket->wCanID
						  + pPacket->wData[0]
						  + pPacket->wData[1]
						  + pPacket->wData[2]
						  + pPacket->wData[3];

		nwrite =  RS232_SendBuf( portnum, (u8*)pPacket, COMMPACKET_SIZE) ;

		if(nwrite < 0)
		{
			printf("write error\n");
		}	
	}
}
