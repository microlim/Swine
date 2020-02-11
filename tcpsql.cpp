/*
	환기제어기 콘트롤 메인프로그램
	last updated   2017/8/12   
	TCP,
	MYSQL,
	Packet access
*/


#include "tcpsql.h"
#include <errno.h>

#define TESTPACKETSEND 0

#include <curl/curl.h>
#include "json-c/json.h"
#include <modbus/modbus-rtu.h>

#include <vector>
using namespace std;

#ifdef _WIN32
#undef snprintf
#define snprintf _snprintf
#endif

#define POLYNORMIAL 0xA001

struct url_data {
    size_t size;
    char* data;
};

char CMD_DB[] = "127.0.0.1;tcprelay;ictuser;1234;" ;
//char CMD_DB[] = "127.0.0.1;ict_local;root;cnrtksdusrnth;" ;
char DEV_CMD_LIST[100][16] ;
char DEV_CMD_LIST_VALUE[100][30] ;
int  DEV_CMD_COUNT=0;

char MY_FARMNO[16] ;
// 주기적인 패킷 전송을 위한 커맨드
unsigned char commandlist[3]={0x10,0x81,0x83} ;
char comindex=0;


int testapicnt=0;
////////// 전역 변수 선언 ///////////////////////////////
char TCP_connected=0;
DefTypeNodeInfo NodeInfo[MAXNODECOUNT] ;
int CurNodeCount=0;  // 
int FirstTryConnect=1;
char livepacket_sendflag=0;

vector<DefCycleDevice> ICTDevInfo ;



//DefTypeUART2CAN TCPStreamingBuffer ;
//////// 쓰레드 관련 변수 /////////////////////////////
pthread_t t_idTCP,t_idTimer,t_idRS232;		

pthread_mutex_t mutex_cmd ;
pthread_mutex_t mutex_rsbuf1 ;
/////////  TCP/IP manager //////////////////////////////
tcpmanager tcpman ;


struct timer keeplive_timer,keeplivewait_timer,tryconnect_timer,fileget_timer;

char GLB_postbuf[1024*10];
char ICTretval[64] ; 

int ICTDEVICE_COUNT=1;
size_t dataSize=0;

modbus_t *ctx;
char *p485name = "/dev/ttyUSB0" ;


int sqlpf( char *sqlcmd,char *fmt,...) ;
void MakeJPacketICTdevice( DefCycleDevice addDev ) ;
void MakeJPacketICTdeviceAll(void) ;



void delchar(char *str, char ch)
{
    for (; *str != '\0'; str++)//종료 문자를 만날 때까지 반복
    {
        if (*str == ch)//ch와 같은 문자일 때
        {
            strcpy(str, str + 1);
            str--;
        }
    }
}

char modbus_write(int ch, int addr,int dataval)
{
	// libmodbus rtu 컨텍스트 생성
	ctx = modbus_new_rtu(p485name, 9600, 'N', 8, 1);
	/*
	 * 디바이스 : /dev/ttyXXX
	 * 전송속도 : 9600, 19200, 57600, 115200
	 * 패리티모드 : N (none) / E (even) / O (odd)
	 * 데이터 비트 : 5,6,7 or 8
	 * 스톱 비트 : 1, 2

	*/
	if (ctx == NULL) {
			fprintf(stderr, "Unable to create the libmodbus context\n");
			return -1;
	}

	modbus_set_slave(ctx, ch); // 슬레이브 주소
	modbus_set_debug(ctx,FALSE); // 디버깅 활성

	// 연결
	if (modbus_connect(ctx) == -1) {
			fprintf(stderr, "Unable to connect %s\n", modbus_strerror(errno));
			modbus_free(ctx);
			return -1;
	}
	
	modbus_write_register(ctx, addr, dataval); 

	usleep(20000) ;
	
	modbus_close(ctx);
	modbus_free(ctx);
}

int modbus_writeread( int ch,int addr,int dataval,int dlen)
{
	// libmodbus rtu 컨텍스트 생성
	ctx = modbus_new_rtu(p485name, 9600, 'N', 8, 1);
	/*
	 * 디바이스 : /dev/ttyXXX
	 * 전송속도 : 9600, 19200, 57600, 115200
	 * 패리티모드 : N (none) / E (even) / O (odd)
	 * 데이터 비트 : 5,6,7 or 8
	 * 스톱 비트 : 1, 2

	*/
	if (ctx == NULL) {
			fprintf(stderr, "Unable to create the libmodbus context\n");
			return -1;
	}

	modbus_set_slave(ctx, ch); // 슬레이브 주소
	modbus_set_debug(ctx,FALSE); // 디버깅 활성

	// 연결
	if (modbus_connect(ctx) == -1) {
			fprintf(stderr, "Unable to connect %s\n", modbus_strerror(errno));
			modbus_free(ctx);
			return -1;
	}


	modbus_write_register(ctx, addr, dataval); 
	usleep(20000) ;	
	
	uint16_t *read_registers;
	read_registers = (uint16_t *) malloc(256 * sizeof(uint16_t));

	if( modbus_read_registers(ctx, addr, dlen, read_registers) != -1 )
	{
	  	printf("mbus read  %d \n",  read_registers[0]); 

	} else {
		printf("read error \n");
		read_registers[0]=00;

	}

	usleep(20000) ;
	
	modbus_close(ctx);
	modbus_free(ctx);

	if( dlen == 1) return read_registers[0];

	return 0;
}

int modbus_read( int ch,int addr,int dlen)
{
	// libmodbus rtu 컨텍스트 생성
	ctx = modbus_new_rtu(p485name, 9600, 'N', 8, 1);
	/*
	 * 디바이스 : /dev/ttyXXX
	 * 전송속도 : 9600, 19200, 57600, 115200
	 * 패리티모드 : N (none) / E (even) / O (odd)
	 * 데이터 비트 : 5,6,7 or 8
	 * 스톱 비트 : 1, 2

	*/
	if (ctx == NULL) {
			fprintf(stderr, "Unable to create the libmodbus context\n");
			return -1;
	}

	modbus_set_slave(ctx, ch); // 슬레이브 주소
	modbus_set_debug(ctx,FALSE); // 디버깅 활성

	// 연결
	if (modbus_connect(ctx) == -1) {
			fprintf(stderr, "Unable to connect %s\n", modbus_strerror(errno));
			modbus_free(ctx);
			return -1;
	}
	
	uint16_t *read_registers;
	read_registers = (uint16_t *) malloc(256 * sizeof(uint16_t));

	if( modbus_read_registers(ctx, addr, dlen, read_registers) != -1 )
	{
	  	printf("mbus read  %d \n",  read_registers[0]); 

	} else {
		printf("read error \n");
		read_registers[0]=00;

	}

	usleep(20000) ;
	
	modbus_close(ctx);
	modbus_free(ctx);

	if( dlen == 1) return read_registers[0];

	return 0;
}

void cycletimer()
{
	int c;
	struct tm tm = *localtime(&(time_t){time(NULL)});

	printf("cycletimer() %04d%02d%02d_%02d:%02d:%02d \n",tm.tm_year+1900,tm.tm_mon+1,tm.tm_mday,tm.tm_hour,tm.tm_min,tm.tm_sec ) ;
	c= ICTDEVICE_COUNT ;

/*	
	for( c = 0 ;c < ICTDEVICE_COUNT;c++)
	{
		DefCycleDevice &getDev = ICTDevInfo.at(c) ;
		

		if( getDev.curcycle > 0 ) 
		{
			getDev.curcycle--;
			//printf("Cycle DEV INFO: %s,%d\n",getDev.devId,getDev.curcycle ) ;
			if( getDev.curcycle <=0 ) 
			{
				printf("Cycle DEV INFO: %s,%d\n",getDev.devId,getDev.curcycle ) ;
				getDev.curcycle = getDev.setcycle ;
				MakeJPacketICTdevice(getDev) ; // cycled trans mobile server
				// packet request add   --------------------------


				//------------------------------------------------

			}
		}

	}

*/

	MakeJPacketICTdeviceAll() ;
	//printf("debug cycletimer() \n");
	if( ICTDEVICE_COUNT < ICTDevInfo.size() ) { ICTDEVICE_COUNT++; } //sleep();
	//if( ICTDEVICE_COUNT >= ICTDevInfo.size() ) ICTDEVICE_COUNT=0;
	//----------------------------------------------------------------
	// 정전화재 WEB API 데이터 취득 


	//----------------------------------------------------------------
}
 
int createTimer( timer_t *timerID, int sec, int msec )  
{  
    struct sigevent         te;  
    struct itimerspec       its;  
    struct sigaction        sa;  
    int                     sigNo = SIGRTMIN;  
   
    /* Set up signal handler. */  
    sa.sa_flags = SA_SIGINFO;  
    sa.sa_sigaction = cycletimer;     // 타이머 호출시 호출할 함수 
    sigemptyset(&sa.sa_mask);  
  
    if (sigaction(sigNo, &sa, NULL) == -1)  
    {  
        printf("sigaction error\n");
        return -1;  
    }  
   
    /* Set and enable alarm */  
    te.sigev_notify = SIGEV_SIGNAL;  
    te.sigev_signo = sigNo;  
    te.sigev_value.sival_ptr = timerID;  
    timer_create(CLOCK_REALTIME, &te, timerID);  
   
    its.it_interval.tv_sec = sec;
    its.it_interval.tv_nsec = msec * 1000000;  
    its.it_value.tv_sec = sec;
    
    its.it_value.tv_nsec = msec * 1000000;
    timer_settime(*timerID, 0, &its, NULL);  
   
    return 0;  
}


size_t curlWriteFunction(void* ptr, size_t size/*always==1*/,
                         size_t nmemb, void* userdata)
{
    char** stringToWrite=(char**)userdata;
    const char* input=(const char*)ptr;
    if(nmemb==0) return 0;
    if(!*stringToWrite)
        *stringToWrite=malloc(nmemb+1);
    else
        *stringToWrite=realloc(*stringToWrite, dataSize+nmemb+1);
    memcpy(*stringToWrite+dataSize, input, nmemb);
    dataSize+=nmemb;
    (*stringToWrite)[dataSize]='\0';
    return nmemb;
}

size_t write_data(void *ptr, size_t size, size_t nmemb, struct url_data *data) {
    size_t index = data->size;
    size_t n = (size * nmemb);
    char* tmp;

    data->size += (size * nmemb);

#ifdef DEBUG
    fprintf(stderr, "data at %p size=%ld nmemb=%ld\n", ptr, size, nmemb);
#endif
    tmp = realloc(data->data, data->size + 1); /* +1 for '\0' */

    if(tmp) {
        data->data = tmp;
    } else {
        if(data->data) {
            free(data->data);
        }
        fprintf(stderr, "Failed to allocate memory.\n");
        return 0;
    }

    memcpy((data->data + index), ptr, n);
    data->data[data->size] = '\0';

    return size * nmemb;
}

