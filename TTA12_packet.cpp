
#include <stdarg.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#ifndef _MSC_VER
	#include <linux/input.h>
#endif
#include <string.h>
#include <stdlib.h>

#include "common.h"
#include "TTA12_packet.h"
#include "util.h"

unsigned short SequenceCount=0 ;

// 설치 현황
BYTE InstalledSensorCount=18;                  
BYTE InstalledActuatorCount=8;
BYTE InstalledSensorNodeCount=3;
BYTE InstalledActuatorNodeCount=1;

// 에러정보
BYTE SensorNodeErrorCode[MAX_SNODE_COUNT] ;
BYTE AcutuatorNodeErrorCode[MAX_ANODE_COUNT] ;

// 센서 및 액츄에이터 설정정보 
SensorValue  SensorSettingData[MAX_SENSOR_COUNT] ; 
ActuatorValue  ActuatorSettingData[MAX_ACTUATOR_COUNT] ;

// 현재 센싱값 및 액츄에이터 노드 상태값
SensorInfo  SI[MAX_SENSOR_COUNT] ; 
ActuatorInfo  AI[MAX_ACTUATOR_COUNT] ;
NODESTATUS  SensorNodeStatus[MAX_SNODE_COUNT] ;
NODESTATUS  AcutuatorNodeStatus[MAX_ANODE_COUNT] ;

// 제어장치 상태값
CONTROLLERSTATUS ControllerStatus;


BYTE TransferDataCount = 1 ; 


TTA12Packet::TTA12Packet() 
{
	FCF.device=0x00;
	FCF.type=0x00 ; 
	FCF.security=0x00;
	FCF.ack_confirm=0x00;
	
	PLF.type=0x00;
	PLF.length=0x00 ; 

	CONTROL_ID=0x00 ; //24bit
	NODE_ID=0x00 ;  //24bit
	RequestType=FTYPE_REQUEST ;
}

TTA12Packet::~TTA12Packet()
{

}

void TTA12Packet::SetFrameDevice( BYTE val )
{
	FCF.device = val ;
}
void TTA12Packet::SetFrameType( BYTE val )
{
	FCF.type = val ;
}
void TTA12Packet::SetSecurity( BYTE val )
{
	FCF.security = val ;
}
void TTA12Packet::SetAckConfirm( BYTE val )
{
	FCF.ack_confirm=val ;
}

void TTA12Packet::SetControlID( WORD val ) 
{
	CONTROL_ID=val ;
}
void TTA12Packet::SetNodeID( WORD val )
{
	NODE_ID=val ;
}

void TTA12Packet::SetPayloadType( BYTE val )
{
	PLF.type=val;
}
void TTA12Packet::SetPayloadLength( BYTE val )
{
	PLF.length=val;
}

void TTA12Packet::SetRequestType( BYTE val )
{
	RequestType=val ;
}



void TTA12Packet::AddSensorPayload(SensorValue data)
{
	packet.Add( data.id ) ;	
	packet.Add( data.type ) ;
	packet.Add( (data.value&0x00ff)  ) ;
	packet.Add( (data.value&0xff00)>>8 ) ;
	

}

void TTA12Packet::AddActuatorPayload(ActuatorValue data)
{
	packet.Add( data.id ) ;	
	packet.Add( data.type ) ;
	packet.Add( (data.value&0x00ff)  ) ;	
	packet.Add( (data.value&0xff00)>>8 ) ;
	
}


int TTA12Packet::AddSensorNodeStatus( int id) 
{
	//packet.Add( SensorNodeStatus[id],sizeof( NodeStatus)) ; 
	int add_count = 0 ; //  추가된 패킷 길이 
	packet.Add( SensorNodeStatus[id].SW_VER ) ;
	packet.Add( SensorNodeStatus[id].PROFILE_VER ) ;
	packet.Add( (SensorNodeStatus[id].CONTROL_ID&0x00ff) ) ;
	packet.Add( (SensorNodeStatus[id].CONTROL_ID&0xff00)>>8 ) ;
	
	packet.Add( (SensorNodeStatus[id].NODE_ID&0x00ff) ) ;
	packet.Add( (SensorNodeStatus[id].NODE_ID&0xff00)>>8 ) ;
	
	
	packet.Add( SensorNodeStatus[id].ISINIT_NODE ) ;
	packet.Add( SensorNodeStatus[id].MONITOR_MODE  ) ;
	
	packet.Add( (SensorNodeStatus[id].MONITOR_TIME&0x00ff) ) ;
	packet.Add( (SensorNodeStatus[id].MONITOR_TIME&0xff00)>>8 ) ;
	

	packet.Add( SensorNodeStatus[id].SENACT_NUM ) ;
	add_count = 15 ;	
	
	for( int c=0; c  < MAX_SENSOR_COUNT ;c++)
	{
		if( SI[c].NODE_ID == id ) // 해당 노드 데이터 
		{
			AddSensorPayload( SI[c].data ) ; 			
			packet.Add( SI[c].state ) ;
			packet.Add( SI[c].isinit) ; 
			packet.Add( SI[c].mode ) ;
			packet.Add( SI[c].time ) ;
			add_count+=8;
		}
		
	}

	packet.Add( (SensorNodeStatus[id].COMM_ERROR_NUM&0x00ff) ) ;
	packet.Add( (SensorNodeStatus[id].COMM_ERROR_NUM&0xff00)>>8 ) ;
	
	packet.Add( (SensorNodeStatus[id].SERVICE_ERROR_NUM&0x00ff) ) ;
	packet.Add( (SensorNodeStatus[id].SERVICE_ERROR_NUM&0xff00)>>8 ) ;
	
		
	return add_count;
}

