
#include "TTA12_define.h"
#include "Packet.h"

#ifndef __TTA12_PACKETMAKE_DEFINE__
#define __TTA12_PACKETMAKE_DEFINE__

extern "C" unsigned short SequenceCount ;

class TTA12Packet
{
public:
	FrameControlField FCF;
	PayloadField PLF;
	WORD SeqNumber;
	WORD CONTROL_ID ; //24bit
	int  NODE_ID ;  //24bit
	BYTE RequestType ;
	BYTE RequestCommand;

	BYTE VALUE_COUNT ;
	CPacket packet;
	BYTE data;
	BYTE buff[512];
/*
	SensorValue *SetSensor ;
	ActuatorValue *SetActuator ;
*/
public :
	
	
	
	TTA12Packet() ;
	~TTA12Packet() ; 

	void Clear(void) { packet.Clear();FCF.reserved =0; }
	CPacket* GetPacket(void) { return &packet; }
	
	void SetFrameDevice( BYTE val ) ; 
	void SetFrameType( BYTE val ) ; 
	void SetSecurity( BYTE val ) ;  
	void SetAckConfirm( BYTE val ) ;  

	void SetControlID( WORD val ) ;  
	void SetNodeID( WORD val ) ;  

	void SetPayloadType( BYTE val ) ; 
	void SetPayloadLength( BYTE val ) ; 

	void SetRequestType( BYTE val ) ; 

	void SetRequestValueCount( BYTE val ) {  VALUE_COUNT=val; }

	void AddSensorPayload(SensorValue data) ;
	void AddActuatorPayload(ActuatorValue data) ; 
	int AddSensorNodeStatus( int id) ; 
	int AddActuatorNodeStatus( int id) ; 


	void MakeRequest(void) ;
	void MakeReponse(void) ;
	void MakeConfirm(void) ;
	void MakeNotify(void) ;
	void MakeAck(void) ;
	
	void MakeSensorData(BYTE type,int count,SensorValue *data) ;
	void MakeActuatorData(BYTE type,int count,ActuatorValue *data) ;
	
	void MakeHeader(void) ;
	void MakePayload(void) ;


};

class TTA12PacketParsing
{
public:
	FrameControlField FCF;
	PayloadField PLF;
	WORD SeqNumber;
	WORD CONTROL_ID ; //24bit
	WORD NODE_ID ;  //24bit
	BYTE RequestType ;
	BYTE RequestCommand;

	CPacket packet;
	BYTE data;
	BYTE buff[512];
	
	TTA12Packet TTAPacket;

	SensorInfo siData[8] ;
  ActuatorInfo aiData[8] ; 
	
	SensorValue sData[8];
	ActuatorValue aData[8];	
  char aDataARG[8] ;
public :
		
	TTA12PacketParsing(void ) ;
	~TTA12PacketParsing() ; 
	
	void Clear(void) { 
		FCF.reserved =0;
		packet.Clear() ; TTAPacket.packet.Clear() ; 
	}
	CPacket* GetPacket(void) { return &packet; }
	CPacket* GetReturnPacket(void) { return &TTAPacket.packet; }
	
	
	SensorInfo GetSensorInfo( WORD node,WORD sensor_id) ;
	ActuatorInfo GetActuatorInfo( WORD node,WORD actuator_id ) ;
	
	int GetSensorData( SensorValue *gsData ) ; 
	int GetActuatorData(ActuatorValue *gaData ) ; 

	void TTA12PacketGetSTATE1(void) ;
	void TTA12PacketGetSTATE2(void) ;
	void TTA12PacketGetSTATE3(void) ;
	
	void ReturnMakeReponse(void) ;
	void ReturnMakeConfirm(void) ;
	void ReturnMakeNotify(void) ;
	//void ReturnMakeData(void) ;
	void ReturnMakeAck(void) ;

	void ActuatorSet(void) ;
	void SensorModeSet(void) ; 

	void ParsingRequest(void) ;
	void ParsingResponse(void) ;
	void ParsingConfirm(void) ;
	void ParsingData(void) ;
	void ParsingNotify(void) ;

	BYTE ParsingPacket(void) ;
};

extern void PROFILE_SET(void) ;


/////////////////////////////
extern SensorInfo SI[MAX_SENSOR_COUNT] ; 
extern ActuatorInfo AI[MAX_ACTUATOR_COUNT] ;

extern BYTE InstalledSensorCount;                  
extern BYTE InstalledActuatorCount;
extern BYTE InstalledSensorNodeCount;
extern BYTE InstalledActuatorNodeCount;

extern BYTE SensorNodeErrorCode[MAX_SNODE_COUNT] ;
extern BYTE AcutuatorNodeErrorCode[MAX_ANODE_COUNT] ;

// 센서 및 액츄에이터 설정정보 
extern SensorValue  SensorSettingData[MAX_SENSOR_COUNT] ; 
extern ActuatorValue  ActuatorSettingData[MAX_ACTUATOR_COUNT] ;

// 현재 센싱값 및 액츄에이터 노드 상태값
extern SensorInfo  SI[MAX_SENSOR_COUNT] ; 
extern ActuatorInfo  AI[MAX_ACTUATOR_COUNT] ;
extern NODESTATUS  SensorNodeStatus[MAX_SNODE_COUNT] ;
extern NODESTATUS  AcutuatorNodeStatus[MAX_ANODE_COUNT] ;

// 제어장치 상태값
extern CONTROLLERSTATUS ControllerStatus;

#endif 
