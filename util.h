
#include <stdarg.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#ifndef _MSC_VER
	#include <linux/input.h>
	#include <unistd.h>
#endif
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>


#ifndef _MSC_VER
	#include "tcpman.h"
#endif
#include "Packet.h"
#include "TTA12_define.h"
extern "C" {
#include "iniparser.h"

}

#ifndef __UTIL_CUBIE__
#define __UTIL_CUBIE__


#define EXT_KEY1 1 
#define EXT_KEY2 139 
#define EXT_KEY3 114 
#define EXT_KEY4 115 

#define MAX_TIMER_NUM 1000

//===================================================

// M A C R O 

//===================================================

#define errReturn(msg) \
{ \
 perror(msg); \
 return false; \
}




#define CHECK_NULL(ptr, rtn) \
{ \
 if ( !(ptr)) { \
  printf("%s is NULL\n", #ptr); \
  return rtn; \
 } \
}


//===================================================

// Timer Manager Class

//===================================================


#ifndef _MSC_VER


static void timer_handler(int sig, siginfo_t *si, void *context);

typedef void (*TimerHandler)(int key, void *userPtr);


typedef struct {
 bool   m_IsUsed;
 timer_t   m_TmId;
 TimerHandler m_TmFn;
 void*   m_UserPtr;
} TimerInfo;


class TimerMgr {
 private:

  int   m_TimerNum;  
  TimerInfo m_TmInfoArr[MAX_TIMER_NUM];
  bool  m_IsCallbacking;

 private:

  int findNewKey();

 public:

  TimerMgr() { memset(this, 0x00, sizeof(TimerMgr)); }
  ~TimerMgr() {}

  bool setSignal();
  bool setTimer(int *key, long resMSec, TimerHandler tmFn, void *userPtr=0);
  bool delTimer(int key);

  bool isCallbacking();
  void callback(int key);
  void clear();

};


bool create_timer();
// [ param ]
// key: unique timer key, output parameter.
// intv: interval, millisecond.
// tmFn: the handler want to be called when timer expired.
// userPtr: user specific data (pointer). default is NULL. 
//   You can receive this data when TimerHandler be called. (see TimerHandler param)

bool set_timer(int *key /* out */, long intv, TimerHandler tmFn, void *userPtr=0);
bool delete_timer(int key);
bool destroy_timer();

#endif

void PacketVariable_init(void) ;

int GetSensorConfigIndex( char getID ) ;
int GetActuatorConfigIndex( char getID ,BYTE arg ) ;

int ReadDevice_Config(void) ;
int ReadServer_Config(void) ;
void Status_Broadcast(const char *fmt, ...) ;
void HexaPrint( BYTE *Str,int size ) ;

extern HWCONFIG SENSOR_MAP[MAX_SENSOR_COUNT] ;
extern HWCONFIG ACTUATOR_MAP[MAX_ACTUATOR_COUNT] ;

extern int CONTROL_PORT ;  //노드 제어용 시리얼 포트     
extern int PORT_SPEED ;  //노드 제어용 시리얼 포트 스피드   
extern int HouseControl_ID ; // 하우스 통합제어기 ID

extern int  SensorInterval ;
extern int  ActuatorInterval ;

extern char GCG_IP[128] ;
extern int GCG_PORT;


extern int INSTALL_COMM_SENSOR_COUNT;
extern COMM_SENSOR_STRUCT COMM_SENSOR[MAX_COMM_SENSOR] ;
#endif 