int TTA12Packet::AddActuatorNodeStatus( int id) 
{
	//packet.Add( SensorNodeStatus[id],sizeof( NodeStatus)) ; 
	int add_count = 0 ; //  추가된 패킷 길이 
	packet.Add( AcutuatorNodeStatus[id].SW_VER ) ;
	packet.Add( AcutuatorNodeStatus[id].PROFILE_VER ) ;
	
	packet.Add( (AcutuatorNodeStatus[id].CONTROL_ID&0x00ff) ) ;
	packet.Add( (AcutuatorNodeStatus[id].CONTROL_ID&0xff00)>>8 ) ;
	
	packet.Add( (AcutuatorNodeStatus[id].NODE_ID&0x00ff) ) ;
	packet.Add( (AcutuatorNodeStatus[id].NODE_ID&0xff00)>>8 ) ;
	
	
	packet.Add( AcutuatorNodeStatus[id].ISINIT_NODE ) ;
	packet.Add( AcutuatorNodeStatus[id].MONITOR_MODE  ) ;
	
	packet.Add( (AcutuatorNodeStatus[id].MONITOR_TIME&0x00ff) ) ;
	packet.Add( (AcutuatorNodeStatus[id].MONITOR_TIME&0xff00)>>8 ) ;
	

	packet.Add( AcutuatorNodeStatus[id].SENACT_NUM ) ;
	add_count = 15 ;

	for( int c=0; c  < MAX_ACTUATOR_COUNT ;c++)
	{
		if( AI[c].NODE_ID == id )
		{
			AddActuatorPayload(AI[c].data ) ; 
			
			packet.Add( AI[c].state ) ;
			packet.Add( AI[c].isinit ) ;
			packet.Add( AI[c].mode ) ;
			packet.Add( AI[c].time ) ;
			add_count+=8;
		}
		
	}
	
	packet.Add( (AcutuatorNodeStatus[id].COMM_ERROR_NUM&0x00ff) ) ;
	packet.Add( (AcutuatorNodeStatus[id].COMM_ERROR_NUM&0xff00)>>8 ) ;
	
	packet.Add( (AcutuatorNodeStatus[id].SERVICE_ERROR_NUM&0x00ff) ) ;
	packet.Add( (AcutuatorNodeStatus[id].SERVICE_ERROR_NUM&0xff00)>>8 ) ;
	
	return add_count;
}

void TTA12Packet::MakeRequest(void)
{
	data=0x00;
	switch( RequestType )
	{
		case REQCMD_INIT:  // 노드 및 센서 동시 초기화 
			SetPayloadType( PLTYPE_NODEINIT ) ; 
			SetPayloadLength( 1 ) ;
	
			packet.Add( PLF.type ) ;
			packet.Add( PLF.length ) ;
			break ; 

		case REQCMD_INIT_NODE: // 노드만 초기화 
			SetPayloadType( PLTYPE_NODEINIT ) ; 
			SetPayloadLength( 1 ) ;
			packet.Add( PLF.type ) ;
			packet.Add( PLF.length ) ;

			break ; 
		case REQCMD_INIT_ACT: 			
		//case REQCMD_INIT_SENSOR:
			SetPayloadType( PLTYPE_NODEINIT ) ; 
			SetPayloadLength( 1 ) ;
			packet.Add( PLF.type ) ;
			packet.Add( PLF.length ) ;
			break ; 
		case REQCMD_NODE_INFO:
			SetPayloadType( PLTYPE_NODEINFO ) ; 
			SetPayloadLength( 1 ) ;
			packet.Add( PLF.type ) ;
			packet.Add( PLF.length ) ;
			break ; 
	/*	
		case REQCMD_ACTUATOR_INFO:
		case REQCMD_SENSOR_INFO:
			SetPayloadType( PLTYPE_NODEINFO ) ; 
			SetPayloadLength( 1 ) ;
			packet.Add( PLF ) ;		
			break ; 		
	*/	
		
		// 셋팅 및 센싱값 요청 
		case REQCMD_ACTUATOR_SET:

		case REQCMD_PASSIVE_MODE:
		case REQCMD_ACTIVE_MODE:
		case REQCMD_EVENT_MODE:			
			
			return ; 

	
	}

	packet.Add( RequestType ) ;

}