/* Post JSON data to a server.
name and value must be UTF-8 strings.
Returns TRUE on success, FALSE on failure.
*/
int PostJSON(char *postURL,char *json)
{
	int retcode = FALSE;


	char *getjsondata=NULL;
	CURL *curl = NULL;
	CURLcode res = CURLE_FAILED_INIT;
	char errbuf[CURL_ERROR_SIZE] = { 0, };
	struct curl_slist *headers = NULL;
	char agent[1024] = { 0, };


	//if(curl_global_init(CURL_GLOBAL_ALL)) {
	//    fprintf(stderr, "Fatal: The initialization of libcurl has failed.\n");
	//    return EXIT_FAILURE;
	//  }
	struct url_data data;
	data.size = 0;
	data.data = malloc(4096); /* reasonable size initial buffer */
	if(NULL == data.data) {
		fprintf(stderr, "Failed to allocate memory.\n");
		return NULL;
	}

	data.data[0] = '\0';
	curl_global_init(CURL_GLOBAL_ALL) ;
	curl = curl_easy_init();
	if(!curl) {
		fprintf(stderr, "Error: curl_easy_init failed.\n");
		goto cleanup;
	}

	curl_easy_setopt(curl, CURLOPT_CAINFO, "curl-ca-bundle.crt");

	snprintf(agent, sizeof agent, "libcurl/%s",
	curl_version_info(CURLVERSION_NOW)->version);
	agent[sizeof agent - 1] = 0;
	curl_easy_setopt(curl, CURLOPT_USERAGENT, agent);

	headers = curl_slist_append(headers, "Expect:");
	headers = curl_slist_append(headers, "Content-Type: application/json");
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json);
	curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, -1L);

	printf("\npost to mobile server json : %s \n",json) ;
	curl_easy_setopt(curl, CURLOPT_URL, postURL);//"http://iot.pigplanxe.co.kr/iotRcvApi.json"

	curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
	curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errbuf);
	
	//curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &curlWriteFunction);
	//curl_easy_setopt(curl, CURLOPT_WRITEDATA, &getjsondata);

	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &data);

	res = curl_easy_perform(curl);

	printf("\nmobile server response json %d size(%d):%s\n",testapicnt,data.size, data.data);

	testapicnt++;
	memset(GLB_postbuf,0x00,1024*10 ) ; 
	if( data.size < 1024*10 )
	{
	memcpy( GLB_postbuf,data.data,data.size );  	

	// accept packet 확인 -----------------------------------------------------

	// -----------------------------------------------------------------------

	retcode=TRUE ;
	} else printf("oversize recv getjsondata:%d\n", data.size);



	if(res != CURLE_OK) {
	size_t len = strlen(errbuf);
	printf("\nlibcurl: (%d) ", res);
	if(len) printf( "%s%s", errbuf, ((errbuf[len - 1] != '\n') ? "\n" : ""));
	printf("%s\n\n", curl_easy_strerror(res));
	goto cleanup;
	}

	retcode = TRUE;

cleanup:

	if( data.data != NULL ) free(data.data);
	curl_slist_free_all(headers);
	curl_easy_cleanup(curl);

	//if(atexit(curl_global_cleanup)) {
	//  fprintf(stderr, "Fatal: atexit failed to register curl_global_cleanup.\n");
	curl_global_cleanup();
	//  return EXIT_FAILURE;
	//}	


	return retcode;
}


int sqlpf( char *sqlcmd,char *fmt,...)
{
	va_list argptr;
	int cnt ;
	va_start(argptr,fmt) ; 
	cnt = vsprintf( sqlcmd,fmt,argptr);
	va_end(argptr) ;
	return(cnt) ; 
}

void *TCPSERVER_Thread(void * arg)
{
	//int clnt_sock=*((int*)arg);
	while(1)
	{
		if( tcpman.initok==2 ) break ;

		tcpman.Wait_Connect() ; 
		usleep(1000*10) ; 
	}
	return NULL;
}

void *RS232_Thread(void * arg)
{
	int n = 0 ;
	int cnt ;
	unsigned char readbuf[255];
	memset( readbuf,0x00,255 ) ;
		
	while(1)
	{
			n = RS232_PollComport(CONTROL_PORT, readbuf, 254);
			if(n > 0)
			{
				pthread_mutex_lock(&mutex_rsbuf1);
				for( cnt = 0 ; cnt < n ;cnt++) 
				{
					USART1_RxQuePush( readbuf[cnt] ) ; 
				}
				memset( readbuf,0x00,255) ;
				pthread_mutex_unlock(&mutex_rsbuf1);
			}
			

		  usleep(50000) ;
	}

	return NULL ;
}


void GetSendPacket( pDefTypeUART2CAN pbuff , unsigned char tx_id , unsigned char rx_id , unsigned char *data )
{
	int i ;

	pbuff->nLength = 1 ; // nLength 포함 길이 1 부터 시작
	pbuff->nCheckSum = 0 ;

	pbuff->Start = UART_START ;
	pbuff->iStart = UART_ISTART ;

	pbuff->nTxCanID = tx_id ;     // LSB's 8bit ID, tartget ID    
	pbuff->nLength ++ ;
	pbuff->nCheckSum += pbuff->nTxCanID ;

	pbuff->nRxCanID = rx_id ;     // LSB's 8bit ID, tartget ID    
	pbuff->nLength ++ ;
	pbuff->nCheckSum += pbuff->nRxCanID ;

	for ( i = 0 ; i < sizeof(pbuff->nData) ; i++ ) {
		pbuff->nData[i] = data[i] ;     // LSB first transfer
		pbuff->nLength ++ ;
		pbuff->nCheckSum += pbuff->nData[i] ;
	}
	pbuff->nLength ++ ;
	pbuff->nCheckSum += pbuff->nLength ;
}


int receivePacket( unsigned char *data , int length , pDefTypeUART2CAN *pRcvPacket)
{
	unsigned char nLength = 0 ;
	unsigned char nCheckSum = 0 ;
	unsigned int i ,j ;
	pDefTypeUART2CAN pbuff ;

	*pRcvPacket = NULL ;
	for ( i = 0 ; i <= length-sizeof(DefTypeUART2CAN) ; i++ ) {
		pbuff = (pDefTypeUART2CAN)(data+i) ;
		if ( pbuff->Start == UART_START && pbuff->iStart == UART_ISTART ) {
			if ( pbuff->nLength == (unsigned)( sizeof(DefTypeUART2CAN)-sizeof(pbuff->Start)-sizeof(pbuff->iStart) ) ) { // error
				nCheckSum = 0 ;
				nCheckSum += pbuff->nTxCanID ;
				nCheckSum += pbuff->nRxCanID ;
				for ( j = 0 ; j < sizeof(pbuff->nData) ; j++ ) {
					nCheckSum += pbuff->nData[j] ;
				}
				nCheckSum += pbuff->nLength ;
				if ( nCheckSum == pbuff->nCheckSum ) {
					*pRcvPacket = pbuff ;
					return i + sizeof(DefTypeUART2CAN) ;
				}
			}
		}
	}
	return i ;
}


void daemonize ()
{
    int x;
    pid_t pid;

    /* Fork off the parent process */
    pid = fork();
    if (pid < 0)
        exit(EXIT_FAILURE);
    if (pid > 0)
        exit(EXIT_SUCCESS);

    /* On success: The child process becomes session leader */
    if (setsid() < 0)
        exit(EXIT_FAILURE);

    signal(SIGCHLD, SIG_IGN);
    signal(SIGHUP, SIG_IGN);

    /* Fork off for the second time*/
    pid = fork();
    if (pid < 0)
        exit(EXIT_FAILURE);
    if (pid > 0)
        exit(EXIT_SUCCESS);

    umask(0);
    //chdir(GOS_WORKING_DIRECTORY);

    /* Close all open file descriptors */
    for (x = sysconf(_SC_OPEN_MAX); x > 0; x--) {
    //    close (x);
    }
}






void ReadNodeInformationFromDB( void )
{
	cf_db_t db;
	
	DefTypeUART2CAN buff ;	
	
	unsigned char data[8] ;

	char TESTDB[] = "127.0.0.1;shopinfo;root;0419;" ;	
	
	int rows, columns, i, j;   
	char *errmsg; 
	char **results;
	

	
	ASSERT_OK (cf_init_db (&db, TESTDB), "fail to init");
	ASSERT_OK (cf_db_open (&db), "fail to open");

	errmsg = NULL ;
	ASSERT (CF_DB_OK == cf_db_get_table (&db, "select * from shopuser", &results, &rows, &columns, &errmsg), errmsg);
		
	if( rows > 0 )
	{
		for ( i = 1; i <= rows; i++) {
			/*
			//for ( j = 0; j < columns-1; j++) {
			//	printf( "%d,%d = %s \n",j,i,results[i*columns+j]);
			NodeInfo[i-1].code = atoi(results[i*columns+0] ) ; 
			strcpy(NodeInfo[i-1].name, results[i*columns+1] ) ;
			strcpy(NodeInfo[i-1].ip, results[i*columns+2] ) ;
			NodeInfo[i-1].canid = atoi(results[i*columns+3] ) ; 
			NodeInfo[i-1].port = atoi(results[i*columns+4] ) ; 
			NodeInfo[i-1].status = atoi(results[i*columns+5] ) ; 
			NodeInfo[i-1].isConnected=0;
			//}
			printf("%d,%s,%s,%d,%d,%d,[%d]\n",NodeInfo[i-1].code,NodeInfo[i-1].name,NodeInfo[i-1].ip,
				NodeInfo[i-1].canid,NodeInfo[i-1].port,NodeInfo[i-1].status,0) ;
				//NodeInfo[i-1].canid,NodeInfo[i-1].port,NodeInfo[i-1].status) ;
			*/
			printf("%s,%s,%s\n",results[i*columns+0],results[i*columns+1],results[i*columns+2] ) ;
			
			printf("........................................................\n") ;
			
			
			CurNodeCount++;
		}
	}
	if ( errmsg ) {
		free( errmsg ) ;
	}
	
	cf_db_free_table (results);	
	// 전송된 DB data 처리 


	ASSERT_OK (cf_db_close (&db), "fail to close");
	cf_release_db (&db);
}



/////////////////////////////////////////////////////////////////////////////////////////////
//
// 디비에 등록된 노드제어기 IP 에 접속 시도
/*
void TryNodeConnect(void)
{
	int i=0;
	if(timer_expired(&tryconnect_timer) || FirstTryConnect )
	{
		int send_system_reboot_cmd = 0 ;
		for( i= 0 ; i < CurNodeCount ;i++)
		{
			if( NodeInfo[i].status == 1 && NodeInfo[i].isConnected == 0)
			{
				char date_now[20] ; // YYYY-MM-DD HH:MM:ss
				cf_db_timestring( date_now , sizeof(date_now) , (int)time(NULL) ) ;

				printf("........................................................\n") ;
				printf("%s Try connect Socket[%s]  \n",date_now,NodeInfo[i].ip ) ;
				NodeInfo[i].isConnected = tcpman.ClientConnect( NodeInfo[i].ip,NodeInfo[i].port ) ;
				if(  NodeInfo[i].isConnected )  
				{
					NodeInfo[i].socketnum=tcpman.clnt_sock ;
					timer_set(&keeplive_timer, CLOCK_SECOND * 10);
					
				}
				else {
					char reboot_cmd[80] ;
					sprintf( reboot_cmd , "wget -q --tries=1 http://%s/etc.htm?tmenu=reboot",NodeInfo[i].ip) ;
					//send_system_reboot_cmd = system( reboot_cmd ) ;
					printf("%s system call [%d]\n%s\n==\n",date_now,send_system_reboot_cmd,reboot_cmd ) ;
				}
				//usleep(1000*5000) ;


			}
		}
		timer_reset(&tryconnect_timer);
		FirstTryConnect=0;
	}

}
*/
int NodeConnectedCheck(void)
{
	int i=0;
	for( i= 0 ; i < CurNodeCount ;i++)
	{
		if(  NodeInfo[i].isConnected && NodeInfo[i].status )  return 1; 
	}
	return 0;
}

void NodeConnectionClose(void)
{
	int i=0;
	printf("Keepalive received packet timeout .....\n") ;
	for( i= 0 ; i < CurNodeCount ;i++)
	{
		if( NodeInfo[i].status == 1 && NodeInfo[i].isConnected == 1 ) //서버쪽에서 끊어지면 아래메세지는 출력않됨
		{		    
			printf("Close Socket[%d]   Keepalive fail.....\n",NodeInfo[i].socketnum ) ;
			tcpman.CloseClient( NodeInfo[i].socketnum ) ;
			NodeInfo[i].isConnected = 0 ;
			
		}
	}

}


