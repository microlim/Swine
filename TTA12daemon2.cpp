/*
	TTA1,2 Simulator ver 2.1

*/


#include <stdarg.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/input.h>
#include <string.h>
#include <stdlib.h>

#include "tcpman.h"
#include "Packet.h"
#include "util.h"
#include "TTA12_packet.h"
extern "C" {
#include "iniparser.h"
#include "rs232.h"
#include "hwctrl.h"
}

////////// ���� ���� ���� ///////////////////////////////
int TransSensorDataCount=0;  
int TransActuatorDataCount=0;  
int SensorReadCount=0; 
int RS2323ReadCount=0; 
char GCG_connected=0;
TTA12PacketParsing TTAParser;  // TTA12 ��Ŷ �ؼ� Ŭ���� 
TTA12PacketParsing TTAPtest;

//////// ������ ���� ���� /////////////////////////////
pthread_t t_idTimer,t_idRS232,t_idSNODE;		

pthread_mutex_t mutex_process ;
pthread_mutex_t mutex_key ;
pthread_mutex_t mutex_rsbuf1 ;

/////////  TCP/IP manager //////////////////////////////
tcpmanager tcpman ;

int ACTUATOR_NODE_CONTROL( int node, int mode, u8 ch,u8 set )
{
	
	COMMPACKET Packet ;

  u8 outSETMASK=0x00;
  u8 outRESETMASK=0xFF;	
	
	Packet.wCanID = ACTUATOR_MAP[node*8+ch].CANID  ; 

	memset( Packet.nData , 0 , 8 ) ;
	if( mode == REQCMD_INIT || mode == REQCMD_INIT_NODE || mode == REQCMD_INIT_ACT ) // �ʱ�ȭ 
	{
			Packet.nData[0]= outSETMASK ;
			Packet.nData[1]= outRESETMASK ;	
	}
	if( mode == REQCMD_ACTUATOR_SET ) // �������� ����
	{
		if( set )
		{
			outSETMASK = 0x01 << ch ;
			outRESETMASK = 0x00 ;
		} else {
			outSETMASK = 0x00 ;
			outRESETMASK = 0x01 << ch  ;
		}
			
			Packet.nData[0]= outSETMASK ;
			Packet.nData[1]= outRESETMASK ;				
	}

	printf( "ACTUATOR SET CAN_ID[%04X]--- ON[%04X] OFF[%04x]\n", 
		Packet.wCanID ,
		Packet.nData[0],
		Packet.nData[1]	
	) ;

	USART_SendPacket( CONTROL_PORT,&Packet ) ;	
	
	return 1 ;
}
////////////////////////////////////////////////////////////////////////////////////
//
// on ������ �������� �����͸� ���� 
//
int TransOnStatusActuatorData( char mode )
{
	ActuatorValue NodeActData[64] ;	
	int NodeDataCount=0;
	int scount=0;
	ActuatorValue GetActData[8] ;

	BYTE ncnt=0; 
	BYTE cnt=0 ; 
	TTA12Packet TTAPacket;

	for( ncnt = 0 ;ncnt < InstalledActuatorNodeCount ;ncnt++)
	{
		TTAPacket.Clear() ; 
		TTAPacket.SetFrameDevice( FCF_ACTUATOR ) ;
		TTAPacket.SetFrameType( FTYPE_DATA ) ;
		TTAPacket.SetSecurity( 1 ) ;
		TTAPacket.SetAckConfirm( 1 ) ;
		TTAPacket.SetControlID( HouseControl_ID ) ;
		
		TTAPacket.SetNodeID( ncnt ) ;
		SequenceCount++ ;	
		TTAPacket.MakeHeader() ; 
		TTAPacket.MakePayload() ;
		NodeDataCount=0;
		
		for( cnt=0; cnt < 8 ;cnt++)
		{  
			//printf(" id=%d,%d aData[%d].value =  %d \n",TTAParser.aData[i].id,TTAParser.aData[i].type,i,TTAParser.aData[i].value ) ; 
			if( AI[ncnt*8+cnt].state == mode &&  AI[ncnt*8+cnt].data.id < 255 )
			{
				
				NodeActData[NodeDataCount].id = AI[ncnt*8+cnt].data.id ; 
				NodeActData[NodeDataCount].type = AI[ncnt*8+cnt].data.type  ; 
				NodeActData[NodeDataCount].value = AI[ncnt*8+cnt].data.value | ( ACTUATOR_MAP[ncnt*8+cnt].ARG<<8 ) ; 
			
				//ACTUATOR_MAP[b*8+i].ARG
				if( mode == 0x04 )
				{
					if( AI[ncnt*8+cnt].data.value == 0 )
					{
						printf("NODE: %d Add OFF ACT %d %d %d %d\n",ncnt , 
								NodeActData[NodeDataCount].id,
								NodeActData[NodeDataCount].type,
								NodeActData[NodeDataCount].value,
							ACTUATOR_MAP[ncnt*8+cnt].ARG

							) ;

					} else {
						printf("NODE : %d Add ON ACT %d %d %d %d\n",ncnt , 
								NodeActData[NodeDataCount].id,
								NodeActData[NodeDataCount].type,
								NodeActData[NodeDataCount].value,
							ACTUATOR_MAP[ncnt*8+cnt].ARG

							) ;
					}	
				} else {
						printf("NODE : %d STARTUP ACT INFO %d %d %d %d\n",ncnt , 
								NodeActData[NodeDataCount].id,
								NodeActData[NodeDataCount].type,
								NodeActData[NodeDataCount].value,
							ACTUATOR_MAP[ncnt*8+cnt].ARG

							) ;
				}

				if( NodeDataCount < 64 ) NodeDataCount++;
				AI[ncnt*8+cnt].state=0x00 ; 
			}
			
		}
		if( NodeDataCount > 0 )
		{
			TTAPacket.MakeActuatorData( PLTYPE_VALUE,NodeDataCount,NodeActData ) ;  	
			CPacket *pk = TTAPacket.GetPacket() ; 
			if( mode == 0x04 )
			{			
					printf("Actuator Status node:%d Data -> GCG ..\n",ncnt) ; 
			} else {
					printf("StartUp Actuator Info node:%d Data -> GCG ..\n",ncnt) ; 
			}	

			HexaPrint(  pk->GetBYTE(),pk->Getsize()  ) ;	
			send_msg( (char *)pk->GetBYTE(),pk->Getsize() ) ;	

/*
			// ������Ŷ�� ������ �Ľ��Ͽ� Ȯ���ϴ� ��ƾ 
			TTAPtest.Clear() ; 
			TTAPtest.packet.Add( (BYTE *)pk->GetBYTE(),pk->Getsize() ) ; 	
			
			if( TTAPtest.ParsingPacket() == FTYPE_DATA )
			{ 

				printf( " DATA packet : %d ,%d \n",TTAPtest.packet.Getsize() , TTAPtest.packet.GetReadIndex () ) ; 

				CPacket *rk = TTAPtest.GetPacket() ; 
				printf("Actuator  FTYPE_DATA TTAP-TEST -------------------------------------------- \n") ;
				scount = TTAPtest.GetActuatorData( GetActData )  ;
				for( int i = 0 ; i < scount ;i ++ )
				{
						printf( "ID:%d  VALUE:%04X\n" , GetActData[i].id , GetActData[i].value ) ;   
				}

			}	
*/


		}
	}
}
////////////////////////////////////////////////////////////////////////////////////
//
// �нú��� ���� �����͸� ���� 
//
int TransPassiveModeSensorData( void )
{
	SensorValue NodeSensorData[8] ;
	int NodeSensorDataCount=0;
	BYTE cnt=0 ;
	BYTE getINDEX;
	TTA12Packet TTAPacket;
	
	TTAPacket.Clear() ; 
	TTAPacket.SetFrameDevice( FCF_SENSOR ) ;
	TTAPacket.SetFrameType( FTYPE_DATA ) ;
	TTAPacket.SetSecurity( 1 ) ;
	TTAPacket.SetAckConfirm( 1 ) ;
	TTAPacket.SetControlID( HouseControl_ID ) ;

	TTAPacket.SetNodeID( TTAParser.NODE_ID ) ;
	SequenceCount++ ;	
	TTAPacket.MakeHeader() ; 
	TTAPacket.MakePayload() ;

	
	if( TTAParser.PLF.length > 0 )
	{ // ���̰� ȹ�� 
		for( cnt=0; cnt < TTAParser.PLF.length ;cnt++)
		{  
			//printf(" id=%d,%d aData[%d].value =  %d \n",TTAParser.sData[i].id,TTAParser.sData[i].type,i,TTAParser.sData[i].value ) ; 
			getINDEX = GetSensorConfigIndex( TTAParser.sData[cnt].id ) ;
			if( getINDEX != -1 )
			{
				NodeSensorData[cnt] = SI[getINDEX].data; 
				NodeSensorDataCount++;			
				
				SENSOR_MAP[TTAParser.NODE_ID*8+SI[cnt].data.id].SET_INTERVAL= 0x00 ; // 
			}
		}
	}		
		
	TTAPacket.MakeSensorData( PLTYPE_VALUE,NodeSensorDataCount,NodeSensorData ) ;  	
	
	CPacket *pk = TTAPacket.GetPacket() ; 
	
	printf("Passive Sensor Data -> GCG ..\n") ; 
	HexaPrint(  pk->GetBYTE(),pk->Getsize()  ) ;	
	send_msg( (char *)pk->GetBYTE(),pk->Getsize() ) ;	
	
}