void TTA12Packet::MakeReponse(void)
{
	BYTE PayloadLength=0;
	SetPayloadType( PLTYPE_NODEINFO ) ; 

	if( NODE_ID != NODE_ALL )  // 
	{	
		SetPayloadLength( 11 + SensorNodeStatus[NODE_ID].SENACT_NUM*8 ) ; // 한개의 노드상태 패킷 길이 
	} else {
		for( int c=0;c < InstalledSensorNodeCount ;c++)
		{
			PayloadLength += ( 11 + SensorNodeStatus[c].SENACT_NUM*8  ) ;
		}
		SetPayloadLength( PayloadLength ) ; 

	}

	packet.Add( PLF.type ) ;
	packet.Add( PLF.length ) ;
	

	if( FCF.device == FCF_SENSOR ) // sensor
	{
		if( NODE_ID != NODE_ALL )
		{
			AddSensorNodeStatus( NODE_ID) ;
		} else { // 전체 노드 상태 

			for( int c=0;c < InstalledSensorNodeCount ;c++)
			{
				AddSensorNodeStatus( c ) ;
			}
		}
	} else { //actuator
		if( NODE_ID != NODE_ALL )
		{
			AddActuatorNodeStatus( NODE_ID) ;
		} else { // 전체 노드 상태 

			for( int c=0;c < InstalledActuatorNodeCount ;c++)
			{
				AddActuatorNodeStatus( c ) ;
			}
		}
	}
	

}

void TTA12Packet::MakeConfirm(void)
{
	
	SetPayloadType( PLTYPE_NODEINFO ) ; 
	SetPayloadLength( sizeof(CONTROLLERSTATUS) ) ;
	//memcpy( &data,&PLF,sizeof(PayloadField) ) ; 			
	
	packet.Add( PLF.type ) ;		
	packet.Add( PLF.length ) ;		

	//packet.Add( (BYTE *)ControlStatus,sizeof(ControlStatus)) ; 
	packet.Add( ControllerStatus.SW_VER ) ;
	packet.Add( ControllerStatus.PROFILE_VER ) ;
	packet.Add( (ControllerStatus.CONTROL_ID&0x00ff) ) ;
	packet.Add( (ControllerStatus.CONTROL_ID&0xff00)>>8 ) ;
	
	
	packet.Add( ControllerStatus.ISINIT_COMM_NODE );
	packet.Add( ControllerStatus.ISINIT_SENACT );
	packet.Add( ControllerStatus.IS_ERROR );
	packet.Add( ControllerStatus.BUSY  );
	packet.Add( ControllerStatus.REQUEST_COMM  );
	
}

void TTA12Packet::MakeNotify(void) 
{
	
	SetPayloadType( PLTYPE_NODEINFO ) ; 
	SetPayloadLength( 1 ) ;
	packet.Add( PLF.type ) ;		
	packet.Add( PLF.length ) ;	

	if( FCF.device == FCF_SENSOR) // sensor
	{
		packet.Add( (BYTE)SensorNodeErrorCode[NODE_ID]) ; 
	} else { //actuator
		packet.Add( (BYTE)AcutuatorNodeErrorCode[NODE_ID] ) ; 
	}

}

void TTA12Packet::MakeAck(void)
{
	SetPayloadType( PLTYPE_NODEINFO ) ; 
	SetPayloadLength( 0 ) ;
	packet.Add( PLF.type ) ;		
	packet.Add( PLF.length ) ;	
}

void TTA12Packet::MakeSensorData(BYTE type,int count,SensorValue *sdata)
{
	
	SetPayloadType( type ) ; 
	SetPayloadLength( count ) ;
	packet.Add( PLF.type ) ;		
	packet.Add( PLF.length ) ;		
	
	if( FCF.type == FTYPE_REQUEST )  packet.Add( RequestType ) ;
	
	for( int i = 0 ; i< count;i++)
	{
		AddSensorPayload(sdata[i]) ; 
	}
}

void TTA12Packet::MakeActuatorData(BYTE type,int count,ActuatorValue *adata)
{
	
	SetPayloadType( type ) ; 
	SetPayloadLength( count ) ;
	packet.Add( PLF.type ) ;		
	packet.Add( PLF.length ) ;
	
	if( FCF.type == FTYPE_REQUEST )  packet.Add( RequestType ) ;
	
	for( int i = 0 ; i< count;i++)
	{
		AddActuatorPayload(adata[i]) ;
	}
}

