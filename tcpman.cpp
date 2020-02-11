#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h> 
#include "tcpman.h"
#include "signal.h"
#include <unistd.h>	// read, write 
#include <sys/stat.h> // open 
#include <fcntl.h>

#include <errno.h>

void* Read_Thread(void * arg);
int send_msg(char * msg, int len);

static int clnt_cnt=0;
static int clnt_socks[MAX_CLNT];
static int clnt_type[MAX_CLNT];

pthread_mutex_t tcp_mutex ;
pthread_mutex_t cmd_mutex ;

char trx_str[128] ;


WORD TCP_CMD_HEAD = 0 ; 
WORD TCP_CMD_TAIL = 0 ; 
MESSAGE_STRUCT TCP_CMD_BUF[256] ;

CMD_DATAQUE			TCPDATAQue;    	
PACKET_DATAQUE      TCPPACKETQue;

//struct timer packet_timer;

int readthreadkill=0;


char TCP_RxQuePush( DefTypeUART2CAN data )
{
  unsigned short tmp = TCPDATAQue.nPushIndex + 1;
  tmp %= CMD_DATAQUE_SIZE;
  if( tmp == TCPDATAQue.nPopIndex ) { return 0; }  // overflow
  
  TCPDATAQue.nBuffer[TCPDATAQue.nPushIndex++] = data;
  TCPDATAQue.nPushIndex %= CMD_DATAQUE_SIZE;

  return 1;
}

char TCP_RxQuePop( DefTypeUART2CAN *data )
{
  // is empty
  if( TCPDATAQue.nPushIndex == TCPDATAQue.nPopIndex ) {
    return 0;
  }
  
  *data = TCPDATAQue.nBuffer[TCPDATAQue.nPopIndex++];
  TCPDATAQue.nPopIndex %= CMD_DATAQUE_SIZE;

  return 1;
}

char TCPP_RxQuePush( DefJSONPACKET data )
{
  unsigned short tmp = TCPPACKETQue.nPushIndex + 1;
  tmp %= CMD_DATAQUE_SIZE2;
  if( tmp == TCPPACKETQue.nPopIndex ) { return 0; }  // overflow
  
  TCPPACKETQue.nBuffer[TCPPACKETQue.nPushIndex++] = data;
  TCPPACKETQue.nPushIndex %= CMD_DATAQUE_SIZE2;

  return 1;
}

char TCPP_RxQuePop( DefJSONPACKET *data )
{
  // is empty
  if( TCPPACKETQue.nPushIndex == TCPPACKETQue.nPopIndex ) {
    return 0;
  }
  
  *data = TCPPACKETQue.nBuffer[TCPPACKETQue.nPopIndex++];
  TCPPACKETQue.nPopIndex %= CMD_DATAQUE_SIZE2;

  return 1;
}

tcpmanager::tcpmanager() 
{
	 initok=0; 
	 serv_sock=0;
	 clnt_sock=0;
	 current_sock=0;
	 pthread_mutex_init(&tcp_mutex, NULL);	 
	 pthread_mutex_init(&cmd_mutex, NULL);	 
	 TCPDATAQue.nPushIndex=TCPDATAQue.nPopIndex =0;
} 

tcpmanager::~tcpmanager() 
{
	CloseServer();
} 

void SortSockBuff(int sock)
{
	int i;
	
	

	pthread_mutex_lock(&tcp_mutex) ;	
	
	close( sock );
	for(i=0; i<clnt_cnt; i++)   // remove disconnected client
	{
		if(sock==clnt_socks[i])
		{
			
			while( i < clnt_cnt-1)
			{
				//printf("%d, %d=%d\n", i , clnt_socks[i],clnt_socks[i+1] ) ;
				clnt_socks[i]=clnt_socks[i+1];
				clnt_type[i]=clnt_type[i+1];
				i++ ;
			}
			
			break;
		}
	}
	if( clnt_cnt > 0) clnt_cnt--;

	for(i=0; i<clnt_cnt; i++)   // remove disconnected client
	{
		printf("close check sockbuf num[%d]:%d \n",i,clnt_socks[i] ) ; 
	}

	pthread_mutex_unlock(&tcp_mutex);
}	


void NodeDisconnectUpdate(int sock)
{
	int i;
	

	pthread_mutex_lock(&tcp_mutex) ;	
	for(i=0; i< CurNodeCount; i++)   // remove disconnected client
	{
		if( sock==NodeInfo[i].socketnum )
		{
			
			NodeInfo[i].isConnected =0;
			NodeInfo[i].socketnum =-1;
			
			break;
		}
	}
	
	pthread_mutex_unlock(&tcp_mutex);
}	