/*
void checkCmdTable( void )
{
	cf_db_t db;
	
	DefTypeUART2CAN buff ;	
	DefTypeUART2CAN KeepAlivebuff ;	

	unsigned char data[8] ;
	char SQLCMD[256] ;
		
	//char TEST_DB[] = "127.0.0.1;TESTNODE;root;0419;" ;
	char TESTDB[] = "127.0.0.1;ict_local;root;cnrtksdusrnth;" ;
	int rows, columns, i, j;   
	char *errmsg; 
	char **results;
	
	ASSERT_OK (cf_init_db (&db, TESTDB), "fail to init");
	ASSERT_OK (cf_db_open (&db), "fail to open");

	errmsg = NULL ;
	ASSERT (CF_DB_OK == cf_db_get_table (&db, "select * from ifs_control_input", &results, &rows, &columns, &errmsg), errmsg);
		
	if( rows > 0 )
	{
		for ( i = 1; i <= rows; i++) {
			
			for ( j = 0; j < columns-1; j++) {
				//printf( "%d,%d = %s \n",j,i,results[i*columns+j]);
				data[j]=atoi( results[i*columns+j] ) ;

			}
			GetSendPacket( &buff , 0xAA , data[7] , data ) ; //data[7] canid
			printf("send TCP Packet from DB.................................\n") ;
			send_msg( (char *)&buff,sizeof(DefTypeUART2CAN) ) ;	
			HexaPrint(  (unsigned char *)&buff,sizeof(DefTypeUART2CAN) ) ;
			printf("........................................................\n") ;

		}
	} else {

		
		if(timer_expired(&keeplive_timer))
		{			
        		time_t cv_time = time(NULL) ; 
        		struct tm *ptm = localtime(&cv_time);

			if( comindex == 0 ) printf("Keep alive TCP Packet send.................................\n") ;
			if( comindex == 1 ) printf("Sensor info Request TCP Packet send........................\n") ;
			if( comindex == 2 ) printf("Controller output info Request TCP Packet send.............\n") ;

			data[0] = commandlist[comindex] ;
			data[1] = ptm->tm_year-100 ; data[2] = ptm->tm_mon+1 ; data[3] = ptm->tm_mday ; 
			data[4] = ptm->tm_hour ; data[5] = ptm->tm_min ; data[6] = ptm->tm_sec ;
			data[7] = 0 ;

#if TESTPACKETSEND			
			comindex++;
			if( comindex > 2 ) 	comindex=0;		
#endif			
			GetSendPacket( &KeepAlivebuff , 0xAA , 0x00 , data ) ;

			send_msg( (char *)&KeepAlivebuff,sizeof(DefTypeUART2CAN) ) ;	
			HexaPrint(  (unsigned char *)&KeepAlivebuff,sizeof(DefTypeUART2CAN) ) ;
			printf("........................................................\n") ;

			timer_restart(&keeplive_timer);
			timer_set(&keeplivewait_timer, CLOCK_SECOND * 8);
			livepacket_sendflag=1;
		}	
	}
	if ( errmsg ) {
		free( errmsg ) ;
	}
	cf_db_free_table (results);	
	// 전송된 DB data 처리 

	memset(SQLCMD,0x00,256 ) ; 
	sqlpf(SQLCMD,(char*)"delete from ifs_control_input" )  ;	
	errmsg = NULL ;
	ASSERT (CF_DB_OK == cf_db_exec (&db, SQLCMD, NULL, 0, &errmsg), errmsg);
        if ( errmsg ) {
                free( errmsg ) ;
        }
	

	ASSERT_OK (cf_db_close (&db), "fail to close");
	cf_release_db (&db);
}

*/
char checkLoginTable( unsigned char *ID,unsigned char *PASS )
{
	cf_db_t db;
	
	char findresult=0;
	unsigned char data[8] ;
	char SQLCMD[256] ;
		
	//char TEST_DB[] = "127.0.0.1;TESTNODE;root;0419;" ;
	//char TESTDB[] = "127.0.0.1;ict_local;root;cnrtksdusrnth;" ;
	char TESTDB[] = "127.0.0.1;shopinfo;root;0419;" ;	

	int rows, columns, i, j;   
	char *errmsg; 
	char **results;
	
	ASSERT_OK (cf_init_db (&db, TESTDB), "fail to init");
	ASSERT_OK (cf_db_open (&db), "fail to open");

	memset(SQLCMD,0x00,256 ) ; 
	sqlpf(SQLCMD,(char*)"select * from shopuser where id='%s' and passwd='%s'",ID,PASS )  ;	

	
	printf("LOGIN confirm: %s\n", SQLCMD ) ; 
	errmsg = NULL ;
	ASSERT (CF_DB_OK == cf_db_get_table (&db, SQLCMD , &results, &rows, &columns, &errmsg), errmsg);
		
	if( rows > 0 )
	{
		printf("LOGIN OK : %s %s  mode:%d\n",results[1*columns+0],results[1*columns+7],atoi(results[1*columns+8])) ; 	
		findresult=atoi(results[1*columns+8]);
	} else {
		printf("LOGIN FAIL : %s\n",ID) ; 
		findresult=50;
	}

	if ( errmsg ) {
		free( errmsg ) ;
	}
	cf_db_free_table (results);	
	

	ASSERT_OK (cf_db_close (&db), "fail to close");
	cf_release_db (&db);
	return findresult ;
}



void recordDB(  DefTypeUART2CAN pbuff )
{
	
	cf_db_t db;
	int rows, columns, i, j;   
	char *errmsg; 
	char **results;
	
	char SQLCMD[256] ;
	char datastr[128] ;
	char TESTDB[] = "127.0.0.1;ict_local;root;cnrtksdusrnth;" ;
	
	ASSERT_OK (cf_init_db (&db, TESTDB), "fail to init");
	ASSERT_OK (cf_db_open (&db), "fail to open");
	
	//ASSERT (CF_DB_OK == cf_db_exec (&db, "create table temp (id integer, value double)", NULL, 0, &errmsg), errmsg);
	memset(SQLCMD,0x00,256 ) ; 
	if( pbuff.nData[0] == 0x11 ) {
			
		printf("Keep alive TCP Packet received.................................\n") ;
	}
	
	sqlpf(datastr,(char*)"%d,%d,%d,%d,%d,%d,%d,%d,now()",
		pbuff.nData[0],pbuff.nData[1],pbuff.nData[2],pbuff.nData[3],
		pbuff.nData[4],pbuff.nData[5],pbuff.nData[6],pbuff.nData[7]
		) ; 


	sqlpf(SQLCMD,(char*)"insert into ifs_control_output (%s) values (%s)",
		"std_gb,val_data1,val_data2,val_data3,val_data4,val_data5,val_data6,val_data7,output_ymd",
		datastr )  ;

	errmsg = NULL ;
	ASSERT (CF_DB_OK == cf_db_exec (&db, SQLCMD, NULL, 0, &errmsg), errmsg);
	if ( errmsg ) {
		free( errmsg ) ;
	}
	ASSERT_OK (cf_db_close (&db), "fail to close");
	cf_release_db (&db);

}


void SQLquery(  char *sqlcmd )
{
	
	cf_db_t db;
	int rows, columns, i, j;   
	char *errmsg; 
	char **results;
	
	char SQLCMD[256] ;
	char datastr[128] ;
	//char TESTDB[] = "127.0.0.1;ict_local;root;cnrtksdusrnth;" ;
	char TESTDB[] = "127.0.0.1;shopinfo;root;0419;" ;	
	
	ASSERT_OK (cf_init_db (&db, TESTDB), "fail to init");
	ASSERT_OK (cf_db_open (&db), "fail to open");
	
	//ASSERT (CF_DB_OK == cf_db_exec (&db, "create table temp (id integer, value double)", NULL, 0, &errmsg), errmsg);
	memset(SQLCMD,0x00,256 ) ; 
	memcpy( SQLCMD,sqlcmd,256 ) ;
	printf("SQLquery: %s \n",SQLCMD);
	errmsg = NULL ;
	ASSERT (CF_DB_OK == cf_db_exec (&db, SQLCMD, NULL, 0, &errmsg), errmsg);
	if ( errmsg ) {
		free( errmsg ) ;
	}
	ASSERT_OK (cf_db_close (&db), "fail to close");
	cf_release_db (&db);

}

void SearchICTcmdFromDB( char *cmd )
{
	cf_db_t db;
	
	unsigned char data[8] ;

	char TESTDB[] = "127.0.0.1;tcprelay;ictuser;1234;" ;	
	
	int rows, columns, i, j;   
	char *errmsg; 
	char **results;
	

	
	ASSERT_OK (cf_init_db (&db, TESTDB), "fail to init");
	ASSERT_OK (cf_db_open (&db), "fail to open");

	errmsg = NULL ;
	ASSERT (CF_DB_OK == cf_db_get_table (&db, "select * from deviceinfo", &results, &rows, &columns, &errmsg), errmsg);
		
	if( rows > 0 )
	{
		for ( i = 1; i <= rows; i++) {
			/*
			//for ( j = 0; j < columns-1; j++) {
			//	printf( "%d,%d = %s \n",j,i,results[i*columns+j]);
			NodeInfo[i-1].code = atoi(results[i*columns+0] ) ; 
			strcpy(NodeInfo[i-1].name, results[i*columns+1] ) ;
			strcpy(NodeInfo[i-1].ip, results[i*columns+2] ) ;
			NodeInfo[i-1].canid = atoi(results[i*columns+3] ) ; 
			NodeInfo[i-1].port = atoi(results[i*columns+4] ) ; 
			NodeInfo[i-1].status = atoi(results[i*columns+5] ) ; 
			NodeInfo[i-1].isConnected=0;
			//}
			printf("%d,%s,%s,%d,%d,%d,[%d]\n",NodeInfo[i-1].code,NodeInfo[i-1].name,NodeInfo[i-1].ip,
				NodeInfo[i-1].canid,NodeInfo[i-1].port,NodeInfo[i-1].status,0) ;
				//NodeInfo[i-1].canid,NodeInfo[i-1].port,NodeInfo[i-1].status) ;
			*/
			printf("field %s,%s,%s,%s,%s\n",results[i*columns+0],results[i*columns+2],results[i*columns+4],results[i*columns+6],results[i*columns+7] ) ;
			
			printf("........................................................\n") ;
	
			
		}
		
	}
	if ( errmsg ) {
		free( errmsg ) ;
	}
	
	cf_db_free_table (results);	
	// 전송된 DB data 처리 


	ASSERT_OK (cf_db_close (&db), "fail to close");
	cf_release_db (&db);
}

