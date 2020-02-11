
#include "common.h"
#include "TTA12_packet.h"
#include "util.h"



HWCONFIG SENSOR_MAP[MAX_SENSOR_COUNT] ;
HWCONFIG ACTUATOR_MAP[MAX_ACTUATOR_COUNT] ;

int  SensorInterval =60 ;
int  ActuatorInterval=3 ;
int  ISOInterval=3;

int CONTROL_PORT=1 ;  //노드 제어용 시리얼 포트     
int PORT_SPEED=115200 ;  //노드 제어용 시리얼 포트 스피드   
int HouseControl_ID = 0 ;

char GCG_IP[128] ;
int GCG_PORT=5000 ;

// 2016년 6월 추가
int INSTALL_COMM_SENSOR_COUNT=0;
COMM_SENSOR_STRUCT COMM_SENSOR[MAX_COMM_SENSOR] ;


void PacketVariable_init(void)
{
	InstalledSensorCount=24;                  
	InstalledActuatorCount=8;
	InstalledSensorNodeCount=3;
	InstalledActuatorNodeCount=1;

	for( int i=0; i < MAX_SNODE_COUNT ;i++)
	{
		SensorNodeStatus[i].SW_VER =0x01 ;
		SensorNodeStatus[i].PROFILE_VER=0x01 ;
		SensorNodeStatus[i].CONTROL_ID=0x00 ;
		SensorNodeStatus[i].NODE_ID=i;
		SensorNodeStatus[i].ISINIT_NODE=0;
		SensorNodeStatus[i].MONITOR_MODE=0;
		SensorNodeStatus[i].MONITOR_TIME=2;
		SensorNodeStatus[i].SENACT_NUM = 8 ;
		SensorNodeStatus[i].COMM_ERROR_NUM=0;
		SensorNodeStatus[i].SERVICE_ERROR_NUM=0;
	
	}
	for( int i=0; i < MAX_ANODE_COUNT ;i++)
	{
		AcutuatorNodeStatus[i].SW_VER =0x01 ;
		AcutuatorNodeStatus[i].PROFILE_VER=0x01 ;
		AcutuatorNodeStatus[i].CONTROL_ID=0x00 ;
		AcutuatorNodeStatus[i].NODE_ID=i;
		AcutuatorNodeStatus[i].ISINIT_NODE=0;
		AcutuatorNodeStatus[i].MONITOR_MODE=0;
		AcutuatorNodeStatus[i].MONITOR_TIME=2;
		AcutuatorNodeStatus[i].SENACT_NUM = 8 ;
		AcutuatorNodeStatus[i].COMM_ERROR_NUM=0;
		AcutuatorNodeStatus[i].SERVICE_ERROR_NUM=0;
	
	}

	for( int i=0; i < MAX_SENSOR_COUNT ;i++)
	{
		SI[i].NODE_ID =i/8 ;
		SI[i].data.id =i ;
		SI[i].data.type =0x00 ;
		SI[i].data.value =i;
		SI[i].state=0;
		SI[i].isinit=1;
		SI[i].mode=REQCMD_PASSIVE_MODE ;//REQCMD_ACTIVE_MODE;
		SI[i].time=SensorInterval;
	
	}
	for( int i=0; i < MAX_ACTUATOR_COUNT ;i++)
	{
		AI[i].NODE_ID =i/8 ;
		AI[i].data.id =i ;
		AI[i].data.type =0x00 ;
		AI[i].data.value =0;
		AI[i].state=0;
		AI[i].isinit=1;
		AI[i].mode=0;
		AI[i].time=0;
	
	}

}

int ReadDevice_Config(void) 
{

    return 0 ;

}


int ReadServer_Config(void) 
{
    return 0 ;
}


int GetSensorConfigIndex( char getID )
{
		int b;
		for( b = 0 ; b < InstalledSensorNodeCount*8 ; b++)
		{
			if( SENSOR_MAP[b].ID == getID ) return b ;
		}
		printf( "Not find sensor index (config) : id %d \n",getID ) ; 
		return -1; 
}