int tcpmanager::InitServer( int port ) 
{
	
	struct ifreq ifr;

	int option = 1;          // SO_REUSEADDR 의 옵션 값을 TRUE 로
	
	serv_sock = socket(AF_INET, SOCK_STREAM, 0);
	if( serv_sock < 0 )
	{
		printf("server socket open error !! \n") ; 
		return 0 ;
	}

	strncpy(ifr.ifr_name, "eth0", IFNAMSIZ); // 네트워트 디바이스 이름이 장비마다  다를수 있다. ifconfig 로 확인바람
    if (ioctl(serv_sock, SIOCGIFADDR, &ifr) < 0) {
        printf("Error");
    } else {
        inet_ntop(AF_INET, ifr.ifr_addr.sa_data+2,
                myip,sizeof(struct sockaddr));
        printf("Initserver IP Address is %s\n", myip);
    }


	setsockopt( serv_sock, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option) );
	
	memset(&serv_adr, 0, sizeof(serv_adr));
	
	serv_adr.sin_family=AF_INET; 
	serv_adr.sin_addr.s_addr=htonl(INADDR_ANY);
	serv_adr.sin_port=htons(port);
	
	if( bind(serv_sock, (struct sockaddr*) &serv_adr, sizeof(serv_adr))==-1 )
	{
		printf("bind error !! \n") ; 
		close(serv_sock); 
		return 0 ;
	}

	if(listen(serv_sock, 5)==-1)
	{
		printf("listen() error \n") ; return 0 ; 
	}
	initok=1;

	return 1; 	
	
}

int tcpmanager::ClientConnect( char *ipstr, int port ) 
{
	

	clnt_sock=socket(AF_INET, SOCK_STREAM, 0); // PF_INER => AF_INET

	memset(&clnt_adr, 0, sizeof(clnt_adr));

	clnt_adr.sin_addr.s_addr=inet_addr(ipstr);
	clnt_adr.sin_family=AF_INET; 
	clnt_adr.sin_port=htons(port);
	

	// Wifi 환경에서 시그날이 약할경우 자주 timeout 에 걸린다.
	struct timeval tv ;
	tv.tv_sec=15 ;
	tv.tv_usec=0;
	setsockopt( clnt_sock, SOL_SOCKET, SO_RCVTIMEO, (char *) &tv, sizeof(tv));	
    
	//Connect to remote server
	if (connect(clnt_sock , (struct sockaddr *)&clnt_adr , sizeof(clnt_adr)) < 0)
	{
	  printf("client connect failed. portnum[%d]\n",port);
	  return 0;
	}
	 
	printf("client Connected %s\n",ipstr) ;
	initok=2 ;

	if(  clnt_sock != -1 )
	{
		pthread_mutex_lock(&tcp_mutex);
		clnt_socks[clnt_cnt]=clnt_sock;
		clnt_type[clnt_cnt]=2;
		clnt_cnt++;
		pthread_create(&t_id, NULL, Read_Thread, (void*)&clnt_sock);
		//SetThreadPriority( t_id, THREAD_PRIORITY_HIGHEST ) ; 
		pthread_detach(t_id);
		readthreadkill=1;
		

		printf("socketID %d Connected client IP: %s \n",clnt_sock, inet_ntoa(clnt_adr.sin_addr));

		for(int i=0; i<clnt_cnt; i++)   // remove disconnected client
		{
			printf("sockbuf num[%d]:%d \n",i,clnt_socks[i] ) ; 
		}		
		pthread_mutex_unlock(&tcp_mutex);


	} 	

	return 1 ; 	
	
}

void tcpmanager::CloseClient(int sock) 
{
	  readthreadkill=0;
	  usleep(1000) ;
	  printf("CloseClient() Disconnect socketID %d\n", sock ) ;
	  //SortSockBuff( sock ) ;
	  //NodeDisconnectUpdate( sock ) ;
	  
}

void tcpmanager::CloseServer(void)
{
	if( initok ) close(serv_sock);
}

int tcpmanager::getConnectionCount( void) 
{
	return clnt_cnt ;
}