char* SearchICTvalFromDB( char *farmNo,char *icId,char eqType,char *eqId,char *cmd,char *retval )
{
	cf_db_t db;
	
	char retdata[64] ;
	char SQLCMD[256] ;
	char TESTDB[] = "127.0.0.1;tcprelay;ictuser;1234;" ;	
	char nodata[] = "0" ;	
	char datastr[128] ;

	int rows, columns, i, j;   
	char *errmsg; 
	char **results;
	

	memset(SQLCMD,0x00,256 ) ; 
	memset(retdata,0x00,64 ) ; 
	memset(datastr,0x00,128 ) ; 
	ASSERT_OK (cf_init_db (&db, TESTDB), "fail to init");
	ASSERT_OK (cf_db_open (&db), "fail to open");

	errmsg = NULL ;

	
	sqlpf( SQLCMD,(char*)"select * from TE_DEVICE_SET_VALUE where FARM_NO='%s' and IC_NO='%s' and EQ_TYPE='%c' and EQ_ID='%s' and CMD_COLUMN='%s'",
		farmNo,icId,eqType,eqId,cmd )  ;
	//printf("SearchICTvalFromDB SQL %s\n", SQLCMD ) ;
	ASSERT (CF_DB_OK == cf_db_get_table (&db, SQLCMD , &results, &rows, &columns, &errmsg), errmsg);
	if ( errmsg ) {
		free( errmsg ) ;
	}
		
	if( rows > 0 )
	{
		//printf("DB findvalue %s,%s,%s,%s,%s\n",results[1*columns+0],results[1*columns+1],results[1*columns+2],results[1*columns+3],results[1*columns+6] ) ;
		
		//printf("value %s,size %d\n",results[1*columns+6],strlen(results[1*columns+6]) ) ;
		memcpy(retdata,results[1*columns+8],strlen(results[1*columns+8])) ;
		memcpy(retval,results[1*columns+8],strlen(results[1*columns+8])) ;
		
	} else {
		printf("SearchICTvalFromDB SQL %s\n", SQLCMD ) ;
		printf("SearchICTvalFromDB TE_DEVICE_SET_VALUE not find data\n") ;	
		memcpy( retdata,nodata,1);
		memcpy( retval,nodata,1);

		sqlpf(datastr,(char*)"'%s','%s','%c','%s','1','R','%s','%s'",
			farmNo,icId,eqType,eqId,cmd,retval
			) ; 

		memset(SQLCMD,0x00,256 ) ; 
		sqlpf(SQLCMD,(char*)"insert into TE_DEVICE_SET_VALUE (%s) values (%s)",
			"FARM_NO,IC_NO,EQ_TYPE,EQ_ID,IC_ID,CMD_TYPE,CMD_COLUMN,SET_VALUE",
			datastr )  ;

		errmsg = NULL ;
		ASSERT (CF_DB_OK == cf_db_exec (&db, SQLCMD, NULL, 0, &errmsg), errmsg);
		if ( errmsg ) {
			free( errmsg ) ;
		}
		printf( "%s \n",SQLCMD ) ; 
	}


	
	cf_db_free_table (results);	
	// 전송된 DB data 처리 


	ASSERT_OK (cf_db_close (&db), "fail to close");
	cf_release_db (&db);

	
	return retdata;
}


void updateCMD( int farmNo,char *icID,char devType,char *devID,char *column_nm,char *setvalue)
{
	
	cf_db_t db;
	int rows, columns, i, j;   
	char *errmsg; 
	char **results;
	
	char SQLCMD[256] ;
	char datastr[128] ;
	char TESTDB[] = "127.0.0.1;tcprelay;ictuser;1234;" ;	
	
	ASSERT_OK (cf_init_db (&db, TESTDB), "fail to init");
	ASSERT_OK (cf_db_open (&db), "fail to open");
	
	

	
	memset(SQLCMD,0x00,256 ) ; 
	memset(datastr,0x00,128 ) ; 

	sqlpf(SQLCMD,(char*)"select * from TE_DEVICE_SET_VALUE where FARM_NO='%d' and IC_NO='%s' and EQ_TYPE='%c' and EQ_ID='%s' and CMD_COLUMN='%s'",
		farmNo,icID,devType,devID,column_nm 	)  ;

	errmsg = NULL ;
	ASSERT (CF_DB_OK == cf_db_get_table (&db, SQLCMD , &results, &rows, &columns, &errmsg), errmsg);
	if ( errmsg ) {
		free( errmsg ) ;
	}
		
	if( rows > 0 ) // 해당값에 대한 데이터가 있을경우 업데이트 
	{	
		
		memset(SQLCMD,0x00,256 ) ; 
		
		sqlpf(SQLCMD,(char*)"update TE_DEVICE_SET_VALUE SET SET_VALUE='%s',LOG_INS_DT=now() WHERE FARM_NO='%d' and IC_NO='%s' and EQ_TYPE='%c' and EQ_ID='%s' and CMD_COLUMN='%s'",
			setvalue ,farmNo,icID,devType,devID,column_nm 
		) ;
		//printf( "%s \n",SQLCMD ) ; 
	} else {

		sqlpf(datastr,(char*)"'%d','%s','%c','%s','1','R','%s','%s'",
			farmNo,icID,devType,devID,column_nm,setvalue
			) ; 

		memset(SQLCMD,0x00,256 ) ; 
		sqlpf(SQLCMD,(char*)"insert into TE_DEVICE_SET_VALUE (%s) values (%s)",
			"FARM_NO,IC_NO,EQ_TYPE,EQ_ID,IC_ID,CMD_TYPE,CMD_COLUMN,SET_VALUE",
			datastr )  ;

		//printf( "%s \n",SQLCMD ) ; 

	}
	cf_db_free_table (results);	


	errmsg = NULL ;
	ASSERT (CF_DB_OK == cf_db_exec (&db, SQLCMD, NULL, 0, &errmsg), errmsg);
	if ( errmsg ) {
		free( errmsg ) ;
	}



	ASSERT_OK (cf_db_close (&db), "fail to close");
	cf_release_db (&db);

}

void updateCMDvalue( int farmNo,char *icID,char devType,char *devID,char *column_nm,char *setvalue)
{
	
	cf_db_t db;
	int rows, columns, i, j;   
	char *errmsg; 
	char **results;
	
	char SQLCMD[256] ;
	char datastr[128] ;
	char TESTDB[] = "127.0.0.1;tcprelay;ictuser;1234;" ;	
	
	ASSERT_OK (cf_init_db (&db, TESTDB), "fail to init");
	ASSERT_OK (cf_db_open (&db), "fail to open");
	
	

	
	memset(SQLCMD,0x00,256 ) ; 
	memset(datastr,0x00,128 ) ; 

	sqlpf(SQLCMD,(char*)"select * from TE_DEVICE_SET_VALUE where FARM_NO='%d' and IC_NO='%s' and EQ_TYPE='%c' and EQ_ID='%s' and CMD_COLUMN='%s' and SET_VALUE='%s'",
		farmNo,icID,devType,devID,column_nm,setvalue )  ;

	errmsg = NULL ;
	ASSERT (CF_DB_OK == cf_db_get_table (&db, SQLCMD , &results, &rows, &columns, &errmsg), errmsg);
	if ( errmsg ) {
		free( errmsg ) ;
	}
		
	if( rows > 0 ) // 해당값에 대한 데이터가 있을경우 업데이트 
	{	
		
		memset(SQLCMD,0x00,256 ) ; 
		
		sqlpf(SQLCMD,(char*)"update TE_DEVICE_SET_VALUE SET SET_VALUE='%s',LOG_INS_DT=now() WHERE FARM_NO='%d' and IC_NO='%s' and EQ_TYPE='%c' and EQ_ID='%s' and CMD_COLUMN='%s' and SET_VALUE='%s'",
			setvalue ,farmNo,icID,devType,devID,column_nm,setvalue 
		) ;
		printf( "%s \n",SQLCMD ) ; 
	} else {

		sqlpf(datastr,(char*)"'%d','%s','%c','%s','1','R','%s','%s'",
			farmNo,icID,devType,devID,column_nm,setvalue
			) ; 

		memset(SQLCMD,0x00,256 ) ; 
		sqlpf(SQLCMD,(char*)"insert into TE_DEVICE_SET_VALUE (%s) values (%s)",
			"FARM_NO,IC_NO,EQ_TYPE,EQ_ID,IC_ID,CMD_TYPE,CMD_COLUMN,SET_VALUE",
			datastr )  ;

		//printf( "%s \n",SQLCMD ) ; 

	}
	cf_db_free_table (results);	


	errmsg = NULL ;
	ASSERT (CF_DB_OK == cf_db_exec (&db, SQLCMD, NULL, 0, &errmsg), errmsg);
	if ( errmsg ) {
		free( errmsg ) ;
	}



	ASSERT_OK (cf_db_close (&db), "fail to close");
	cf_release_db (&db);

}

void updateCMDLIST( int farmNo,char *icID,char devType,char *devID)
{
	
	cf_db_t db;
	int rows, columns, i, j;   
	char *errmsg; 
	char **results;
	
	char SQLCMD[256] ;
	char datastr[128] ;
	char TESTDB[] = "127.0.0.1;tcprelay;ictuser;1234;" ;	
	
	ASSERT_OK (cf_init_db (&db, TESTDB), "fail to init");
	ASSERT_OK (cf_db_open (&db), "fail to open");
	
	
	for( i= 0 ; i < DEV_CMD_COUNT ;i++)
	{
	
		memset(SQLCMD,0x00,256 ) ; 

		sqlpf(SQLCMD,(char*)"select * from TE_DEVICE_SET_VALUE where FARM_NO='%d' and IC_NO='%s' and EQ_TYPE='%c' and EQ_ID='%s' and CMD_COLUMN='%s'",
			farmNo,icID,devType,devID,DEV_CMD_LIST[i] 	)  ;

		errmsg = NULL ;
		ASSERT (CF_DB_OK == cf_db_get_table (&db, SQLCMD , &results, &rows, &columns, &errmsg), errmsg);
		if ( errmsg ) {
			free( errmsg ) ;
		}
			
		if( rows > 0 ) // 해당값에 대한 데이터가 있을경우 업데이트 
		{	
			
			memset(SQLCMD,0x00,256 ) ; 
			
			sqlpf(SQLCMD,(char*)"update TE_DEVICE_SET_VALUE SET SET_VALUE='%s' WHERE FARM_NO='%d' and IC_NO='%s' and EQ_TYPE='%c' and EQ_ID='%s' and CMD_COLUMN='%s'",
				DEV_CMD_LIST_VALUE[i] ,farmNo,icID,devType,devID,DEV_CMD_LIST[i] 
			) ;
			printf( "%s \n",SQLCMD ) ; 
		} else {

			sqlpf(datastr,(char*)"'%d','%s','%c','%s','1','R','%s','%s'",
				farmNo,icID,devType,devID,DEV_CMD_LIST[i],DEV_CMD_LIST_VALUE[i] 
				) ; 

			memset(SQLCMD,0x00,256 ) ; 
			sqlpf(SQLCMD,(char*)"insert into TE_DEVICE_SET_VALUE (%s) values (%s)",
				"FARM_NO,IC_NO,EQ_TYPE,EQ_ID,IC_ID,CMD_TYPE,CMD_COLUMN,SET_VALUE",
				datastr )  ;

			//printf( "%s \n",SQLCMD ) ; 

		}
		
		errmsg = NULL ;
		ASSERT (CF_DB_OK == cf_db_exec (&db, SQLCMD, NULL, 0, &errmsg), errmsg);
		if ( errmsg ) {
			free( errmsg ) ;
		}


	}

	ASSERT_OK (cf_db_close (&db), "fail to close");
	cf_release_db (&db);

}