void TTA12Packet::MakePayload(void)
{

	switch( FCF.type )
	{
		case FTYPE_REQUEST:			
			 MakeRequest() ; 	
			break ; 
		case FTYPE_RESPONSE:
			 MakeReponse() ; 
			break ; 
		case FTYPE_CONFIRM:
			 MakeConfirm() ;
			break ; 
		case FTYPE_NOTIFY:
			MakeNotify() ;
			break ; 
		case FTYPE_ACK:
			MakeAck() ;
			break ; 

		case FTYPE_DATA:
		
			break ; 

	
	}
	
	

}

void TTA12Packet::MakeHeader(void)
{
	// Maker header
	memcpy( buff,&FCF,sizeof(FCF) ) ; 
	packet.Add( buff,sizeof(FCF)  ) ;	
	
	packet.Add((SequenceCount&0x00ff)) ;
	packet.Add((SequenceCount&0xff00)>>8) ;
	
	packet.Add((CONTROL_ID&0x00ff)) ;
	packet.Add((CONTROL_ID&0xff00)>>8) ;
	
	packet.Add((NODE_ID&0x00FF)) ;
	packet.Add((NODE_ID&0xFF00)>>8) ;

}



TTA12PacketParsing::TTA12PacketParsing(void) 
{
	FCF.device=0x00;
	FCF.type=0x00 ; 
	FCF.security=0x00;
	FCF.ack_confirm=0x00;
	PLF.type=0x00;
	PLF.length=0x00 ; 

	CONTROL_ID=0x00 ; //16bit
	NODE_ID=0x00 ;  //16bit
	
}

TTA12PacketParsing::~TTA12PacketParsing()
{

}

SensorInfo TTA12PacketParsing::GetSensorInfo(  WORD node,WORD sensor_id) // sensor_id 0~7  노드당 최대 센서 8개 
{
	if( sensor_id < 8 )
		return SI[node*8+sensor_id] ; 
	else
		return SI[node] ; // default 0번 return by chj 
}

ActuatorInfo TTA12PacketParsing::GetActuatorInfo( WORD node,WORD actuator_id ) // actuator_id 0~7  노드당 최대 엑추에이터 8개 
{
	return AI[node*8+actuator_id] ; 
}


void TTA12PacketParsing::ReturnMakeReponse(void)
{
	TTAPacket.SetFrameDevice( FCF.device ) ;
	TTAPacket.SetFrameType( FTYPE_RESPONSE ) ;
	TTAPacket.SetSecurity( FCF.security ) ;
	TTAPacket.SetAckConfirm( FCF.ack_confirm ) ;
	
	TTAPacket.SetControlID( CONTROL_ID ) ;
	TTAPacket.SetNodeID( NODE_ID) ;
	
	SequenceCount++ ;	
	TTAPacket.MakeHeader() ; 
	TTAPacket.MakePayload() ;

}

void TTA12PacketParsing::ReturnMakeConfirm(void)
{
	TTAPacket.SetFrameDevice( FCF.device ) ;
	TTAPacket.SetFrameType( FTYPE_CONFIRM ) ;
	TTAPacket.SetSecurity( FCF.security ) ;
	TTAPacket.SetAckConfirm( FCF.ack_confirm ) ;
	
	TTAPacket.SetControlID( CONTROL_ID ) ;
	TTAPacket.SetNodeID( NODE_ID) ;
	SequenceCount++ ;	
	TTAPacket.MakeHeader() ; 
	TTAPacket.MakePayload() ;

}

void TTA12PacketParsing::ReturnMakeNotify(void)
{
	TTAPacket.SetFrameDevice( FCF.device ) ;
	TTAPacket.SetFrameType( FTYPE_NOTIFY ) ;
	TTAPacket.SetSecurity( FCF.security ) ;
	TTAPacket.SetAckConfirm( FCF.ack_confirm ) ;
	
	TTAPacket.SetControlID( CONTROL_ID ) ;
	TTAPacket.SetNodeID( NODE_ID) ;
	SequenceCount++ ;	
	TTAPacket.MakeHeader() ; 
	TTAPacket.MakePayload() ;

}
/*
void TTA12PacketParsing::ReturnMakeData(void)
{
	TTAPacket.SetFrameDevice( FCF.device ) ;
	TTAPacket.SetFrameType( FTYPE_DATA ) ;
	TTAPacket.SetSecurity( FCF.security ) ;
	TTAPacket.SetAckConfirm( FCF.ack_confirm ) ;
	
	TTAPacket.SetControlID( CONTROL_ID ) ;
	TTAPacket.SetNodeID( NODE_ID) ;
	SequenceCount++ ;	
	TTAPacket.MakeHeader() ; 
	TTAPacket.MakePayload() ;

}
*/
void TTA12PacketParsing::ReturnMakeAck(void)
{
	TTAPacket.SetFrameDevice( FCF.device ) ;
	TTAPacket.SetFrameType( FTYPE_ACK ) ;
	TTAPacket.SetSecurity( FCF.security ) ;
	TTAPacket.SetAckConfirm( FCF.ack_confirm ) ;
	
	TTAPacket.SetControlID( CONTROL_ID ) ;
	TTAPacket.SetNodeID( NODE_ID) ;
	SequenceCount++ ;	
	TTAPacket.MakeHeader() ; 
	TTAPacket.MakePayload() ;

}