int	tcpmanager::Wait_Connect(void)  
{
	clnt_adr_sz=sizeof(clnt_adr);
	printf("tcpmanager Wait conntection...\n") ;
	clnt_sock=accept(serv_sock, (struct sockaddr*)&clnt_adr, (socklen_t*)&clnt_adr_sz);
	if(  clnt_sock != -1 )
	{
		pthread_mutex_lock(&tcp_mutex);
		clnt_socks[clnt_cnt]=clnt_sock;
		clnt_type[clnt_cnt]=1;
		if ( strcmp(myip,inet_ntoa(clnt_adr.sin_addr) )==0 )
		{
			clnt_type[clnt_cnt]=5;
			printf("web api trans~~ \n" ) ;
		}
		clnt_cnt++;
		pthread_create(&t_id, NULL, Read_Thread, (void*)&clnt_sock);
		//SetThreadPriority( t_id, THREAD_PRIORITY_HIGHEST ) ; 
		pthread_detach(t_id);
		usleep(1000) ;
		printf("tcpmanager socketID %d Connected client IP: %s \n", clnt_sock, inet_ntoa(clnt_adr.sin_addr));
		
		

		for(int i=0; i<clnt_cnt; i++)   // remove disconnected client
		{
			printf("lived sockbuf num[%d]:%d type %d\n",i,clnt_socks[i],clnt_type[i] ) ; 
		}		
		pthread_mutex_unlock(&tcp_mutex);

/*
		g_GPIO->output( PTT_OUT,0 ) ;

		pthread_mutex_lock(&mutex_cmd) ;

		memset(trx_str,0x00,128 ) ;
		sprintf(trx_str,"$CMRC,CONSOCK,%d,%s*\n",clnt_sock,inet_ntoa(clnt_adr.sin_addr)) ;	

		CMD_BUF[CMD_TAIL].rx_index = clnt_sock ;
		CMD_BUF[CMD_TAIL].size = strlen(trx_str) ;
		memset( CMD_BUF[CMD_TAIL].buf , 0x00 , 128 ) ; 
		memcpy( CMD_BUF[CMD_TAIL].buf , trx_str, CMD_BUF[CMD_TAIL].size ) ; 	
		if( CMD_TAIL < 255 ) CMD_TAIL++ ; 
		pthread_mutex_unlock(&mutex_cmd);
*/
	} 
	return 1 ;
}