//char DEV_CMD_LIST[100][16] ;
//DEV_CMD_LIST_VALUE
//int  DEV_CMD_COUNT=0;
//
//
void GetICTDeviceCMD( char devType)
{
	cf_db_t db;
	
	unsigned char data[8] ;
	char SQLCMD[256] ;

	char TESTDB[] = "127.0.0.1;tcprelay;ictuser;1234;" ;	
	
	int rows, columns, i, j;   
	char *errmsg; 
	char **results;
	
	for( i = 0 ; i < 100 ;i++ )  
	{ 
		memset(DEV_CMD_LIST[i],0x00,16); 
		memset(DEV_CMD_LIST_VALUE[i],0x00,30); 
	}	


	
	
	ASSERT_OK (cf_init_db (&db, TESTDB), "fail to init");
	ASSERT_OK (cf_db_open (&db), "fail to open");

	errmsg = NULL ;
	memset(SQLCMD,0x00,256 ) ; 
	sqlpf( SQLCMD,(char*)"select * from TE_DEVICE_CMD where EQ_TYPE='%c' and CMD_TYPE='R'",devType)  ;

	ASSERT (CF_DB_OK == cf_db_get_table (&db, SQLCMD, &results, &rows, &columns, &errmsg), errmsg);
	DEV_CMD_COUNT=0;
	
	if( rows > 0 )
	{
		for ( i = 1; i <= rows; i++) {
			//printf("DevCMD list %c:%s \n",devType,results[i*columns+6]) ;
			strcpy( DEV_CMD_LIST[DEV_CMD_COUNT], results[i*columns+6] ) ;
			strcpy( DEV_CMD_LIST_VALUE[DEV_CMD_COUNT], results[i*columns+9] ) ;
			printf("DevCMD list %c:%s,%s \n",devType,DEV_CMD_LIST[DEV_CMD_COUNT],DEV_CMD_LIST_VALUE[DEV_CMD_COUNT]) ;
			DEV_CMD_COUNT++ ;
		}
		
	}
	if ( errmsg ) {
		free( errmsg ) ;
	}
	
	cf_db_free_table (results);	
	// 전송된 DB data 처리 


	ASSERT_OK (cf_db_close (&db), "fail to close");
	cf_release_db (&db);
}

void GetICTInfo( void )
{
	cf_db_t db;
	
	unsigned char data[8] ;
	

	char TESTDB[] = "127.0.0.1;tcprelay;ictuser;1234;" ;	
	
	int rows, columns, i, j;   
	char *errmsg; 
	char **results;
	
	
	ASSERT_OK (cf_init_db (&db, TESTDB), "fail to init");
	ASSERT_OK (cf_db_open (&db), "fail to open");

	errmsg = NULL ;
	ASSERT (CF_DB_OK == cf_db_get_table (&db, "select * from TE_ICT_CONFIG", &results, &rows, &columns, &errmsg), errmsg);
		
	if( rows > 0 )
	{
		for ( i = 1; i <= rows; i++) {
			//printf("DB field %s,%s,%s,%s,%s\n",results[i*columns+0],results[i*columns+1],results[i*columns+2],results[i*columns+3],results[i*columns+4] ) ;
			strcpy( MY_FARMNO , results[i*columns+0] ) ; 
			printf("MY_FARMNO :%s \n" , MY_FARMNO ) ; 

		}
		
	}
	printf("GetICTInfo return 1\n"  ) ; 
	if ( errmsg ) {
		free( errmsg ) ;
	}
	printf("GetICTInfo return 2\n"  ) ; 
	cf_db_free_table (results);	
	// 전송된 DB data 처리 

	printf("GetICTInfo return 3\n"  ) ; 	
	ASSERT_OK (cf_db_close (&db), "fail to close");
	printf("GetICTInfo return 4\n"  ) ; 
	cf_release_db (&db);
	printf("GetICTInfo return 5\n"  ) ; 
	
}

void GetICTDeviceInfo( int resetflag )
{
	cf_db_t db;
	
	unsigned char data[8] ;
	char SQLCMD[256] ;

	char TESTDB[] = "127.0.0.1;tcprelay;ictuser;1234;" ;	
	
	int rows, columns, i, j;   
	char *errmsg; 
	char **results;
	

	DefCycleDevice addDev;

	if( resetflag ) ICTDevInfo.clear() ;
	
	ASSERT_OK (cf_init_db (&db, TESTDB), "fail to init");
	ASSERT_OK (cf_db_open (&db), "fail to open");

	errmsg = NULL ;

	memset(SQLCMD,0x00,256 ) ; 
	sqlpf( SQLCMD,(char*)"select * from TE_DEVICE_LIST where USE_YN='Y' and SEND_CYCLE > 0 and FARM_NO='%s'",MY_FARMNO)  ;

	//MY_FARMNO
	ASSERT (CF_DB_OK == cf_db_get_table (&db,SQLCMD , &results, &rows, &columns, &errmsg), errmsg);
		
	if( rows > 0 )
	{
		for ( i = 1; i <= rows; i++) {
			//printf("DB field %s,%s,%s,%s,%s\n",results[i*columns+0],results[i*columns+1],results[i*columns+2],results[i*columns+5],results[i*columns+6] ) ;
			memset( addDev.devId,0x00,8) ;
			memset( addDev.icNo,0x00,32) ;
			
			strcpy( addDev.icNo, results[i*columns+1] ) ;
			strcpy( addDev.devId, results[i*columns+3] ) ;

			addDev.farmNo=atoi(results[i*columns+0] );
			addDev.devType=results[i*columns+2][0];
			addDev.enable=results[i*columns+7][0];
			addDev.setcycle=atoi(results[i*columns+4] ); // minute
			addDev.curcycle=0;
			ICTDevInfo.push_back(addDev) ;
		
		}
		
	}
	if ( errmsg ) {
		free( errmsg ) ;
	}
	
	cf_db_free_table (results);	
	// 전송된 DB data 처리 


	ASSERT_OK (cf_db_close (&db), "fail to close");
	cf_release_db (&db);
}

void MakeJPacketICTdeviceAll( void)
{
	cf_db_t db;

	json_object *pkgobj,*cmdobj,*dval ;	
	json_object *respacketobj;
	json_object *cmdlistobj;

	json_object *arrayobj;
    json_object *cmd_array_obj, *cmd_array_obj_name;

    //DefCycleDevice addDev 
	int cmdobjcount=0;
	char cmdmode=0;
	char devid[64] ;
	char farmno[64];
	char icno[8];
	char equno[8];
	char timestr[64]; 
	char *ctrlcmd ;


	if( ICTDevInfo.size() < 1 ) 
	{
		printf("not found cycled device !!\n");
		return ;	
	}

	//farmno=icno=equno=timestr=NULL;	

	char SQLCMD[256] ;
	unsigned char data[8] ;

	char TESTDB[] = "127.0.0.1;tcprelay;ictuser;1234;" ;	
	
	int rows, columns, i, j,c;   
	char *errmsg; 
	char **results;
	char devCount=0;
	int sendPacketsize=0;
	int getsetval_now =0;
	
	memset(SQLCMD,0x00,256 ) ; 
	memset(devid,0x00,64 ) ; 
	memset(farmno,0x00,64 ) ; 
	memset(icno,0x00,8 ) ; 
	memset(equno,0x00,8 ) ; 
	memset(timestr,0x00,64 ) ; 

	ASSERT_OK (cf_init_db (&db, TESTDB), "fail to init");
	ASSERT_OK (cf_db_open (&db), "fail to open");

	DefCycleDevice preDev = ICTDevInfo.at(0) ;
	sqlpf( farmno,(char*)"%d",preDev.farmNo )  ;
	sqlpf( icno,(char*)"%s",preDev.icNo )  ;
	//return ;
	
	
	struct tm tm = *localtime(&(time_t){time(NULL)});
    sqlpf( timestr,(char*)"%04d-%02d-%02d %02d:%02d:%02d",tm.tm_year+1900,tm.tm_mon+1,tm.tm_mday,tm.tm_hour,tm.tm_min,tm.tm_sec )  ;
	
	
	respacketobj = json_object_new_object();

	
	json_object *jstrtype;

	jstrtype = json_object_new_string("req") ;
	json_object_object_add(respacketobj, "type",jstrtype  );	

	if( preDev.devType == 'C' ) // 냉반방기 통합제어기에서 처리 
	{
		//jstrtype = json_object_new_string("res") ;
		//json_object_object_add(respacketobj, "type",jstrtype  );
	} else {

	}
	

	json_object *jstrfarmno = json_object_new_string(farmno) ;
	json_object_object_add(respacketobj, "farmNo",jstrfarmno);
	
	json_object *jstricno = json_object_new_string(icno) ;
	json_object_object_add(respacketobj, "icNo",jstricno);
	
	
	arrayobj = json_object_new_array();	

	//DefCycleDevice addDev;		

	for( c = 0 ;c < ICTDEVICE_COUNT;c++)
	{
		DefCycleDevice &addDev = ICTDevInfo.at(c) ;

		if( addDev.curcycle > 0 ) 
		{
			addDev.curcycle--;
			//printf("Cycle DEV INFO: %s,%d\n",addDev.devId,addDev.curcycle ) ;
			if( addDev.curcycle <=0 ) 
			{
				//printf("Cycle DEV INFO: %s,%d\n",addDev.devId,addDev.curcycle ) ;
				addDev.curcycle = addDev.setcycle ;	
				
				errmsg = NULL ;

				sqlpf( SQLCMD,(char*)"select * from TE_DEVICE_CMD where EQ_TYPE='%c' and CMD_TYPE='R'",addDev.devType )  ;
				//printf("MakeJPacketICTdevice SQL %s\n", SQLCMD ) ;
				ASSERT (CF_DB_OK == cf_db_get_table (&db, SQLCMD , &results, &rows, &columns, &errmsg), errmsg);		

				sqlpf( equno,(char*)"%c%s",addDev.devType,addDev.devId )  ;
				
				cmdlistobj = json_object_new_object();

				json_object *jstrequno = json_object_new_string(equno) ;
				json_object_object_add(cmdlistobj, "eqId",jstrequno);	

				memset(ICTretval,0x00,64);
				
				if( addDev.devType == 'C')
				{
					//printf("FARMNO:%d  DEV: %s,%c,%s,%c,%d,%d\n",getDev.farmNo,getDev.icNo,getDev.devType,getDev.devId,getDev.enable,getDev.curcycle,getDev.setcycle ) ;
					//void updateCMD( int farmNo,char *icID,char devType,char *devID,char *column_nm,char *setvalue)
					
					char setvalstr[8];
					
					// curtemp
					getsetval_now = modbus_read(atoi(addDev.devId),200,1) ;
					
					memset( setvalstr,0x00,8) ;
					sqlpf( setvalstr,(char*)"%02d.%01d", getsetval_now/10,getsetval_now%10)  ;
					printf("cycled read eqId : %d readset = %s \n",atoi(addDev.devId),setvalstr) ; 
										
					updateCMD(preDev.farmNo,addDev.icNo,addDev.devType,addDev.devId,"curtemp",setvalstr) ;
					
					
					// settemp
					getsetval_now = modbus_read(atoi(addDev.devId),211,1) ;
					
					memset( setvalstr,0x00,8) ;
					sqlpf( setvalstr,(char*)"%02d.%01d", getsetval_now/10,getsetval_now%10)  ;
					printf("cycled read eqId : %d readset = %s \n",atoi(addDev.devId),setvalstr) ; 
										
					updateCMD(preDev.farmNo,addDev.icNo,addDev.devType,addDev.devId,"settemp",setvalstr) ;

					updateCMD(preDev.farmNo,addDev.icNo,addDev.devType,addDev.devId,"updateTm",timestr) ;

					
				}

				if( addDev.devType == 'E') // 군사급이기는 응답이 길기때문에 따로 요청한다.
				{
				
					if( rows > 0 )
					{				
						
						for ( i = 1; i <= rows; i++) {
							memset(ICTretval,0x00,64);
							//printf("device cmd list %s,%s,%s\n",results[i*columns+2],results[i*columns+5],results[i*columns+6]) ;
						
							if( cmdlistobj != NULL) // 다중 명령 처리
							{
								
								SearchICTvalFromDB(farmno,icno,addDev.devType,addDev.devId,(char *)results[i*columns+6],ICTretval) ;
						
								json_object *jstrcmdstr = json_object_new_string("0") ;
								json_object_object_add(cmdlistobj, (char *)results[i*columns+6],jstrcmdstr );				
								//printf("%s %s\n",(char *)results[i*columns+6],ICTretval); 
							} 
						
						} // for
						json_object_array_add(arrayobj, cmdlistobj);

					}

					json_object_object_add(respacketobj, "cmdString", arrayobj);	

					sendPacketsize = (int)strlen(json_object_to_json_string(respacketobj)) ;
					printf("Etype cycled socket post json(%d:%d)=%s\n",devCount,sendPacketsize,json_object_to_json_string(respacketobj));			
					
					memset( GLB_postbuf,0x00,1024*10 ) ; 
					if( sendPacketsize >= 1024*10 ) sendPacketsize = 1024*10-1; 

					memcpy( GLB_postbuf,json_object_to_json_string(respacketobj),sendPacketsize );
					
					send_msg( (char *)GLB_postbuf, sendPacketsize ) ;	usleep(1000*500) ;		
					
					// 패킷 재구성
					respacketobj = json_object_new_object();
					json_object_object_add(respacketobj, "type",jstrtype  );
					json_object_object_add(respacketobj, "farmNo",jstrfarmno);
					json_object_object_add(respacketobj, "icNo",jstricno);

					arrayobj = json_object_new_array();	

				} else {
				
				
					if( rows > 0 )
					{				
						
						for ( i = 1; i <= rows; i++) {
							memset(ICTretval,0x00,64);
							//printf("device cmd list %s,%s,%s\n",results[i*columns+2],results[i*columns+5],results[i*columns+6]) ;
						
							if( cmdlistobj != NULL) // 다중 명령 처리
							{
								
								SearchICTvalFromDB(farmno,icno,addDev.devType,addDev.devId,(char *)results[i*columns+6],ICTretval) ;
						
								json_object *jstrcmdstr = json_object_new_string("0") ;
								json_object_object_add(cmdlistobj, (char *)results[i*columns+6],jstrcmdstr );				
								//printf("%s %s\n",(char *)results[i*columns+6],ICTretval); 
							} 
						
						} // for
						json_object_array_add(arrayobj, cmdlistobj);
						devCount++;



						if( devCount > 9 ) 
						{
							json_object_object_add(respacketobj, "cmdString", arrayobj);	

							sendPacketsize = (int)strlen(json_object_to_json_string(respacketobj)) ;
							printf("cycled socket post json(%d:%d)=%s\n",devCount,sendPacketsize,json_object_to_json_string(respacketobj));			
							
							memset( GLB_postbuf,0x00,1024*10 ) ; 
							if( sendPacketsize >= 1024*10 ) sendPacketsize = 1024*10-1; 

							memcpy( GLB_postbuf,json_object_to_json_string(respacketobj),sendPacketsize );
							
							send_msg( (char *)GLB_postbuf, sendPacketsize ) ;	usleep(1000*500) ;		
							
							// 패킷 재구성
							respacketobj = json_object_new_object();
							json_object_object_add(respacketobj, "type",jstrtype  );
							json_object_object_add(respacketobj, "farmNo",jstrfarmno);
							json_object_object_add(respacketobj, "icNo",jstricno);

							arrayobj = json_object_new_array();	
							devCount=0;
						}
					} 
				}
				
				//json_object_put(cmdlistobj);	
				
				
				
				if ( errmsg ) {
					free( errmsg ) ;
				}
				cf_db_free_table (results);	

			}
		} //if( addDev.curcycle > 0 ) 

	} //for


	json_object_object_add(respacketobj, "cmdString", arrayobj);	


	
	
	// 전송된 DB data 처리 


	ASSERT_OK (cf_db_close (&db), "fail to close");
	cf_release_db (&db);
	


	//struct tm tm = *localtime(&(time_t){time(NULL)});
    //sqlpf( timestr,(char*)"%04d%02d%02d_%02d%02d%02d",tm.tm_year+1900,tm.tm_mon+1,tm.tm_mday,tm.tm_hour,tm.tm_min,tm.tm_sec )  ;
	//printf("cycletimer() %04d%02d%02d_%02d%02d%02d \n",tm.tm_year+1900,tm.tm_mon+1,tm.tm_mday,tm.tm_hour,tm.tm_min,tm.tm_sec ) ;	

	//json_object *jstrtime = json_object_new_string(timestr) ;
	//json_object_object_add(respacketobj, "regTm",jstrtime);
	


	
	if( devCount>0 ) // 전송할 객체가 있을경우 
	{
		sendPacketsize = (int)strlen(json_object_to_json_string(respacketobj)) ;
		printf("cycled socket post json(%d:%d)=%s\n",devCount,sendPacketsize,json_object_to_json_string(respacketobj));			
		
		memset( GLB_postbuf,0x00,1024*10 ) ; 
		if( sendPacketsize >= 1024*10 ) sendPacketsize = 1024*10-1; 

		memcpy( GLB_postbuf,json_object_to_json_string(respacketobj),sendPacketsize );
		
		send_msg( (char *)GLB_postbuf, sendPacketsize ) ;	usleep(1000*500) ;
	}
	
	//printf("iotRcvApi.json post string=%s\n", GLB_postbuf);
	//PostJSON("http://iot.pigplanxe.co.kr/iotRcvApi.json",GLB_postbuf); 
	
}


