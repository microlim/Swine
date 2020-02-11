///////////////////////////////////////////////////////////////////////////////////////////
//
//	Frame buffer make for communicaiton packet
//
//

#pragma once

#include <stddef.h>

#ifndef __CPACKET_DEFINE__
#define __CPACKET_DEFINE__

#define BYTE  	unsigned char 
#define WORD 	unsigned short 

#define MAXPACKETSIZE  2048*2
#define MINPACKETSIZE  0

class CPacket
{
public:
	//BYTE *pbuf;	
	BYTE pbuf[MAXPACKETSIZE];	
	int nAccessCnt ; 
	int allocsize ; 
	int CurReadIndex ;
public:
	CPacket(void);
	virtual ~CPacket(void);
	
	int	Getsize(void) { return nAccessCnt; } 	
	int Add( BYTE data, int nMode);
	int Add( BYTE data ) ; 
	int Add( BYTE *data, int length ) ; 
	int Insert( int index, BYTE data ) ; 
	int Insert( int index, BYTE *data , int length ) ; 
	int subtract(int lenth=1);
	int subtractTosame(BYTE data, int lenth = 1);
	BYTE GetData(int nPos = 0);
	int GetReadIndex( void ) { return CurReadIndex ; }
	int SameBYTEFindCnt(BYTE data1, BYTE data2,  int lenth = 0);
	
	int Clear( void ) ;
	
	BYTE *GetBYTE( void ) { return pbuf; } 

};

unsigned short CalcCRC(unsigned char* pDataBuffer, unsigned long usDataLen) ;
unsigned short CalcCRCbyAlgorithm(unsigned char* pDataBuffer, unsigned long usDataLen) ; 

#endif