void *Read_Thread(void * arg)
{
	int clnt_sock=*((int*)arg);
	int str_len=0;
	char msg[BUF_SIZE] ;
	
	DefJSONPACKET TCPStreamingBuffer ;   
	
	int		i,j,dicnt,getsocktype;
	int     ocnt,ccnt ;
	unsigned short 		nchksum;
    unsigned int	    pSize = sizeof( DefJSONPACKET) ; 
	//unsigned char		*buffer = (unsigned char *)TCPStreamingBuffer.nData;
	
    getsocktype=0;
    

	for( i =0  ; i < clnt_cnt ;i ++)
	{
		if(clnt_socks[i] == clnt_sock )   getsocktype=clnt_type[i] ;
	}

	memset( &TCPStreamingBuffer , 0x00 , pSize ) ; 
	i=0;
	ocnt=ccnt=0;
	dicnt=0;
	
	while( (str_len=read(clnt_sock, msg, sizeof(msg)) ) > 0 )
	{	
		
		//for( i=0; i< str_len;i++) printf( "rr data:%c\n" ,msg[i] ) ;
		pthread_mutex_lock(&cmd_mutex);
		//send_msg(msg, str_len);
		
		j=0;
		
		while( j < str_len   ) // received packet check
		{
			// packet streaming
			
			if( msg[j] != 0x09 && msg[j] != 0x0d && msg[j] != 0x0a )
			{	
				TCPStreamingBuffer.nData[dicnt] = msg[j];
			
			
			if( TCPStreamingBuffer.nData[dicnt] == '{' ) ocnt++;
			if( TCPStreamingBuffer.nData[dicnt] == '}' ) ccnt++;
			//printf( "data:%c [%d][%d]\n" ,buffer[dicnt],ocnt,ccnt ) ; 
			if( dicnt< (BUF_SIZE-2) )  dicnt++;

			
			//if( TCPStreamingBuffer.nData[0] == '{' ) 
			//{
				// Add command buffer //////////////////////////
				if( ocnt == ccnt && ocnt > 0 && ccnt > 0  )
				{
					if( TCPStreamingBuffer.nData[0] == '{' )
					{
						TCPStreamingBuffer.type = getsocktype;
						TCPP_RxQuePush(TCPStreamingBuffer) ;
						printf( "push %c,%s\n" ,TCPStreamingBuffer.nData[0] ,TCPStreamingBuffer.nData ) ; 
		
						ocnt=ccnt=dicnt=0;
						memset(TCPStreamingBuffer.nData , 0x00 , BUF_SIZE ) ; 
						break;
					} else {

						printf( "not push %c,%s\n" ,TCPStreamingBuffer.nData[0] ,TCPStreamingBuffer.nData ) ; 
						break;
					}
				}
				////////////////////////////////////////////////

				if( ocnt < ccnt )
				{
					//TCPP_RxQuePush(TCPStreamingBuffer) ;
					printf( "packet error\n") ; 
					printf( "error data %c,%s\n" ,TCPStreamingBuffer.nData[0] ,TCPStreamingBuffer.nData ) ; 
					ocnt=ccnt=dicnt=0;
					memset(TCPStreamingBuffer.nData , 0x00 , BUF_SIZE ) ; 
					break;
				}
			//}
			} 
			j++;
		}
		
//{"test":"1","test2","2"}
		pthread_mutex_unlock(&cmd_mutex);
	
			
	}
	
	SortSockBuff( clnt_sock ) ;
	//NodeDisconnectUpdate( clnt_sock ) ;
	
	if( readthreadkill == 1)
	{
		printf( "TCP read return code :%d\n" ,str_len ) ;  
		printf("Read Thread Disconnect socketID %d because server losted \n", clnt_sock ) ;
	} else {
		printf( "TCP read thread killed \n" ) ;  

	}

	readthreadkill=1;
	return NULL;
}
/*
070 4481 1977
void *Read_Thread(void * arg)
{
	int clnt_sock=*((int*)arg);
	int str_len=0;
	int read_len=0;
	char buf[MAXBUF] ;
	int des_fd;
	int file_read_len ;
	int		i,j;
	unsigned short 		nchksum;

	while(1) 
	{ 
		char file_name[MAXBUF]; 
		memset(buf, 0x00, MAXBUF); 
		//client_sockfd = accept(server_sockfd, (struct sockaddr *)&clientaddr, &client_len); 
		//printf("New Client Connect : %s\n", inet_ntoa(clientaddr.sin_addr)); 
		
		read_len = read(clnt_sock, buf, MAXBUF); 
		if(read_len > 0) 
		{ 
			strcpy(file_name, buf); 
			printf("create file %s\n",  file_name); 
		} else { 
			//close(clnt_sock); 
			break; 
		} 
	
	FILE_OPEN: 
		des_fd = open(file_name, O_WRONLY | O_CREAT | O_EXCL, 0700);
		if(!des_fd) { perror("file open error : "); break; }
		
		if(errno == EEXIST) { 
			close(des_fd); 
			size_t len = strlen(file_name); 
			file_name[len++] = '_'; 
			file_name[len++] = 'n'; 
			file_name[len] = '\0'; 
			goto FILE_OPEN; 
		
		} 
	
		while(1)
		{ 
			memset(buf, 0x00, MAXBUF); 
			file_read_len = read(clnt_sock, buf, MAXBUF); 
			printf("%d received data %02X %02X\n",file_read_len,buf[0],buf[1]); 
			if( file_read_len <= 0 ) 
			{ 
				printf("NULL finish received file\n"); 
				break; 
			}
			if( file_read_len ==2 && buf[0]==0x55 && buf[1]==0x55 ) 
			{ 
				printf("ENDCODE finish received file\n"); 
				break; 
			}
			write(des_fd, buf, file_read_len); 
		} 
		//close(clnt_sock); 
		close(des_fd); 
	}


	
	SortSockBuff( clnt_sock ) ;

	printf( "TCP file record thread killed \n" ) ;  


	readthreadkill=1;
	return NULL;
}
*/
int send_msg(char *msg, int len)   // send to all
{
	int i,j;
	int clnt_sock ;
//	pthread_mutex_lock(&cmd_mutex);
//	printf(" tcp send  start\n") ;
	for(i=0; i<clnt_cnt; i++)
	{
		
		if( write(clnt_socks[i], msg, len) != len )
		{

			printf("TCP send Error %d\n",len) ; 
/*			
			clnt_sock = clnt_socks[i] ;
			printf(" Disconnect socketID %d\n", clnt_sock ) ;
			printf(" Disconnect socketID %d\n", clnt_sock ) ;
			printf(" Disconnect socketID %d\n", clnt_sock ) ;
			
			for(j=0; j<clnt_cnt; j++)   // remove disconnected client
			{
				if(clnt_sock==clnt_socks[j])
				{
					while(j <clnt_cnt-1) 
					{
						clnt_socks[j]=clnt_socks[j+1];
						j++ ;
					}
					break ;
				}
			}
	
			if( clnt_cnt > 0) clnt_cnt--;
			close(clnt_sock);
			break;
*/
		}
		//printf("%s",msg) ; 
		//usleep(1000*10) ;
	}
	//printf(" tcp send  end\n") ;
//	pthread_mutex_unlock(&cmd_mutex);
	return clnt_cnt ;
}