void MakeJPacketICTdevice( DefCycleDevice addDev )
{
	cf_db_t db;

	json_object *pkgobj,*cmdobj,*dval ;	
	json_object *respacketobj;
	json_object *cmdlistobj;

	json_object *arrayobj;
    json_object *cmd_array_obj, *cmd_array_obj_name;


	int cmdobjcount=0;
	char cmdmode=0;
	char devid[64] ;
	char farmno[64];
	char icno[8];
	char equno[8];
	char timestr[64]; 
	char *ctrlcmd ;


	//farmno=icno=equno=timestr=NULL;	

	char SQLCMD[256] ;
	unsigned char data[8] ;

	char TESTDB[] = "127.0.0.1;tcprelay;ictuser;1234;" ;	
	
	int rows, columns, i, j;   
	char *errmsg; 
	char **results;
	
	
	memset(SQLCMD,0x00,256 ) ; 
	memset(devid,0x00,64 ) ; 
	memset(farmno,0x00,64 ) ; 
	memset(icno,0x00,8 ) ; 
	memset(equno,0x00,8 ) ; 
	memset(timestr,0x00,64 ) ; 

	ASSERT_OK (cf_init_db (&db, TESTDB), "fail to init");
	ASSERT_OK (cf_db_open (&db), "fail to open");

	sqlpf( farmno,(char*)"%d",addDev.farmNo )  ;
	sqlpf( icno,(char*)"I%d",1 )  ;
	sqlpf( equno,(char*)"%c%s",addDev.devType,addDev.devId )  ;
	
	//return ;
	respacketobj = json_object_new_object();

	
	json_object *jstrtype;
	if( addDev.devType == 'B' || addDev.devType == 'C' )
	{
		jstrtype = json_object_new_string("res") ;
		json_object_object_add(respacketobj, "type",jstrtype  );
	} else {
		jstrtype = json_object_new_string("req") ;
		json_object_object_add(respacketobj, "type",jstrtype  );
	}
	

	json_object *jstrfarmno = json_object_new_string(farmno) ;
	json_object_object_add(respacketobj, "farmNo",jstrfarmno);
	
	json_object *jstricno = json_object_new_string(icno) ;
	json_object_object_add(respacketobj, "icNo",jstricno);
	

	
	cmdlistobj = json_object_new_object();			
	
	errmsg = NULL ;

	sqlpf( SQLCMD,(char*)"select * from TE_DEVICE_CMD where EQ_TYPE='%c' and CMD_TYPE='R'",addDev.devType )  ;
	//printf("MakeJPacketICTdevice SQL %s\n", SQLCMD ) ;
	ASSERT (CF_DB_OK == cf_db_get_table (&db, SQLCMD , &results, &rows, &columns, &errmsg), errmsg);		

	arrayobj = json_object_new_array();	
	
	json_object *jstrequno = json_object_new_string(equno) ;
	json_object_object_add(cmdlistobj, "eqId",jstrequno);	

	memset(ICTretval,0x00,64);
	if( rows > 0 )
	{
		
	
		
		for ( i = 1; i <= rows; i++) {
			memset(ICTretval,0x00,64);
			//printf("device cmd list %s,%s,%s\n",results[i*columns+2],results[i*columns+5],results[i*columns+6]) ;
		
			if( cmdlistobj != NULL) // 다중 명령 처리
			{
				
				SearchICTvalFromDB(farmno,icno,addDev.devType,addDev.devId,(char *)results[i*columns+6],ICTretval) ;
		
				json_object *jstrcmdstr = json_object_new_string(ICTretval) ;
				json_object_object_add(cmdlistobj, (char *)results[i*columns+6],jstrcmdstr );				
				//printf("%s %s\n",(char *)results[i*columns+6],ICTretval); 
			} 
		
		} // for
		json_object_array_add(arrayobj, cmdlistobj);
		
		
		
		json_object_object_add(respacketobj, "cmdString", arrayobj);	
	} else {

		json_object_array_add(arrayobj, cmdlistobj);
		json_object_object_add(respacketobj, "cmdString", arrayobj);	

	}
	if ( errmsg ) {
		free( errmsg ) ;
	}
	
	cf_db_free_table (results);	
	// 전송된 DB data 처리 


	ASSERT_OK (cf_db_close (&db), "fail to close");
	cf_release_db (&db);
	


	struct tm tm = *localtime(&(time_t){time(NULL)});
    sqlpf( timestr,(char*)"%04d%02d%02d_%02d%02d%02d",tm.tm_year+1900,tm.tm_mon+1,tm.tm_mday,tm.tm_hour,tm.tm_min,tm.tm_sec )  ;
	//printf("cycletimer() %04d%02d%02d_%02d%02d%02d \n",tm.tm_year+1900,tm.tm_mon+1,tm.tm_mday,tm.tm_hour,tm.tm_min,tm.tm_sec ) ;	

	//json_object *jstrtime = json_object_new_string(timestr) ;
	//json_object_object_add(respacketobj, "regTm",jstrtime);
	

	memset(GLB_postbuf,0x00,1024*10 ) ; 
	memcpy( GLB_postbuf,json_object_to_json_string(respacketobj),(int)strlen(json_object_to_json_string(respacketobj)) );

	printf("cycled socket post json=%s\n", GLB_postbuf);			
	send_msg( (char *)GLB_postbuf, strlen(GLB_postbuf) ) ;	usleep(1000*200) ;
	
	//printf("iotRcvApi.json post string=%s\n", GLB_postbuf);
	//PostJSON("http://iot.pigplanxe.co.kr/iotRcvApi.json",GLB_postbuf); 
	
}



void ErrorJsonSend ( char *farmno,char *icno,char *equno,char *errstr)
{
	json_object *respacketobj;
	respacketobj = json_object_new_object();
	
	

	json_object *jstrtype = json_object_new_string("res") ;
	json_object_object_add(respacketobj, "type",jstrtype  );
	
	json_object *jstrfarmno = json_object_new_string(farmno) ;
	json_object_object_add(respacketobj, "farmNo",jstrfarmno);
	
	json_object *jstricno = json_object_new_string(icno) ;
	json_object_object_add(respacketobj, "icNo",jstricno);
	
	json_object *jstrequno = json_object_new_string(equno) ;
	json_object_object_add(respacketobj, "eqId",jstrequno);		
	
	json_object *jstrtime = json_object_new_string(errstr) ;
	json_object_object_add(respacketobj, "regTm",jstrtime);

	json_object *jstrcmdstr = json_object_new_string(errstr) ;
	json_object_object_add(respacketobj, "cmdString", jstrcmdstr);
	
	memset(GLB_postbuf,0x00,1024*10 ) ; 
	memcpy( GLB_postbuf,json_object_to_json_string(respacketobj),(int)strlen(json_object_to_json_string(respacketobj)) );

	printf("myobj.to_string()=%s\n", GLB_postbuf);				

	// 소켓연결응답인지 web api 응답인지 구분
	send_msg( (char *)GLB_postbuf, strlen(GLB_postbuf) ) ;			
}