void TTA12PacketParsing::ActuatorSet(void) 
{
	BYTE i,siSIZE;
	char nARG,chINDEX ;
	siSIZE = sizeof(ActuatorValue) ;	
	if( PLF.length > 0 )
	{ // 센싱값 획득 
		for( i=0; i < PLF.length ;i++)
		{
			aData[i].id =  packet.GetData(START_DATA_PACKET+(i*siSIZE)) ;
			aData[i].type =  packet.GetData(START_DATA_PACKET+(i*siSIZE+1)) ;
			aData[i].value =  packet.GetData(START_DATA_PACKET+(i*siSIZE+3)) ;			
			aData[i].value<<=8;
			aData[i].value |=  packet.GetData(START_DATA_PACKET+(i*siSIZE+2)) ;
		}	
		// 액츄에이터 버퍼에 기록 
		/*
		for( i=0; i< PLF.length;i++)
		{
			AI[NODE_ID*8+i].data=aData[i] ;
		}
		*/
		for( i=0; i< PLF.length;i++)
		{
			nARG=(aData[i].value&0xff00) >> 8 ; 
			printf( "TTA12PacketParsing::ActuatorSet  %d\n", PLF.length) ; 
			chINDEX = GetActuatorConfigIndex( aData[i].id ,nARG) ; 
			if( chINDEX != -1 )
			{
					//ACTUATOR_MAP[chINDEX].ARG  = nARG ;
					//printf( "packet data index :%d . arg : %X\n",chINDEX ,aData[i].value) ;
					//aData[i].value &= 0x00ff ;
					AI[chINDEX].data=aData[i] ;
					AI[chINDEX].data.value &=0x00ff ;

			}
				
		}	
	}

	
	 
}

void TTA12PacketParsing::SensorModeSet(void) 
{
	BYTE i,siSIZE;
	int chINDEX ;
	siSIZE = sizeof(SensorValue) ;	
	if( PLF.length > 0 )
	{ // 센서 세팅 모드 획득 
		for( i=0; i < PLF.length ;i++)
		{
			sData[i].id =  packet.GetData(START_DATA_PACKET+(i*siSIZE)) ;
			sData[i].type =  packet.GetData(START_DATA_PACKET+(i*siSIZE+1)) ;
			sData[i].value =  packet.GetData(START_DATA_PACKET+(i*siSIZE+3)) ;			
			sData[i].value<<=8;
			sData[i].value |=  packet.GetData(START_DATA_PACKET+(i*siSIZE+2)) ;
		}	
		// 노드 세팅 버퍼에 기록 
		for( i=0; i< PLF.length;i++)
		{			
			
			chINDEX = GetSensorConfigIndex( sData[i].id ) ; 
			SensorSettingData[chINDEX]= sData[i] ;
			SI[chINDEX].NODE_ID = NODE_ID ;
			SI[chINDEX].data.id = sData[i].id ; 
			SI[chINDEX].mode = RequestCommand ; //sData[i].type ;
			SI[chINDEX].time = sData[i].value | sData[i].type  ; // value 16bit    time 은 8bit TTA12 수정 필요
		}	
	
	} 
}

void TTA12PacketParsing::ParsingRequest(void)
{
	RequestCommand = packet.GetData(9) ;// Request Command type 
		
	switch( RequestCommand )
	{
		case REQCMD_INIT :
			//노드 및 센서초기화
			ReturnMakeReponse();
			break ;
		
		case REQCMD_INIT_NODE :
			//노드 초기화
			ReturnMakeReponse();
			break;
		
		case REQCMD_INIT_ACT :
			//센서 및 엑츄에이터 초기화 
			ReturnMakeReponse();
			break ; 

		case REQCMD_NODE_INFO :
			//노드 센서 및 엑츄에이터 노드 정보
			ReturnMakeReponse();
			break ; 
		
		case REQCMD_ACTUATOR_SET :
			//액츄에이터 동작설정 : 설정값이 동작시간인지 동작값인지 구분필요
			// 전달된 값으로 액츄에이터 동작
			ActuatorSet() ; 
			ReturnMakeReponse();
			break ;	
		case REQCMD_PASSIVE_MODE : 
			//센서노드의 센서값 즉시 획득으로 설정 
			SensorModeSet() ; 
			ReturnMakeReponse();			
			break ; 

		case REQCMD_ACTIVE_MODE :
			//센서노드의 센서값 지정된 주기 획득으로 설정
			SensorModeSet() ; 
			ReturnMakeReponse();
			break ; 
		case REQCMD_EVENT_MODE :
			//센서노드의 센서값 조건만족시 획득으로 설정  
			SensorModeSet() ; 
			ReturnMakeReponse();
			break ; 

	}
}