////////////////////////////////////////////////////////////////////////////////////
//
// ��Ƽ���� �����͸� ���� 
//
int TransActiveModeSensorData( char mode )
{
	SensorValue NodeSensorData[8] ;
	int scount=0;
	SensorValue GetSensorData[8] ;
	int NodeSensorDataCount=0;	
	int NODE_NUM=0;BYTE cnt=0 ; 
	TTA12Packet TTAPacket;
	CPacket *pk ;	
	for(NODE_NUM=0; NODE_NUM < InstalledSensorNodeCount; NODE_NUM++)
	{
		TTAPacket.Clear() ; 
		TTAPacket.SetFrameDevice( FCF_SENSOR ) ;
		TTAPacket.SetFrameType( FTYPE_DATA ) ;
		TTAPacket.SetSecurity( 1 ) ;
		TTAPacket.SetAckConfirm( 1 ) ;
		TTAPacket.SetControlID( HouseControl_ID ) ;

		TTAPacket.SetNodeID( NODE_NUM ) ;
			
		TTAPacket.MakeHeader() ; 
		TTAPacket.MakePayload() ;

		for( cnt = 0 ; cnt < 8;cnt++)
		{
			if( SI[NODE_NUM*8+cnt].state == mode && SI[NODE_NUM*8+cnt].data.id < 255 )
			{
				
				NodeSensorData[NodeSensorDataCount].id = SI[NODE_NUM*8+cnt].data.id ; 
				NodeSensorData[NodeSensorDataCount].type = SI[NODE_NUM*8+cnt].data.type  ; 
				NodeSensorData[NodeSensorDataCount].value = SI[NODE_NUM*8+cnt].data.value  ; 
			
				/*
				printf("NODE: %d Add data %d %d %d \n",NODE_NUM , 
						NodeSensorData[NodeSensorDataCount].id,
						NodeSensorData[NodeSensorDataCount].type,
						NodeSensorData[NodeSensorDataCount].value
					) ;
					*/
				NodeSensorDataCount++;
				SI[NODE_NUM*8+cnt].state=0x00 ; 
			}
		}
			
		TTAPacket.MakeSensorData( PLTYPE_VALUE,NodeSensorDataCount,NodeSensorData ) ;  	
		

		pk = TTAPacket.GetPacket() ; 
		
		
		if( NodeSensorDataCount > 0 )
		{
			printf("Active Sensor Data -> GCG ..%d\n",pk->Getsize()) ; 
			SequenceCount++ ;
			HexaPrint(  pk->GetBYTE(),pk->Getsize()  ) ;	
			send_msg( (char *)pk->GetBYTE(),pk->Getsize() ) ;	
/*
			// ������Ŷ�� ������ �Ľ��Ͽ� Ȯ���ϴ� ��ƾ 
			TTAPtest.Clear() ; 
			TTAPtest.packet.Add( (BYTE *)pk->GetBYTE(),pk->Getsize() ) ; 	
			
			if( TTAPtest.ParsingPacket() == FTYPE_DATA )
			{ 

				CPacket *rk = TTAPtest.GetPacket() ; 
				printf("Active Sensor FTYPE_DATA TTAP-TEST -------------------------------------------- \n") ;
				scount = TTAPtest.GetSensorData( GetSensorData )  ;
				for( int i = 0 ; i < scount ;i ++ )
				{
						printf( "ID:%d  VALUE:%d\n" , GetSensorData[i].id , GetSensorData[i].value ) ;   
				}
			}	
*/

		}
		NodeSensorDataCount=0;
	}


	TransSensorDataCount=0;
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

void *Timer_handler(void * arg)
{ 
	//int clnt_sock=*((int*)arg);
	int AIndex=0;
	int SIndex=0;

	BYTE LiveTime=0; 

	while(1)
	{	
		pthread_mutex_lock(&cmd_mutex);
			for( AIndex = 0 ; AIndex < MAX_ACTUATOR_COUNT ; AIndex++) //�������� ON TIME ó�� 
			{
				if( AI[AIndex].data.value == 255 ) continue ;
				if( AI[AIndex].data.value > 0  )
				{
						AI[AIndex].data.value-- ;
						//printf( "Actuator decrease id:%d :%d\n",AIndex,AI[AIndex].data.value ) ;
						if( AI[AIndex].data.value == 0 ) 
						{ 
							AI[AIndex].state=0x04 ; 
							if( TransActuatorDataCount < 63 ) TransActuatorDataCount++ ;
							printf( "Actuator OFF index:%d id:%d \n", AIndex,AI[AIndex].data.id) ;						
							//ACTUATOR_NODE_CONTROL( AI[AIndex].NODE_ID, REQCMD_ACTUATOR_SET, AI[AIndex].data.id,0x00 ) ;
							ACTUATOR_NODE_CONTROL(ACTUATOR_MAP[AIndex].NODE_ID, REQCMD_ACTUATOR_SET, ACTUATOR_MAP[AIndex].CH,0x00 ) ;
						}
				} else {
						//printf( "Actuator OFF id:%d \n",AIndex ) ;
				}			
			}
			
			for( SIndex = 0 ; SIndex < MAX_SENSOR_COUNT ; SIndex++)
			{		
				if( SI[SIndex].mode == REQCMD_ACTIVE_MODE && SI[SIndex].data.id < 255 ) // ��Ƽ�� ����� ���� �ð� ī��Ʈ ó��
				{
					if( SENSOR_MAP[	SIndex ].COUNT > 0 )
					{
						SENSOR_MAP[	SIndex ].COUNT-- ; 
						if( SENSOR_MAP[	SIndex ].COUNT <= 0 )
						{
								// ���� ���ۿ� ���̰� ���
								//TransSensorData[TransSensorDataCount].id = SI[SIndex].data.id ; 
								//TransSensorData[TransSensorDataCount].type = SI[SIndex].data.type  ; 
								//TransSensorData[TransSensorDataCount].value = SI[SIndex].data.value  ; 
								SI[SIndex].state=0x04 ; 
								printf("Active Mode Sensor Transfer time is up : ID: %d\n",SI[SIndex].data.id ) ; 
								if( TransSensorDataCount < 63 ) TransSensorDataCount++ ;
								// ���� ī��Ʈ �ʱ�ȭ 
								SENSOR_MAP[	SIndex ].COUNT = SENSOR_MAP[	SIndex ].SET_INTERVAL ;

						}
									
					}
				}
			}

			for( SIndex = 0 ; SIndex < MAX_ACTUATOR_COUNT ; SIndex++)
			//for( SIndex = 0 ; SIndex < 8 ; SIndex++)
			{		
				 //printf("%d Actuator \n",SIndex ) ; 	
				LiveTime=AI[SIndex].data.value&0x00ff ;
				if( LiveTime > 0 ) // ON ������ ������ üũ
				{
					// printf("%d Actuator OnStatus value : %d ID: %d COUNT:%d\n",SIndex, LiveTime,AI[SIndex].data.id,ACTUATOR_MAP[	SIndex ].COUNT ) ; 	
					if( ACTUATOR_MAP[	SIndex ].COUNT > 0 )
					{
							ACTUATOR_MAP[	SIndex ].COUNT-- ; 
							if( ACTUATOR_MAP[	SIndex ].COUNT <= 0 )
							{
									AI[SIndex].state=0x04 ; 
									printf("Actuator On Status Transfer time is up : ID: %d\n",AI[SIndex].data.id ) ; 
									if( TransActuatorDataCount < 63 ) TransActuatorDataCount++ ;
									// ���߿����� Ÿ��ī��Ʈ �ʱ�ȭ 
									ACTUATOR_MAP[	SIndex ].COUNT = ACTUATOR_MAP[	SIndex ].SET_INTERVAL ;

							}
									
					}
				}
				
			}

			if( TransSensorDataCount > 0 && tcpman.getConnectionCount() )
			{  
				// ���̰� ���� DATA PACKET ���� ����
				//pthread_mutex_lock(&cmd_mutex);
				TransActiveModeSensorData(0x04) ;
				//pthread_mutex_unlock(&cmd_mutex);
				TransSensorDataCount=0;
			}

			if( TransActuatorDataCount > 0 && tcpman.getConnectionCount() )
			{  
				// ON ������ Actuator DATA PACKET ���� ����
				//pthread_mutex_lock(&cmd_mutex);
				TransOnStatusActuatorData(0x04) ;
				//pthread_mutex_unlock(&cmd_mutex);
				TransActuatorDataCount=0;
			}
			pthread_mutex_unlock(&cmd_mutex);
			
			

		 sleep(1) ; 	
	}
}

void *SNode_handler(void * arg)
{ 
			// �������κ��� �������� ����Ͽ� ���ۿ� ���  	
			//--------------------------------------------------------------------------------------
			// ��ġ�� ��ü ������忡 �ֱ������� ����Ÿ ��û ��� ���
			//--------------------------------------------------------------------------------------
			// DATA TYPE    -------  NODE TYPE  ---- NODE ID  	
			//-------------------------------------------------------
			//                     BASEBD   0x00
			// ADC LO  0x0000      ADC      0x10      0x00~0xFF
			// ADC HI  0x0100      
			// RELAY   0x0200      REALY    0x20
			// ISO     0x0300      ISO      0x30
			// COUNTER 0x0400      COUNTER  0x40 
			// SCAN    0x0700
			COMMPACKET Packet ;
			
			while(1)
			{
				for( int bs = 0 ; bs < InstalledSensorNodeCount ;bs++)
				{
					
					if( SENSOR_MAP[bs*8].TYPE == 1 ) // AD BOARD
					{
						Packet.wCanID = (SENSOR_MAP[bs*8].CANID&0x00FF)|0x0000 ;
						memset( Packet.nData , 0 , 8 ) ;
						Packet.nData[0]= 0x00 ;
						Packet.nData[1]= 0xFF ;

						//pthread_mutex_lock(&mutex_rsbuf1);
						USART_SendPacket(CONTROL_PORT, &Packet ) ;usleep(1000*500) ;
						//pthread_mutex_unlock(&mutex_rsbuf1);
						
						Packet.wCanID = (SENSOR_MAP[bs*8].CANID&0x00FF)|0x0100 ;
						memset( Packet.nData , 0 , 8 ) ;
						Packet.nData[0]= 0x00 ;
						Packet.nData[1]= 0xFF ;
						
						//pthread_mutex_lock(&mutex_rsbuf1);
						USART_SendPacket(CONTROL_PORT, &Packet ) ;usleep(1000*500) ;
						//pthread_mutex_unlock(&mutex_rsbuf1);
					}

					if( SENSOR_MAP[bs*8].TYPE == 2 ) // ISO BOARD
					{
						Packet.wCanID = SENSOR_MAP[bs*8].CANID ;
						memset( Packet.nData , 0 , 8 ) ;
						Packet.nData[0]= 0x00 ;
						Packet.nData[1]= 0xFF ;
						
						//pthread_mutex_lock(&mutex_rsbuf1);
						USART_SendPacket(CONTROL_PORT, &Packet ) ;usleep(1000*500) ;
						//pthread_mutex_unlock(&mutex_rsbuf1);
					}

					if( SENSOR_MAP[bs*8].TYPE == 3 ) // COUNTER BOARD
					{
						Packet.wCanID = SENSOR_MAP[bs*8].CANID ;
						memset( Packet.nData , 0 , 8 ) ;
						Packet.nData[0]= 0x00 ;
						Packet.nData[1]= 0xFF ;
						
						//pthread_mutex_lock(&mutex_rsbuf1);
						USART_SendPacket(CONTROL_PORT, &Packet ) ;usleep(1000*500) ;
						//pthread_mutex_unlock(&mutex_rsbuf1);
					}
				
				}
					
				if( RS2323ReadCount < 60 ) RS2323ReadCount++ ; 
				else RS2323ReadCount=0 ;
				usleep(1000*500) ;
		}
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


void InitializeActuatorOffAtStartup(void)
{
		int i=0;
		for( i=0; i < InstalledActuatorNodeCount*8 ;i++)
		{  

				printf(" ACTUATOR INIT OFF NODE_ID=%d,ID=%d,CH=%d,INIT TIME=%d\n",
					ACTUATOR_MAP[i].NODE_ID,
					ACTUATOR_MAP[i].ID,
					ACTUATOR_MAP[i].CH,
					ACTUATOR_MAP[i].INIT_TIME ) ; 
				
				AI[i].data.id = ACTUATOR_MAP[i].ID ;
				//AI[i].data.value = ACTUATOR_MAP[i].INIT_TIME ;
				//ACTUATOR_MAP[	i ].COUNT= ActuatorInterval ;
				ACTUATOR_NODE_CONTROL( ACTUATOR_MAP[i].NODE_ID , REQCMD_ACTUATOR_SET,ACTUATOR_MAP[i].CH, 0 ) ;

/*
			if( ACTUATOR_MAP[i].INIT_TIME > 0 )
			{
				printf(" ACTUATOR INIT ON NODE_ID=%d,ID=%d,CH=%d,INIT TIME=%d\n",
					ACTUATOR_MAP[i].NODE_ID,
					ACTUATOR_MAP[i].ID,
					ACTUATOR_MAP[i].CH,
					ACTUATOR_MAP[i].INIT_TIME ) ; 
				
				AI[i].data.id = ACTUATOR_MAP[i].ID ;
				AI[i].data.value = ACTUATOR_MAP[i].INIT_TIME ;
				ACTUATOR_MAP[	i ].COUNT= ActuatorInterval ;
				ACTUATOR_NODE_CONTROL( ACTUATOR_MAP[i].NODE_ID , REQCMD_ACTUATOR_SET,ACTUATOR_MAP[i].CH, 1 ) ;
			} else {
				printf(" ACTUATOR INIT OFF NODE_ID=%d,ID=%d,CH=%d,INIT TIME=%d\n",
					ACTUATOR_MAP[i].NODE_ID,
					ACTUATOR_MAP[i].ID,
					ACTUATOR_MAP[i].CH,
					ACTUATOR_MAP[i].INIT_TIME ) ; 
				
				AI[i].data.id = ACTUATOR_MAP[i].ID ;
				//AI[i].data.value = ACTUATOR_MAP[i].INIT_TIME ;
				//ACTUATOR_MAP[	i ].COUNT= ActuatorInterval ;
				ACTUATOR_NODE_CONTROL( ACTUATOR_MAP[i].NODE_ID , REQCMD_ACTUATOR_SET,ACTUATOR_MAP[i].CH, 0 ) ;
			}
*/			
		}
}

void HardwareControlForTTA12Packet(void)
{
	// ���� �������尣�� ���� ���� 
	// ������ ������ �������� ���� ����
	// --------------------------------------------------------------
	// TTA1,2 ��Ŷ�� ���� ���� �ϵ���� ���� ���� 
	// --------------------------------------------------------------
	int i=0;
	int Size =0;
	int nodeCH=0 ;
	char aARG ;BYTE aVALUE;
	printf("TTAParser.RequestCommand %02x\n",TTAParser.RequestCommand ) ; 

	switch( TTAParser.RequestCommand )
	{
		case REQCMD_INIT :
			//��� �� �����ʱ�ȭ
			ACTUATOR_NODE_CONTROL( TTAParser.NODE_ID, REQCMD_INIT,0,0x00 ) ;
			for( Size = 0 ; Size < MAX_SENSOR_COUNT ; Size++)
			{
				SENSOR_MAP[Size].NODE_ID = 0 ;
				SENSOR_MAP[Size].CH = 0 ;
				SENSOR_MAP[Size].COUNT=0;
			}	

			break ;
		
		case REQCMD_INIT_NODE :
			//��� �ʱ�ȭ
			
			break;
		
		case REQCMD_INIT_ACT :
			//���� �� �������� �ʱ�ȭ 
			
			break ; 

		case REQCMD_NODE_INFO :
			//��� ���� �� �������� ��� ����
			
			break ; 
		
		case REQCMD_ACTUATOR_SET :
			//�������� ���ۼ��� : ����ȣ, �������� ID ���� 
			//�������� ���� ���� 
				if( TTAParser.PLF.length > 0 )
				{ 
					for( i=0; i < TTAParser.PLF.length ;i++)
					{  
						//AI[NODE_ID*8+i].data=aData[i] ;
						 
						//printf(" id=%d,%d aData[%d].value =  %d \n",AI[i].data.id,AI[i].data.type,i,AI[i].data.value ) ; 
						
						aARG = (TTAParser.aData[i].value&0xFF00)>>8 ;
						aVALUE = TTAParser.aData[i].value&0x00FF ;
						printf("HardwareControlForTTA12Packet\n") ;
						nodeCH = GetActuatorConfigIndex (TTAParser.aData[i].id ,aARG) ;
						if( nodeCH > -1 )
						{
							if( aVALUE > 0 )
							{
								
								ACTUATOR_MAP[	nodeCH ].COUNT= ActuatorInterval ;
								ACTUATOR_NODE_CONTROL( ACTUATOR_MAP[nodeCH].NODE_ID , REQCMD_ACTUATOR_SET,ACTUATOR_MAP[nodeCH].CH, 1 ) ;
								//printf(" id=%d,%d aData[%d].value =  %X,count %d \n",TTAParser.aData[i].id,TTAParser.aData[i].type,i,TTAParser.aData[i].value,ACTUATOR_MAP[	nodeCH ].COUNT ) ;
								printf("ON [id=%d,ARG=%X] findINDEX=%d,count %d \n",TTAParser.aData[i].id,aARG,nodeCH,ACTUATOR_MAP[	nodeCH ].COUNT ) ;
							} else {
								
								printf("OFF [id=%d,ARG=%X] findINDEX=%d,count %d \n",TTAParser.aData[i].id,aARG,nodeCH,ACTUATOR_MAP[	nodeCH ].COUNT ) ;
								AI[nodeCH].state=0x04 ; 
							  if( TransActuatorDataCount < 63 ) TransActuatorDataCount++ ;								
								
								ACTUATOR_NODE_CONTROL( ACTUATOR_MAP[nodeCH].NODE_ID , REQCMD_ACTUATOR_SET,ACTUATOR_MAP[nodeCH].CH, 0 ) ;
							}
						} else {
								printf("NOT FIND %d [id=%d,ARG=%X] findINDEX=%d,count %d \n",TTAParser.PLF.length,TTAParser.aData[i].id,aARG,nodeCH,ACTUATOR_MAP[	nodeCH ].COUNT ) ;
						}
					}
				}
			
			
			break ;	
		case REQCMD_PASSIVE_MODE : 
				
				//��������� ������ ��� ȹ������ ���� 
				TransPassiveModeSensorData();

				
			break ; 

		case REQCMD_ACTIVE_MODE :
			//��������� ������ ������ �ֱ� ȹ������ ����
				if( TTAParser.PLF.length > 0 )
				{ // ���̰� ȹ�� 
					for( i=0; i < TTAParser.PLF.length ;i++)
					{  
						
						//printf(" id=%d,%d aData[%d].value =  %d \n",TTAParser.sData[i].id,TTAParser.sData[i].type,i,TTAParser.sData[i].value ) ; 
					
						nodeCH = GetSensorConfigIndex( TTAParser.sData[i].id );
						SENSOR_MAP[nodeCH].NODE_ID = TTAParser.NODE_ID ;
												
						if( 	(SI[i].time&0xC0) == PERSEC ) 
						{
							SENSOR_MAP[nodeCH].COUNT = SI[nodeCH].time&0x3F ;
							SENSOR_MAP[nodeCH].SET_INTERVAL= SI[nodeCH].time&0x3F ;
							
						}
						if( 	(SI[i].time&0xC0) == PERMIN )
						{
							SENSOR_MAP[nodeCH].COUNT = (SI[nodeCH].time&0x3F)*TIME_MIN ;
							SENSOR_MAP[nodeCH].SET_INTERVAL= (SI[nodeCH].time&0x3F)*TIME_MIN ;
							
						}
						if( 	(SI[i].time&0xC0) == PERHOUR )
						{
							SENSOR_MAP[nodeCH].COUNT = (SI[nodeCH].time&0x3F)*TIME_HOUR ;
							SENSOR_MAP[nodeCH].SET_INTERVAL= (SI[nodeCH].time&0x3F)*TIME_HOUR ;
							
						}
            if( 	(SI[i].time&0xC0) == PERDAY )
						{
							SENSOR_MAP[nodeCH].COUNT = (SI[nodeCH].time&0x3F)*TIME_DAY ;
							SENSOR_MAP[nodeCH].SET_INTERVAL= (SI[nodeCH].time&0x3F)*TIME_DAY ; 
							
						}
	
						//printf(" id=%d SI[%d].mode = %d [%02X:%d]\n",SI[i].data.id,i,SI[i].mode,SI[i].time&0xC0, 
						//		SENSOR_MAP[TTAParser.NODE_ID*8+SI[i].data.id].COUNT) ; 			
						/*

							SENSOR_MAP[Size].NODE_ID = 0 ;
							SENSOR_MAP[Size].SENSOR_ID = 0 ;
							SENSOR_MAP[Size].COUNT=0;		
							SI[i].NODE_ID = NODE_ID ;
							SI[i].data.id = sData[i].id ; 
							SI[i].mode = RequestCommand ; //sData[i].type ;
							SI[i].time = sData[i].value | sData[i].type  ; // value 16bit    time �� 8bit TTA12 ���� �ʿ�					
						*/
					}
				}
			
			break ; 
		case REQCMD_EVENT_MODE :
			//��������� ������ ���Ǹ����� ȹ������ ����  

		
			break ; 
	}

}

// DATA TYPE    -------  NODE TYPE  ---- NODE ID  	
//-------------------------------------------------------
//                     BASEBD   0x00
// ADC LO  0x0000      ADC      0x10      0x00~0xFF
// ADC HI  0x0100      
// RELAY   0x0200      REALY    0x20
// ISO     0x0300      ISO      0x30
// COUNTER 0x0400      COUNTER  0x40 
// SCAN    0x0700
		
void SensorValueReadToNode (void)
{
		//printf("%x %x \n", USART1StreamingBuffer.nHeader,USART1StreamingBuffer.wCanID );	// 0xaa55
		BYTE NodeID = 0 ;
		BYTE DataTYPE = 0 ;
		BYTE NodeTYPE = 0 ; 
		
		NodeID = USART1StreamingBuffer.wCanID&0x000F;
		NodeTYPE = USART1StreamingBuffer.wCanID&0x00F0;
		DataTYPE = (USART1StreamingBuffer.wCanID&0xFF00)>>8;

		if( NodeTYPE == 0x00 ) // BASE board �ܵ��������
		{
			if( DataTYPE ==0x00 )   // ADC ����ä��
			{			
				NodeID = 0 ;
				SI[NodeID*8+0].data.value = USART1StreamingBuffer.wData[0] ;
				SI[NodeID*8+1].data.value = USART1StreamingBuffer.wData[1] ;
				SI[NodeID*8+2].data.value = USART1StreamingBuffer.wData[2] ;
				SI[NodeID*8+3].data.value = USART1StreamingBuffer.wData[3] ;

				printf( "ADC 1,2,3,4  %4d %4d %4d %4d\n",
					SI[NodeID*8+0].data.value ,
					SI[NodeID*8+1].data.value ,
					SI[NodeID*8+2].data.value ,
					SI[NodeID*8+3].data.value  ) ; 

			}
			if( DataTYPE ==0x01 )  // ADC ����ä��
			{			
				NodeID = 0 ;
				SI[NodeID*8+4].data.value = USART1StreamingBuffer.wData[0] ;
				SI[NodeID*8+5].data.value = USART1StreamingBuffer.wData[1] ;
				SI[NodeID*8+6].data.value = USART1StreamingBuffer.wData[2] ;
				SI[NodeID*8+7].data.value = USART1StreamingBuffer.wData[3] ;				
				
				printf( "ADC 5,6,7,8 %4d %4d %4d %4d\n",
					SI[NodeID*8+4].data.value ,
					SI[NodeID*8+5].data.value ,
					SI[NodeID*8+6].data.value ,
					SI[NodeID*8+7].data.value  ) ; 

			}


			if( DataTYPE ==0x03 ) // ISO �Է�
			{	
				NodeID = 1 ;
				BYTE bitVAL=0x01; 
				printf( "ISO IN %02X\n",
					USART1StreamingBuffer.nData[0] ) ;
				for( char i = 0 ; i < 8 ;i ++)
				{
					if( (USART1StreamingBuffer.nData[0]&bitVAL)==bitVAL ) 
						SI[NodeID*8+i].data.value=0xFFFF; 
					else 
						SI[NodeID*8+i].data.value=0x0000;
					bitVAL<<=1; 

					printf( "ISO IN CH %d:%02X\n",i, SI[NodeID*8+i].data.value ) ; 
				}
				
 

			}
			
			if( DataTYPE ==0x04 ) // COUNTER �Է�
			{			
				NodeID = 2 ;
				printf( "COUNTER IN CH1 %d,CH2 %d\n",
					USART1StreamingBuffer.dwData[0],USART1StreamingBuffer.dwData[1] ) ; 
				SI[NodeID*8+0].data.value=USART1StreamingBuffer.dwData[0]; 
				SI[NodeID*8+1].data.value=USART1StreamingBuffer.dwData[1]; 

			}	

		} else {  // Extend board 
			
			if( NodeTYPE ==0x10 )   // ADC 
			{	
				if( DataTYPE ==0x00 )   // ADC ����ä��
				{			
					
					SI[NodeID*8+0].data.value = USART1StreamingBuffer.wData[0] ;
					SI[NodeID*8+1].data.value = USART1StreamingBuffer.wData[1] ;
					SI[NodeID*8+2].data.value = USART1StreamingBuffer.wData[2] ;
					SI[NodeID*8+3].data.value = USART1StreamingBuffer.wData[3] ;

					printf( "ADC 1,2,3,4  %4d %4d %4d %4d\n",
						SI[NodeID*8+0].data.value ,
						SI[NodeID*8+1].data.value ,
						SI[NodeID*8+2].data.value ,
						SI[NodeID*8+3].data.value  ) ; 

				}
				if( DataTYPE ==0x01 )  // ADC ����ä��
				{			
					
					SI[NodeID*8+4].data.value = USART1StreamingBuffer.wData[0] ;
					SI[NodeID*8+5].data.value = USART1StreamingBuffer.wData[1] ;
					SI[NodeID*8+6].data.value = USART1StreamingBuffer.wData[2] ;
					SI[NodeID*8+7].data.value = USART1StreamingBuffer.wData[3] ;				
					
					printf( "ADC 5,6,7,8 %4d %4d %4d %4d\n",
						SI[NodeID*8+4].data.value ,
						SI[NodeID*8+5].data.value ,
						SI[NodeID*8+6].data.value ,
						SI[NodeID*8+7].data.value  ) ; 

				}
			}


			if( NodeTYPE ==0x30 && DataTYPE ==0x03 ) // ISO �Է�
			{	
				BYTE bitVAL=0x01; 
				printf( "ISO IN %02X\n",
					USART1StreamingBuffer.nData[0] ) ;
				for( char i = 0 ; i < 8 ;i ++)
				{
					if( (USART1StreamingBuffer.nData[0]&bitVAL)==bitVAL ) 
						SI[NodeID*8+i].data.value=0xFFFF; 
					else 
						SI[NodeID*8+i].data.value=0x0000;
					bitVAL<<=1; 

					printf( "ISO IN CH %d:%02X\n",i, SI[NodeID*8+i].data.value ) ; 
				}
				
			}

			if( NodeTYPE ==0x40 && DataTYPE ==0x04 ) // COUNTER �Է�
			{			
				NodeID = 2 ;
				printf( "COUNTER IN CH1 %d,CH2 %d\n",
					USART1StreamingBuffer.dwData[0],USART1StreamingBuffer.dwData[1] ) ; 
				SI[NodeID*8+0].data.value=USART1StreamingBuffer.dwData[0]; 
				SI[NodeID*8+1].data.value=USART1StreamingBuffer.dwData[1]; 

			}	
		}
		
		if( SensorReadCount < 20 ) SensorReadCount++ ;
			else SensorReadCount=0 ;		

}

void
daemonize ()
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


int main(int argc, char **argv)
{
	bool rtn;
	int tmKey ; 
	int idx;
    int c ;

    while ((c = getopt (argc, argv, "c:dv:")) != -1) {
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
	// ȯ�� �������� 
	//
	PacketVariable_init() ; 

  ReadDevice_Config() ;
  ReadServer_Config() ;	

	//////////////////////////////////////////////////////////////////////////
	//
	// ���� �ʱ�ȭ 
	//
	pthread_mutex_init(&mutex_key, NULL);
	pthread_mutex_init(&mutex_rsbuf1, NULL);
	pthread_mutex_init(&mutex_process, NULL);
	

	int isOK = 0;
	int retry_count=0 ;
	
	//////////////////////////////////////////////////////////////////////////
	//
	// �ø�����Ʈ �ʱ�ȭ 
	//
	
	printf("Serial port open \n") ;
	if( RS232_OpenComport(CONTROL_PORT, PORT_SPEED) )
	{
		printf("Can not open comport\n") ;
	}
	
	pthread_create(&t_idRS232, NULL, RS232_Thread, NULL) ; 
	pthread_detach(t_idRS232) ;

	pthread_create(&t_idSNODE, NULL, SNode_handler, NULL) ; 
	pthread_detach(t_idSNODE) ;
	
	pthread_create(&t_idTimer, NULL, Timer_handler, NULL) ; 
	pthread_detach(t_idTimer) ;
	//////////////////////////////////////////////////////////////////////////
	//
	// GCG ���� �õ� 
	//

	while( retry_count < 5 )
	{
		GCG_connected = isOK = tcpman.ClientConnect( GCG_IP,GCG_PORT ) ;
		
		if( isOK != -1 ) break ;
		usleep(1000*1000) ;
		retry_count++ ;
	}
	printf("======================================\n") ;
	printf("TTA1,2 daemon build 2.3002  2015-9-28  \n" ) ;
	printf("NO Handshaek version ...    \n" ) ;
	printf("config.ini applied    \n" ) ;
	printf("Initialize Actuator at Power ON   \n" ) ;
	printf("======================================\n") ;
	
	BuzzerBeep(); BuzzerBeep(); 
	sleep(1) ;

	//////////////////////////////////////////////////////////////////////////
	//
	// �������� �ʱ�ȭ ( ��� �������� OFF ) 
	//
	InitializeActuatorOffAtStartup() ;
	
	
	//////////////////////////////////////////////////////////////////////////
	//
	// �ʱ� ������ ���  
	//
	printf("Sensor Status Reading ................\n") ;
	while( RS2323ReadCount < 30 )
	{
		if( UART1PacketStreaming() )
		{
				SensorValueReadToNode() ;
		}
		if( SensorReadCount > 5 ) break ;
	}
	

	//////////////////////////////////////////////////////////////////////////
	//
	// �ʱ� ������ ����  ( ��缾���� ��Ƽ����� ���� :�����ð����� ������ ����)
	//
  
	TransActiveModeSensorData(0x00) ; 
	//TransOnStatusActuatorData(0x00) ; 	// 


	//////////////////////////////////////////////////////////////////////////
	//
	// ���� LOOP 
	//
	BYTE pTYPE=0xFF ;
	while(1)
	{

		//RS232 ���Ź��� üũ
		if( UART1PacketStreaming() )
		{
				SensorValueReadToNode() ; 
		}

		// GCG connection üũ 
		if( tcpman.getConnectionCount() == 0 ) 
		{
				printf("GCG connection losted.... try reconnection \n") ;
				GCG_connected = isOK = tcpman.ClientConnect( GCG_IP,GCG_PORT ) ;
				usleep(1000*1000) ;
				if( isOK != -1 ) continue ;
		}

		//TCP/IP ������Ŷ üũ from GCG  
		if( TCP_CMD_HEAD < TCP_CMD_TAIL )
		{

			printf("Reveived GCG Packet...................................................\n") ;
			HexaPrint(  TCP_CMD_BUF[TCP_CMD_HEAD].buf,TCP_CMD_BUF[TCP_CMD_HEAD].size ) ;
			
			pthread_mutex_lock(&cmd_mutex);
			if( TCP_CMD_BUF[TCP_CMD_HEAD].size > 0 )
			{
				//pthread_mutex_lock(&mutex_process);
				idx = 0;
				do {
					//printf("TCP BUFF head %d: tail %d\n",TCP_CMD_HEAD,TCP_CMD_TAIL ) ;
					TTAParser.Clear() ; 
					TTAParser.packet.Add( TCP_CMD_BUF[TCP_CMD_HEAD].buf + idx, TCP_CMD_BUF[TCP_CMD_HEAD].size - idx); 

					// TTA3���� ���Ž� FTYPE_REQUEST �� �������Դϴ�.
					pTYPE = TTAParser.ParsingPacket() ;
					
					switch (pTYPE)
					{
						case  FTYPE_REQUEST:
									printf("FTYPE_REQUEST\n" ) ;
									HardwareControlForTTA12Packet() ;	
									break;
						
						case  FTYPE_RESPONSE:
									printf("FTYPE_RESPONSE \n") ;
									break;
						
						case  FTYPE_CONFIRM :
									printf("FTYPE_CONFIRM\n") ;
									break;
						
						case  FTYPE_NOTIFY:
									printf("FTYPE_NOTIFY\n") ;
									break;
						
						case  FTYPE_DATA:
								  printf("FTYPE_DATA\n") ;
									break;

					}
					
					if( pTYPE != FTYPE_CONFIRM &&  pTYPE != 0xFF  )
					{
						// Response ��Ŷ�� ����� ��������  
						//2015.9.29 GCG���� RESPONSE PACKET �� ó���������� ���۾���,
						//�����ϸ� GCG������ DATA ��Ŷ �̿ܿ���  No Data Packet ���� ǥ�õ�.						
						CPacket *rk = TTAParser.GetReturnPacket() ;  
						if( rk->Getsize() > 8 )	send_msg( (char *)rk->GetBYTE(),rk->Getsize() ) ;
						
					}

					idx += TTAParser.packet.GetReadIndex () + 1;
					printf("CMD BUFFER GetReadIndex : %d ................................................\n",TTAParser.packet.GetReadIndex () ) ; 
				} while (TCP_CMD_BUF[TCP_CMD_HEAD].size > idx);
				//pthread_mutex_unlock(&mutex_process);

			}
	
			
			TCP_CMD_HEAD++ ; 
			if(  TCP_CMD_HEAD == TCP_CMD_TAIL ) TCP_CMD_HEAD=TCP_CMD_TAIL=0 ; 
			pthread_mutex_unlock(&cmd_mutex);
		} // if( TCP_CMD_HEAD < TCP_CMD_TAIL )

		if( SequenceCount > 65530 ) SequenceCount=0;
		usleep(20000) ;
	} // while 
	
	tcpman.CloseServer() ;	
	//destroy_timer(); 
	//////////////////////////////////////////////////////////////////////////
	//
	// �������� �ʱ�ȭ ( ��� �������� OFF ) 
	//
	InitializeActuatorOffAtStartup() ;

	printf("Program Exit \n") ;
	
}