//////////////////////////////////////////////////
//
// 장비로 부터 수신되 패킷 처리 
//
void JsonPaketParsingDevice(char *jsonpaketstr)
{
	json_object *arrayobj;
	json_object *pkgobj,*cmdobj,*dval ;	
	json_object *respacketobj;
	json_object *cmdlistobj;

    json_object *cmd_array_obj, *cmd_array_obj_name;

	int cmdobjcount=0;
	int i,devMode,findEqno;
	int arraylen=0;
	char cmdmode=0;
	char devid[64] ;
	char eqType;
	char eqId[8];
	char eqIdBack[8];
	char eqIdNum[8];

	char *farmno,*icno,*equno,*timestr,*ctrlcmd ;
	farmno=icno=equno=timestr=NULL;

	int typeEcnt=0;
	//system("free");
	//printf("JsonPaketParsing()=%s\n", jsonpaketstr);	
	memset( eqIdNum,0x00,8 ) ;


	devMode=0;
	pkgobj = json_tokener_parse(jsonpaketstr); 

	
	dval = json_object_object_get(pkgobj, "type");
	if( dval != NULL )
	{
		printf("type:%s\n",json_object_get_string(dval) ) ;  
		if( strcmp("req",json_object_get_string(dval)) == 0 )
		{
			cmdmode=1;
			printf("req command\n" ) ; 
		}
		if( strcmp("res",json_object_get_string(dval)) == 0 )
		{
			cmdmode=2;
			printf("res command\n" ) ; 
		}
		if( strcmp("set",json_object_get_string(dval)) == 0 )
		{
			cmdmode=3;
			printf("set command\n" ) ; 
		}
	}


	if( cmdmode != 2 ) return ;  // device response packet 만 처리 

	dval = json_object_object_get(pkgobj, "farmNo");
	if( dval != NULL ) farmno = json_object_get_string(dval);
	else farmno="error";
	printf("farmNo:%s\n",farmno ) ;  
	
	dval = json_object_object_get(pkgobj, "icNo");
	if( dval != NULL ) icno = json_object_get_string(dval);
	else icno="error";
	printf("icNo:%s\n",icno ) ;  
	/*
	dval = json_object_object_get(pkgobj, "eqId");
	if( dval != NULL ) equno = json_object_get_string(dval);
	else equno="error";
	printf("eqId:%s\n",equno ) ; 			
	*/
	
	//dval = json_object_object_get(pkgobj, "regTm");
	//if( dval != NULL ) timestr = json_object_get_string(dval);
	//else timestr="error";
	//printf("regTime:%s\n",timestr  ) ;
		
	// device reponse access ------------------------------------------------
	ctrlcmd=NULL;

	dval = json_object_object_get(pkgobj, "cmdString");
	if( dval != NULL ) ctrlcmd  = json_object_get_string(dval);		
	
	if( ctrlcmd != NULL && ctrlcmd[0] != '[')
	{
		printf("ctrl cmdString:%s\n",ctrlcmd ) ; 
		cmdobj = NULL ;

	} else { 

		if( ctrlcmd != NULL )
		{
			printf("array cmdString:%s\n",ctrlcmd ) ; 
			cmdobj = json_object_object_get(pkgobj, "cmdString");
		} else { 
			printf("not array cmdString:%s\n",ctrlcmd ) ; 
			cmdobj = NULL ;
			return ;
		}
		
	}
					
	respacketobj = json_object_new_object();
	

	json_object *jstrtype = json_object_new_string("res") ;
	json_object_object_add(respacketobj, "type",jstrtype  );
	
	

	json_object *jstrfarmno = json_object_new_string(farmno) ;
	json_object_object_add(respacketobj, "farmNo",jstrfarmno);
	
	json_object *jstricno = json_object_new_string(icno) ;
	json_object_object_add(respacketobj, "icNo",jstricno);
	
	//json_object *jstrequno = json_object_new_string(equno) ;
	//json_object_object_add(respacketobj, "eqId",jstrequno);		
	
	//json_object *jstrtime = json_object_new_string(timestr) ;
	//json_object_object_add(respacketobj, "regTm",jstrtime);
    memset( eqIdBack,0x00,8 ) ;
    
    arrayobj = json_object_new_array();	
	
	if( cmdobj != NULL) // 다중 명령 처리
	{
		
		arraylen = json_object_array_length(cmdobj);
		
		for (i = 0; i < arraylen; i++) {
		
			
			
			cmdlistobj = json_object_new_object();		
			cmd_array_obj = json_object_array_get_idx(cmdobj, i);
			findEqno=0;
			json_object_object_foreach(cmd_array_obj, key, picval) 
			{	
				memset(ICTretval,0x00,64);
				
				if( !strcmp((char *)key,"eqId") )
				{
					memset(devid,0x00,64 ) ; 
					equno = json_object_get_string(picval) ;
					findEqno=1;
					eqType=equno[0];
					
					memset( eqId,0x00,8 ) ;
					
					memcpy( eqId,&equno[1],strlen(equno)-1) ;
					
					if( strcmp(eqIdBack,eqId) )
					{
						memset( eqIdBack,0x00,8 ) ;
						memcpy( eqIdBack,&equno[1],strlen(equno)-1) ;
						typeEcnt=0;
					}
					//sqlpf(devid,(char*)"eqType:%c eqId:%s",eqType,eqId )  ;
					//printf("eqID:%s\n",devid ) ; 			
					
				
				} 

				json_object *jstrcmdstr = json_object_new_string(ICTretval) ;
				json_object_object_add(cmdlistobj, (char *)key,jstrcmdstr );				
				//printf("%s %s\n",(char *)key,ICTretval); 
			
			}	
	    
			json_object_array_add(arrayobj, cmdlistobj);
			if( findEqno  )
			{

				if( eqType == 'E' ) 
				{
					sprintf(eqIdNum, "%s-%d",eqId,typeEcnt);
					typeEcnt++;
				} else sprintf(eqIdNum, "%s",eqId);  // LIM   2019.12.09
				
				json_object_object_foreach(cmd_array_obj, key2, picval2) 
				{	
					memset(ICTretval,0x00,64);
					
					
					if( strcmp((char *)key2,"eqId") ) // eqID 가 아니값에 대한  디비 업데이트 
					{
						//void updateCMD( int farmNo,char *icID,char devType,char *devID,char *column_nm,char *setvalue)
						//farmno,icno 
						printf("%c%s:%s %s\n",eqType,eqIdNum,(char *)key2,json_object_get_string(picval2)); 
						
						//if( eqType=='E' ) // 군사급이기의 경우 pigmnum 이 여러개임으로 따로 처리함
						//updateCMDvalue(atoi(farmno),icno,eqType,eqIdNum,(char *)key2,json_object_get_string(picval2)) ; 
						//	else
						updateCMD(atoi(farmno),icno,eqType,eqIdNum,(char *)key2,json_object_get_string(picval2)) ; 
						
						if( equno[0] == 'B' || equno[0] == 'C') // 통합제어기에 연결된 장비 처리
						{

						} else { // 소켓에 연결된 장비 응답 처리  디비에 업데이트 

						}

					} else {


					}
				
				}	

			}
		   
		  //cmd_array_obj_name = json_object_object_get(cmd_array_obj, "name");
		  //printf("name=%s\n", json_object_get_string(medi_array_obj_name));
		}		
		json_object_object_add(respacketobj, "cmdString", arrayobj);


	} else { // 단일 명령 처리 
	
		json_object *jstrcmdstr = json_object_new_string(ctrlcmd) ;
		json_object_object_add(respacketobj, "cmdString",jstrcmdstr );	

	}
	

	/*memset( GLB_postbuf,0x00,2048 ) ; 
	memcpy( GLB_postbuf,json_object_to_json_string(respacketobj),(int)strlen(json_object_to_json_string(respacketobj)) );

	printf("ICT direct post=%s\n", GLB_postbuf);				

	// 통합제어기 연결장비만 직업 응답
	
	if( devMode ) PostJSON("http://iot.pigplanxe.co.kr/iotRcvApi.json",GLB_postbuf); 
	*/
	//json_object_put(cmdlistobj);	
	json_object_put(respacketobj);
		
//{ "type":"req", "farmNo" :"1387", "icNo" : "I1",  "cmdString":[{"eqId" : "C1","settemp":"0"},{"eqId" : "W1","wateramt_day":"0"}] }
//{ "type":"set", "farmNo" :"1387", "icNo" : "I1",  "cmdString":[{"eqId" : "C1","settemp":"27.1"}] }

	 
}