void TTA12PacketParsing::TTA12PacketGetSTATE1(void)
{
	WORD getCONTROL_ID= 0 ;  // 버퍼시작 8 
	WORD getNODE_ID= 0 ; 
	BYTE SensorCount=0;
	BYTE i,siSIZE;

	BYTE getSW_VER,getPROFILE_vER,getTEMP,getMONTIME,getSENSOR_NUM;
	WORD getCOMM_ERR_COUNT,getSERVICE_ERR_COUNT;
	
	siSIZE = 8 ; // sizeof(SensorInfo) ;

	getSW_VER     =packet.GetData(9) ;
	getPROFILE_vER=packet.GetData(10) ;
	
	getCONTROL_ID = packet.GetData(12) ;
	getCONTROL_ID <<= 8;
	getCONTROL_ID |= packet.GetData(11) ;	

	getNODE_ID = packet.GetData(14) ;
	getNODE_ID <<= 8;
	getNODE_ID |= packet.GetData(13) ;	

	getTEMP = packet.GetData(15) ; // ISINIT_NODE 
	getTEMP = packet.GetData(16) ; // MONITOR_MODE 

	getMONTIME = packet.GetData(18) ;
	getMONTIME <<= 8;
	getMONTIME |= packet.GetData(17) ;	

	getSENSOR_NUM = packet.GetData(19) ; // COUNT

	
	if( getSENSOR_NUM > 0 )
	{
		if( FCF.device ==  FCF_SENSOR )
		{
			for( i=0; i< getSENSOR_NUM;i++)
			{
				siData[i].data.id =  packet.GetData(20+(i*siSIZE)) ;
				siData[i].data.type =  packet.GetData(20+(i*siSIZE+1)) ;
				siData[i].data.value =  packet.GetData(20+(i*siSIZE+3)) ;
				siData[i].data.value<<=8;
				siData[i].data.value |=  packet.GetData(20+(i*siSIZE+2)) ;
				siData[i].state = packet.GetData(20+(i*siSIZE+4)) ;
				siData[i].isinit = packet.GetData(20+(i*siSIZE+5)) ;
				siData[i].mode = packet.GetData(20+(i*siSIZE+6)) ;
				siData[i].time = packet.GetData(20+(i*siSIZE+7)) ;
			}
		} else {
			
			for( i=0; i< getSENSOR_NUM; i++ )
			{
				aiData[i].data.id =  packet.GetData(20+(i*siSIZE)) ;
				aiData[i].data.type =  packet.GetData(20+(i*siSIZE+1)) ;
				aiData[i].data.value =  packet.GetData(20+(i*siSIZE+3)) ;
				aiData[i].data.value<<=8;
				aiData[i].data.value |=  packet.GetData(20+(i*siSIZE+2)) ;
				aiData[i].state = packet.GetData(20+(i*siSIZE+4)) ;
				aiData[i].isinit = packet.GetData(20+(i*siSIZE+5)) ;
				aiData[i].mode = packet.GetData(20+(i*siSIZE+6)) ;
				aiData[i].time = packet.GetData(20+(i*siSIZE+7)) ;
			}


		}

		i = getSENSOR_NUM*siSIZE + 20 ;
		
		getCOMM_ERR_COUNT = packet.GetData(i+1) ;
		getCOMM_ERR_COUNT <<= 8;
		getCOMM_ERR_COUNT |= packet.GetData(i) ;	

		getSERVICE_ERR_COUNT = packet.GetData(i+3) ;
		getSERVICE_ERR_COUNT <<= 8;
		getSERVICE_ERR_COUNT |= packet.GetData(i+2) ;	

	} else {

		getCOMM_ERR_COUNT = packet.GetData(21) ;
		getCOMM_ERR_COUNT <<= 8;
		getCOMM_ERR_COUNT |= packet.GetData(20) ;	

		getSERVICE_ERR_COUNT = packet.GetData(23) ;
		getSERVICE_ERR_COUNT <<= 8;
		getSERVICE_ERR_COUNT |= packet.GetData(22) ;	

	}

	// 전체 센서 및 엑츄에이터 버퍼에 복사 
	
	if( FCF.device ==  FCF_SENSOR )
	{	
		SensorNodeStatus[getNODE_ID].SW_VER = getSW_VER ;
		SensorNodeStatus[getNODE_ID].PROFILE_VER = getPROFILE_vER ;
		SensorNodeStatus[getNODE_ID].CONTROL_ID = getCONTROL_ID ;
		SensorNodeStatus[getNODE_ID].NODE_ID = getNODE_ID ;
		SensorNodeStatus[getNODE_ID].ISINIT_NODE = (getTEMP&0xF0)>>4 ;
		SensorNodeStatus[getNODE_ID].MONITOR_MODE = getTEMP&0x0F ;
		SensorNodeStatus[getNODE_ID].MONITOR_TIME = getMONTIME ;
		SensorNodeStatus[getNODE_ID].SENACT_NUM = getSENSOR_NUM ;
		
		for( i=0; i< getSENSOR_NUM;i++)
		{
			SI[NODE_ID*8+i]=siData[i] ;
		}
		SensorNodeStatus[getNODE_ID].COMM_ERROR_NUM = getCOMM_ERR_COUNT ;
		SensorNodeStatus[getNODE_ID].SERVICE_ERROR_NUM = getSERVICE_ERR_COUNT ;

	} else {
		AcutuatorNodeStatus[getNODE_ID].SW_VER = getSW_VER ;
		AcutuatorNodeStatus[getNODE_ID].PROFILE_VER = getPROFILE_vER ;
		AcutuatorNodeStatus[getNODE_ID].CONTROL_ID = getCONTROL_ID ;
		AcutuatorNodeStatus[getNODE_ID].NODE_ID = getNODE_ID ;
		AcutuatorNodeStatus[getNODE_ID].ISINIT_NODE = (getTEMP&0xF0)>>4 ;
		AcutuatorNodeStatus[getNODE_ID].MONITOR_MODE = getTEMP&0x0F ;
		AcutuatorNodeStatus[getNODE_ID].MONITOR_TIME = getMONTIME ;
		AcutuatorNodeStatus[getNODE_ID].SENACT_NUM = getSENSOR_NUM ;
		
		for( i=0; i< getSENSOR_NUM;i++)
		{
			AI[NODE_ID*8+i]=aiData[i] ;
		}
		AcutuatorNodeStatus[getNODE_ID].COMM_ERROR_NUM = getCOMM_ERR_COUNT ;
		AcutuatorNodeStatus[getNODE_ID].SERVICE_ERROR_NUM = getSERVICE_ERR_COUNT ;

	}
}

