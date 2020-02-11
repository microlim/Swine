

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <netinet/in.h>
#include <pthread.h>
#include <netdb.h>
#include "common.h"
#include <time.h>


#include "hwctrl.h"

#define BUF_SIZE 1024*20
#define MAX_CLNT 256
#define MAXBUF 1024

#ifndef __TCPMANAGER__
#define __TCPMANAGER__

#define	DATAQUE_SIZE   1024

typedef struct _tagDATAQUE_
{
  unsigned short	nPushIndex;
  unsigned short	nPopIndex;
  unsigned char		nBuffer[DATAQUE_SIZE];
} DATAQUE, *LP_DATAQUE;


class tcpmanager
{
private :	

	struct sockaddr_in serv_adr, clnt_adr;
	int clnt_adr_sz;
	
	pthread_t t_id;	

	int current_sock ;
	
public: 
	int serv_sock, clnt_sock;
	int initok;

	char myip[64] ;

tcpmanager() ; 
	~tcpmanager() ; 
	
	int InitServer( int port ) ;
	int ClientConnect( char *ipstr, int port ) ;
	
	void CloseServer(void) ;
	void CloseClient(int sock) ;
	int getConnectionCount( void) ;
	
	int	Wait_Connect(void) ; 
	
		
} ; 

extern "C"
{
int send_msg(char *msg, int len)  ;
}



extern pthread_mutex_t tcp_mutex ;
extern pthread_mutex_t cmd_mutex ;

extern WORD TCP_CMD_HEAD ; 
extern WORD TCP_CMD_TAIL ; 
extern MESSAGE_STRUCT TCP_CMD_BUF[256] ;

extern DefTypeNodeInfo NodeInfo[MAXNODECOUNT] ;
extern int CurNodeCount ;
extern int readthreadkill;

extern CMD_DATAQUE			TCPDATAQue;    	
extern char TCP_RxQuePush( DefTypeUART2CAN data ) ;
extern char TCP_RxQuePop( DefTypeUART2CAN *data ) ;

extern PACKET_DATAQUE      TCPPACKETQue;    	
extern char TCPP_RxQuePush( DefJSONPACKET data ) ;
extern char TCPP_RxQuePop( DefJSONPACKET *data ) ;

#endif