int GetActuatorConfigIndex( char getID ,BYTE arg )
{
		int b;
		for( b = 0 ; b < InstalledActuatorNodeCount*8 ; b++)
		{
			if( ACTUATOR_MAP[b].ID == getID && ACTUATOR_MAP[b].ARG == arg ) return b ;
		}
		printf( "Not find Actuator index (config) : id %d: arg %d \n",getID,arg ) ; 
		return -1; 
}

void Status_Broadcast(const char *fmt, ...)
{

		char printf_buf[1024];
    va_list args;
  
    va_start(args, fmt);
    vsprintf(printf_buf, fmt, args);
    va_end(args);
 
    CPacket tCMD ;
    tCMD.Add( (BYTE *)printf_buf , strlen( printf_buf ) ) ; 
    //tCMD.AddCRC() ;tCMD.Add(0x0D) ;tCMD.Add(0x0A) ;
 	// Not used	 by chj
	// send_msg( (char *)tCMD.GetBYTE(),tCMD.Getsize() ) ;
	  
}


/*
void Packet_Make102(const char *fmt, ...)
{

		char printf_buf[1024];
    va_list args;
  
    va_start(args, fmt);
    vsprintf(printf_buf, fmt, args);
    va_end(args);
 
    make_packet.Clear() ;
    make_packet.Add( (BYTE *)printf_buf , strlen( printf_buf ) ) ; 
    make_packet.AddCRC() ;make_packet.Add(0x0D) ;make_packet.Add(0x0A) ;
	
		pthread_mutex_lock(&mutex_cmd);
		CMD_BUF[CMD_TAIL].rx_index = 102 ;
		CMD_BUF[CMD_TAIL].size = make_packet.Getsize() ;
		memset( CMD_BUF[CMD_TAIL].buf , 0x00 , 128 ) ; 
		memcpy( CMD_BUF[CMD_TAIL].buf , make_packet.GetBYTE() , make_packet.Getsize() ) ; 
						
		if( CMD_TAIL < 255 ) CMD_TAIL++ ; 
		pthread_mutex_unlock(&mutex_cmd);    

}
*/

void HexaPrint( BYTE *Str,int size )
{
	int crcnt=0;
	int i = 0 ;
	for( i=0; i< size ;i++)
	{
		printf("%02X ", Str[i]);
		if( crcnt++> 8) { printf("\n") ; crcnt=0; }
	}

	printf("\n") ; 

}



#ifndef _MSC_VER


//===================================================
// Constant
//===================================================
#define MY_TIMER_SIGNAL  SIGRTMIN
#define ONE_MSEC_TO_NSEC 1000000
#define ONE_SEC_TO_NSEC  1000000000

bool TimerMgr::setSignal()
{

 int rtn;
 struct sigaction sa;

 sa.sa_flags  = SA_SIGINFO;
 sa.sa_sigaction = timer_handler;

 sigemptyset(&sa.sa_mask);
 rtn = sigaction(MY_TIMER_SIGNAL, &sa, NULL);
 if (rtn == -1)
 {
  errReturn("sigaction");
 }
 return true;

}