void TTA12PacketParsing::TTA12PacketGetSTATE2(void)
{
	WORD getCONTROL_ID= 0 ;  // 버퍼시작 9 

	BYTE getSW_VER,getPROFILE_VER,getISINIT_NODE,getISINIT_SENSOR;
	BYTE getERR_COUNT,getBUSY,getREQUEST_COMM;
	
	getSW_VER     =packet.GetData(9) ;
	getPROFILE_VER=packet.GetData(10) ;
	
	getCONTROL_ID = packet.GetData(12) ;
	getCONTROL_ID <<= 8;
	getCONTROL_ID |= packet.GetData(11) ;	

	getISINIT_NODE		= packet.GetData(13) ;
	getISINIT_SENSOR	= packet.GetData(14) ;
	getERR_COUNT		= packet.GetData(15) ;
	getBUSY				= packet.GetData(16) ;
	getREQUEST_COMM		= packet.GetData(17) ;
	

	// 콘트롤러 버퍼에 복사 
	
	ControllerStatus.SW_VER				= getSW_VER ;
	ControllerStatus.PROFILE_VER		= getPROFILE_VER ;
	ControllerStatus.CONTROL_ID			= getCONTROL_ID ;
	ControllerStatus.ISINIT_COMM_NODE	= getISINIT_NODE ;
	ControllerStatus.ISINIT_SENACT		= getISINIT_SENSOR ;
	ControllerStatus.IS_ERROR			= getERR_COUNT ;
	ControllerStatus.BUSY				= getBUSY ;
	ControllerStatus.REQUEST_COMM		= getREQUEST_COMM ;
		
}

void TTA12PacketParsing::TTA12PacketGetSTATE3(void)
{
	// 버퍼시작 9 

	BYTE getERR_CODE; 
	
	getERR_CODE    = packet.GetData(9) ;
	// NODE 에러버퍼에 복사 
	if( FCF.device ==  FCF_SENSOR )
	{	
		SensorNodeErrorCode[NODE_ID]= getERR_CODE ;
	} else {
		AcutuatorNodeErrorCode[NODE_ID]= getERR_CODE ;
	}
		
}

void TTA12PacketParsing::ParsingResponse(void)
{
	if(PLF.length > 0 )
	{ // STATE1 노드정보 및 상태 패킷 획득 
	  	TTA12PacketGetSTATE1() ;
	} 
	if( FCF.ack_confirm )
	{ // ACK 전송패킷 구성

		ReturnMakeAck();
	}
}

void TTA12PacketParsing::ParsingConfirm(void)
{
	if( PLF.length > 0 )
	{ // STATE2 제어기 정보 및 상태 패킷 획득 
		  	
		TTA12PacketGetSTATE2() ;
	
	} 
	if( FCF.ack_confirm )
	{ // ACK 전송패킷 구성

		ReturnMakeAck();
	}
}