//////////////////////////////////////////////////////////////////////////////////
//
//  모바일서버로 부터 요청된 패킷 처리 
//
void JsonPaketParsing(char *jsonpaketstr)
{
	json_object *arrayobj;
	json_object *pkgobj,*cmdobj,*dval ;	
	json_object *respacketobj;
	json_object *cmdlistobj;

    json_object *cmd_array_obj, *cmd_array_obj_name;

	int cmdobjcount=0;
	int getsetval_now=0;

	int i,devMode,findEqno;
	int arraylen=0;
	char cmdmode=0;
	char devid[64] ;
	char eqType;
	char eqId[8];
	char eqIdBack[8];
	char eqIdNum[8];

	char *farmno,*icno,*equno,*timestr,*ctrlcmd ;
	farmno=icno=equno=timestr=NULL;

	int typeEcnt=0;
	//system("free");
	//printf("JsonPaketParsing()=%s\n", jsonpaketstr);	
	memset( eqIdNum,0x00,8 ) ;


	devMode=0;
	pkgobj = json_tokener_parse(jsonpaketstr); 

	printf("Server Packet access ......................................\n" ) ; 
	dval = json_object_object_get(pkgobj, "type");
	if( dval != NULL )
	{
		printf("type:%s\n",json_object_get_string(dval) ) ;  
		if( strcmp("req",json_object_get_string(dval)) == 0 )
		{
			cmdmode=1;
			printf("req command\n" ) ; 
		}
		if( strcmp("res",json_object_get_string(dval)) == 0 )
		{
			cmdmode=2;
			printf("res command\n" ) ; 
		}
		if( strcmp("set",json_object_get_string(dval)) == 0 )
		{
			cmdmode=3;
			printf("set command\n" ) ; 
		}
		if( strcmp("reset",json_object_get_string(dval)) == 0 )
		{
			cmdmode=4;
			printf("reset command  system reboot\n" ) ; 
		}
	}

	if( cmdmode == 4 )
	{
		system("reboot");
	}
	

	dval = json_object_object_get(pkgobj, "farmNo");
	if( dval != NULL ) farmno = json_object_get_string(dval);
	else farmno="error";
	printf("farmNo:%s\n",farmno ) ;  
	
	dval = json_object_object_get(pkgobj, "icNo");
	if( dval != NULL ) icno = json_object_get_string(dval);
	else icno="error";
	printf("icNo:%s\n",icno ) ;  

	// ======================================================================================					
	if( cmdmode == 3 ) // SET packet 은 device 에 전달
	{
		send_msg( (char *)jsonpaketstr, strlen(jsonpaketstr) ) ;	
 	} 
	// ======================================================================================					
	ctrlcmd=NULL;

    // ======================================================================================					
	// 485포트,232포트 연결되어지 장비의 경우 SET 명령을 소켓으로 보내지 않코 직접 아래에서 찾아 처리한다. 
	// ======================================================================================					
	dval = json_object_object_get(pkgobj, "cmdString");
	if( dval != NULL ) ctrlcmd  = json_object_get_string(dval);		
	
	if( ctrlcmd != NULL && ctrlcmd[0] != '[')
	{
		printf("ctrl cmdString:%s\n",ctrlcmd ) ; 
		cmdobj = NULL ;

	} else { 

		if( ctrlcmd != NULL )
		{
			printf("array cmdString:%s\n",ctrlcmd ) ; 
			cmdobj = json_object_object_get(pkgobj, "cmdString");
		} else { 
			printf("not array cmdString:%s\n",ctrlcmd ) ; 
			cmdobj = NULL ;
			return ;
		}
		
	}
					
	respacketobj = json_object_new_object();
	

	json_object *jstrtype = json_object_new_string("res") ;
	json_object_object_add(respacketobj, "type",jstrtype  );
	
	

	json_object *jstrfarmno = json_object_new_string(farmno) ;
	json_object_object_add(respacketobj, "farmNo",jstrfarmno);
	
	json_object *jstricno = json_object_new_string(icno) ;
	json_object_object_add(respacketobj, "icNo",jstricno);
	
	//json_object *jstrequno = json_object_new_string(equno) ;
	//json_object_object_add(respacketobj, "eqId",jstrequno);		
	
	//json_object *jstrtime = json_object_new_string(timestr) ;
	//json_object_object_add(respacketobj, "regTm",jstrtime);
    memset( eqIdBack,0x00,8 ) ;
    
    arrayobj = json_object_new_array();	
	
	if( cmdobj != NULL) // 다중 명령 처리
	{
		
		arraylen = json_object_array_length(cmdobj);
		
		for (i = 0; i < arraylen; i++) {
		
			
			
			cmdlistobj = json_object_new_object();		
			cmd_array_obj = json_object_array_get_idx(cmdobj, i);
			findEqno=0;
			json_object_object_foreach(cmd_array_obj, key, picval) 
			{	
				memset(ICTretval,0x00,64);
				
				if( !strcmp((char *)key,"eqId") )
				{
					memset(devid,0x00,64 ) ; 
					equno = json_object_get_string(picval) ;
					findEqno=1;
					eqType=equno[0];
					
					memset( eqId,0x00,8 ) ;
					
					memcpy( eqId,&equno[1],strlen(equno)-1) ;
					
					if( strcmp(eqIdBack,eqId) )
					{
						memset( eqIdBack,0x00,8 ) ;
						memcpy( eqIdBack,&equno[1],strlen(equno)-1) ;
						typeEcnt=0;
					}
					//sqlpf(devid,(char*)"eqType:%c eqId:%s",eqType,eqId )  ;
					//printf("eqID:%s\n",devid ) ; 			
					
				
				} 

				json_object *jstrcmdstr = json_object_new_string(ICTretval) ;
				json_object_object_add(cmdlistobj, (char *)key,jstrcmdstr );				
				//printf("%s %s\n",(char *)key,ICTretval); 
			
			}	
	    
			json_object_array_add(arrayobj, cmdlistobj);
			if( findEqno  )
			{
				if( eqType == 'E' ) 
				{
					sprintf(eqIdNum, "%s-%d",eqId,typeEcnt);
					typeEcnt++;
				} else sprintf(eqIdNum, "%s",eqId);
				
				json_object_object_foreach(cmd_array_obj, key2, picval2) 
				{	
					memset(ICTretval,0x00,64);
					
					
					if( strcmp((char *)key2,"eqId") ) // eqID 가 아니값에 대한  디비 업데이트 
					{
						//void updateCMD( int farmNo,char *icID,char devType,char *devID,char *column_nm,char *setvalue)
						//farmno,icno 
						printf("SET %c%s: %s->%s\n",eqType,eqIdNum,(char *)key2,json_object_get_string(picval2)); 
						
						//if( eqType=='E' ) // 군사급이기의 경우 pigmnum 이 여러개임으로 따로 처리함
						//updateCMDvalue(atoi(farmno),icno,eqType,eqIdNum,(char *)key2,json_object_get_string(picval2)) ; 
						//	else
						updateCMD(atoi(farmno),icno,eqType,eqIdNum,(char *)key2,json_object_get_string(picval2)) ; 
						
						if( equno[0] == 'C' ) // 통합제어기에 연결된 장비 처리 B 정전화재, C 냉난방기 
						{

							char setvalstr[8]; memset( setvalstr,0x00,8) ;
							
							int strendpos = strlen(json_object_get_string(picval2)) ;

							if( strchr(json_object_get_string(picval2),'.') == NULL ) 
							{
								 
								memcpy( setvalstr,json_object_get_string(picval2), strendpos-1 ) ;
								setvalstr[strendpos]='0' ;
		

							} else {

								memcpy( setvalstr,json_object_get_string(picval2), strendpos ) ;
								delchar(setvalstr, '.') ;
							}
							
							printf("eqId : %d setvalstr = %d \n",atoi(eqIdNum),atoi(setvalstr)); // LIM
							
							// 485에 연결된 장비는 여기서 설정명령을 내리고, 바로 설정 응답을 받아 기록한다. 
							getsetval_now = modbus_writeread(atoi(eqIdNum),211,atoi(setvalstr),1) ;
							
							memset( setvalstr,0x00,8) ;
							sqlpf( setvalstr,(char*)"%02d.%01d", getsetval_now/10,getsetval_now%10)  ;
							printf("eqId : %d readset = %s \n",atoi(eqIdNum),setvalstr) ; 
											
						
						} else { // 

						}

					} else {


					}
				
				}	

			}
		   
		  //cmd_array_obj_name = json_object_object_get(cmd_array_obj, "name");
		  //printf("name=%s\n", json_object_get_string(medi_array_obj_name));
		}		
		json_object_object_add(respacketobj, "cmdString", arrayobj);


	} else { // 단일 명령 처리 
	
		json_object *jstrcmdstr = json_object_new_string(ctrlcmd) ;
		json_object_object_add(respacketobj, "cmdString",jstrcmdstr );	

	}
	

	/*memset( GLB_postbuf,0x00,2048 ) ; 
	memcpy( GLB_postbuf,json_object_to_json_string(respacketobj),(int)strlen(json_object_to_json_string(respacketobj)) );

	printf("ICT direct post=%s\n", GLB_postbuf);				

	// 통합제어기 연결장비만 직업 응답
	
	if( devMode ) PostJSON("http://iot.pigplanxe.co.kr/iotRcvApi.json",GLB_postbuf); 
	*/
	//json_object_put(cmdlistobj);	
	json_object_put(respacketobj);
				
	 
}


int paketParsing( int socktype,char *jsonpaketstr)
{
	
	//  socktype = 1  연결장비  socktype =5  서버
	//	socktype      cmdmode
	//  5				1,3       서버로부터의 전송 요청,셋팅
	//  1               2         기기로부터의 응답 -> 응답값 디비 기록 -> 서버로 전송  
	
	if( socktype == 5 ) // web api request and set command access 
	{

	  
	  printf("MSERVER send to device -----------------------------------------------\n") ; 		
	  JsonPaketParsing(jsonpaketstr);	
	  //send_msg( (char *)jsonpaketstr, strlen(jsonpaketstr) ) ;	

	} else {

	//{ "type":"req", "farmNo" :"1387", "icNo" : "I1", "eqId" : "W1", "cmdString":{"wateramt_day":"0","wateramt_mon":"0"}, "regTm" : "20190417_182323" }
	printf("DEVICE send to mserver -----------------------------------------------\n") ; 		

	JsonPaketParsingDevice(jsonpaketstr); // 각장치의 응답 패킷을 해석하여 디비에 업데이트한다. 
	//PostJSON("http://iot.pigplanxe.co.kr/iotRcvApi.json",jsonpaketstr); 


	}
	return 1 ;	
}

int main(int argc, char **argv)
{
	bool rtn;
	int tmKey ; 
	int idx;
    int c ;
	

    while ((c = getopt (argc, argv, "c:dv:")) != -1) {
        printf("option %c\n ",c ) ; 
		switch (c) {
            case 'd':
                daemonize ();
                break;
            default:
                break;
        }
    }

	//////////////////////////////////////////////////////////////////////////
	//
	// 변수 초기화 
	//
	pthread_mutex_init(&mutex_cmd, NULL);

  


	int isOK = 0;
	int retry_count=0 ;
	
	DefTypeUART2CAN cmdpacket;
	DefJSONPACKET tcpcmdpacket ;

	for( c= 0 ; c < CurNodeCount ;c++)
	{

		NodeInfo[c].isConnected =0;
		NodeInfo[c].socketnum =-1;
		NodeInfo[c].code =-1;
		NodeInfo[c].status =0;

	}

	//////////////////////////////////////////////////////////////////////////
	//
	// 환기제어기 접속 정보 DB 에서 취득
	//
	//PacketVariable_init() ; 

	//ReadDevice_Config() ;
	//ReadServer_Config() ;	
	//ReadNodeInformationFromDB();

	
	//timer_set(&tryconnect_timer, CLOCK_SECOND * 20);

	GetICTInfo() ;
	GetICTDeviceInfo(1); 
	ICTDEVICE_COUNT=0;
	if( ICTDevInfo.size() > 0 ) ICTDEVICE_COUNT=1;

	for( c = 0 ; c < ICTDevInfo.size() ;c++)
	{
		
		DefCycleDevice &getDev = ICTDevInfo.at(c) ;
		if( getDev.enable == 'Y' &&  getDev.setcycle > 0 ) { getDev.curcycle = getDev.setcycle ;  getDev.started=0; }
		printf("FARMNO:%d  DEV: %s,%c,%s,%c,%d,%d\n",getDev.farmNo,getDev.icNo,getDev.devType,getDev.devId,getDev.enable,getDev.curcycle,getDev.setcycle ) ;

		//GetICTDeviceCMD(getDev.devType);
		//updateCMDLIST( getDev.farmNo,getDev.icNo,getDev.devType,getDev.devId) ;
	}
	
	//////////////////////////////////////////////////////////////////////////
	//
	// server 접속 시도 
	//
	printf("======================================\n") ;
	tcpman.InitServer(5100) ;
	pthread_create(&t_idTCP, NULL, TCPSERVER_Thread, NULL) ;
	pthread_detach(t_idTCP) ;
	printf("======================================\n") ;	

	printf("======================================\n") ;
	printf("ICT controller server 2019-12-27 ver1.5 \n" ) ;
	printf("======================================\n") ;
	
	sleep(1) ;
	printf("stand by ................\n") ;
	
 
   timer_t timerID;
    
    // 타이머를 만든다
    // 매개변수 1 : 타이머 변수
    // 매개변수 2 : second
    // 매개변수 3 : ms
    createTimer(&timerID,60, 0);
	//dbtest() ;
	

	//////////////////////////////////////////////////////////////////////////
	//
	// 메인 LOOP 
	//

	BYTE pTYPE=0xFF ;

	printf("Start Main file get Loop ................\n") ;
	while(1)
	{
		///////////////////////////////////////////////////////////////////////////////////////
		//
		// received tcp packet access
		//
		
		if(  TCPP_RxQuePop( &tcpcmdpacket) ) 
		{
			pthread_mutex_lock(&cmd_mutex);
  			printf("POP Jsonpacket ...................................................\n") ;
			//HexaPrint( (unsigned char *)&tcpcmdpacket.nData,1024) ;      
						
			printf("Type[%d] %s \n",tcpcmdpacket.type,tcpcmdpacket.nData ) ;
			


			pthread_mutex_unlock(&cmd_mutex);

			paketParsing(tcpcmdpacket.type,tcpcmdpacket.nData );
			//PostJSON("http://iot.pigplanxe.co.kr/iotRcvApi.json",GLB_postbuf); 
		

		}  

		usleep(20000) ;
	} // while 
	
	tcpman.CloseServer() ;	
	//destroy_timer(); 


	printf("Program Exit \n") ;
	
}