bool TimerMgr::setTimer(int *key, long intv, TimerHandler tmFn, void *userPtr)
{
 int rtn;
 timer_t timerId;
 struct sigevent sigEvt;

 if (m_TimerNum >= MAX_TIMER_NUM)
 {
  printf("Too many timer\n");
  return false;
 }
 *key = findNewKey();
 /* Create Timer */
 memset(&sigEvt, 0x00, sizeof(sigEvt));
 sigEvt.sigev_notify    = SIGEV_SIGNAL;
 sigEvt.sigev_signo    = MY_TIMER_SIGNAL;
 sigEvt.sigev_value.sival_int = *key;
 rtn = timer_create(CLOCK_REALTIME, &sigEvt, &timerId);
 if (rtn != 0)
 {
  errReturn("timer_create");
 }
 /* Set Timer Interval */
 long nano_intv;
 struct itimerspec its;
 nano_intv = intv * ONE_MSEC_TO_NSEC;
 // initial expiration
 its.it_value.tv_sec  = nano_intv / ONE_SEC_TO_NSEC;
 its.it_value.tv_nsec  = nano_intv % ONE_SEC_TO_NSEC;
 // timer interval
 its.it_interval.tv_sec = its.it_value.tv_sec;
 its.it_interval.tv_nsec = its.it_value.tv_nsec;

 rtn = timer_settime(timerId, 0, &its, NULL);

 if (rtn != 0)
 {
  errReturn("timer_settimer");
 }
 /* Save Timer Inforamtion */
 TimerInfo tm;
 tm.m_TmId   = timerId;
 tm.m_TmFn  = tmFn;
 tm.m_UserPtr  = userPtr;

 m_TmInfoArr[*key] = tm;
 m_TmInfoArr[*key].m_IsUsed  = true;
 m_TimerNum++;

 return true;

}

bool TimerMgr::delTimer(int key)
{
 int rtn;
 TimerInfo tm;

 if (m_TimerNum <= 0)
 {
  printf("Timer not exist\n");
  return false;
 }

 if (m_TmInfoArr[key].m_IsUsed == true)
 {
  tm = m_TmInfoArr[key];
 } else {
  printf("Timer key(%d) not used\n", key);
  return false;
 }
 rtn = timer_delete(tm.m_TmId);
 if (rtn != 0)
 {
  errReturn("timer_delete");
 }
 m_TmInfoArr[key].m_IsUsed = false;
 m_TimerNum--;

 return true;

}

void TimerMgr::callback(int key)
{
 TimerInfo tm;
 tm = m_TmInfoArr[key];
 m_IsCallbacking = true;
 (tm.m_TmFn)(key, tm.m_UserPtr);
 m_IsCallbacking = false;
}

bool TimerMgr::isCallbacking()
{
 return m_IsCallbacking;
}

void TimerMgr::clear()
{
 for (int i = 0; i < MAX_TIMER_NUM; i++)
 {
  if (m_TmInfoArr[i].m_IsUsed == true)
   delTimer(i);
 }
 m_TimerNum  = 0;
}

int TimerMgr::findNewKey()
{
 for (int i = 0; i < MAX_TIMER_NUM; i++)
 {
  if (m_TmInfoArr[i].m_IsUsed == false)
   return i;
 }
}

//===================================================
// USER API
//===================================================

TimerMgr *g_TmMgr = 0;

bool create_timer()
{
 if (g_TmMgr)
 {
  printf("global timer handler exist aleady.\n");
  return false;
 }
 g_TmMgr = new TimerMgr();
 if ( !g_TmMgr)
 {
  printf("create_timer() fail\n");
  return false;
 }
 return g_TmMgr->setSignal();
}

bool set_timer(int *key, long resMSec, TimerHandler tmFn, void *userPtr)
{

 CHECK_NULL(key, false);
 CHECK_NULL(tmFn, false);
 bool rtn;
 rtn = g_TmMgr->setTimer(key, resMSec, tmFn, userPtr);
 if (rtn != true)
 {
  printf("set_timer(%p,%lu,%p,%p) fail\n", 
    key, resMSec, tmFn, userPtr);
  return false;
 }
 return true;
}

bool delete_timer(int key)
{
 bool rtn;
 rtn = g_TmMgr->delTimer(key);

 if (rtn != true)
 {
  printf("delete_timer(%d) fail\n", key);
  return false;
 }
 return true;

}

bool destroy_timer()
{
 CHECK_NULL(g_TmMgr, false);
 if (g_TmMgr->isCallbacking() == true)
 {
  printf("Can't destroy timer in callback function.\n");
  return false;
 }
 g_TmMgr->clear();
 delete g_TmMgr;
 g_TmMgr = 0;
 return true;
}

//===================================================
// Timer Handler 
//===================================================

static void timer_handler(int sig, siginfo_t *si, void *context)
{
 int key = si->si_value.sival_int;
 g_TmMgr->callback(key);
}

#endif