void TTA12PacketParsing::ParsingNotify(void)
{
	if( PLF.length > 0 )
	{ // STATE2 제어기 정보 및 상태 패킷 획득 
		  	
		TTA12PacketGetSTATE3() ;
	
	} 
	if( FCF.ack_confirm )
	{ // Confirm 전송패킷 구성

		ReturnMakeConfirm();
	}

}


void TTA12PacketParsing::ParsingData(void) 
{	
	BYTE i,siSIZE;
	int nARG;
	int chINDEX ;
	siSIZE = sizeof(SensorValue) ;	
	if( PLF.length > 0 )
	{ 
		// 센싱값 획득 
		for( i=0; i < PLF.length ;i++)
		{
			if( FCF.device == FCF_SENSOR )
			{
				sData[i].id =  packet.GetData(9+(i*siSIZE)) ;
				sData[i].type =  packet.GetData(9+(i*siSIZE+1)) ;
				sData[i].value =  packet.GetData(9+(i*siSIZE+3)) ;			
				sData[i].value<<=8;
				sData[i].value |=  packet.GetData(9+(i*siSIZE+2)) ;
			} else {
				aData[i].id =  packet.GetData(9+(i*siSIZE)) ;
				aData[i].type =  packet.GetData(9+(i*siSIZE+1)) ;
				aData[i].value =  packet.GetData(9+(i*siSIZE+3)) ;			
				aData[i].value<<=8;
				aData[i].value |=  packet.GetData(9+(i*siSIZE+2)) ;	
			}
			aDataARG[i]=0x00 ; 
		}
		
		// 노드 센싱값 버퍼에 기록 
		if( FCF.device == FCF_SENSOR )
		{
			for( i=0; i< PLF.length;i++)
			{
				chINDEX = GetSensorConfigIndex( sData[i].id ) ; 
				if(chINDEX != -1 )
				{
					SI[chINDEX].data=sData[i] ;
				}
			}	
		} else {
			for( i=0; i< PLF.length;i++)
			{
				nARG=(aData[i].value&0xff00) >> 8 ; 
				aDataARG[i] = nARG ; 
				
//				printf( "TTA12PacketParsing::ParsingData\n") ; 
				chINDEX = GetActuatorConfigIndex( aData[i].id ,nARG) ; 
				if( chINDEX != -1 )
				{
					//ACTUATOR_MAP[chINDEX].ARG  = nARG ;
					//printf( "packet data index :%d\n",chINDEX ) ;
					aData[i].value &= 0x00ff ;
					AI[chINDEX].data=aData[i] ;
				}
				
			}	

		}
	
	} 

}

int TTA12PacketParsing::GetSensorData( SensorValue *gsData ) 
{
	  int i=0; 
		for( i=0; i < PLF.length ;i++)
		{
				gsData[i] = sData[i] ;
		}		
		return PLF.length  ;
}

int TTA12PacketParsing::GetActuatorData( ActuatorValue *gaData )
{
	  int i=0;
//		char nARG,chINDEX ;
		for( i=0; i < PLF.length ;i++)
		{				
					gaData[i] = aData[i] ;
					gaData[i].value |= aDataARG[i]<<8 ;
		}				
		return PLF.length  ;
}

BYTE TTA12PacketParsing::ParsingPacket(void)
{
	if( packet.Getsize() <  8 ) 
	{ 
		printf( "=============================\n") ; 
		printf( "Receive Packet small size :%d\n",packet.Getsize() ) ; 
		printf( "=============================\n" ) ; 
		return 0xFF ;
	}
	
	data = packet.GetData(0) ; 
	memcpy( &FCF,&data,1 ) ;
	
	WORD scount=0; 
	
	scount = packet.GetData(2) ;
	scount <<= 8;
	scount |= packet.GetData(1) ;
	SequenceCount = scount ;

	scount = packet.GetData(4) ;
	scount <<= 8;
	scount |= packet.GetData(3) ;
	
	CONTROL_ID = scount;

	scount = packet.GetData(6) ;
	scount <<= 8;
	scount |= packet.GetData(5) ;
	
	NODE_ID = scount;
	
	PLF.type= packet.GetData(7) ;
	PLF.length= packet.GetData(8) ;

	switch( FCF.type )
	{
	case FTYPE_REQUEST:
		ParsingRequest();
		break;
	case FTYPE_RESPONSE:
		ParsingResponse() ;
		break;
	case FTYPE_CONFIRM:
		ParsingConfirm() ;
		break;
	case FTYPE_NOTIFY:
		ParsingNotify() ;
		break;
	case FTYPE_DATA:
		ParsingData() ;
		break;
	case FTYPE_ACK:
		break;
	
	}

	return FCF.type ;
	
}




