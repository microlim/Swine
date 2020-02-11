#ifndef __COMMON_DEFINE__
#define __COMMON_DEFINE__

#define BYTE unsigned char
#define WORD unsigned short

#define u8 unsigned char
#define u16 unsigned short
#define u32 unsigned int
#define u64 unsigned long long

// Message packet buffer 
typedef struct 
{
	int    tx_index ; 
	int    rx_index ; 
	
	BYTE   buf[128] ;
	int    size ;
	
} MESSAGE_STRUCT ;


#define	COMMPACKET_SIZE		14
#define PACKET_HEADER			(u16)0xaa55
typedef struct __tagCOMMPACKET__ {
	u16		nHeader;	// 0xaa55
	u16		wCanID;		// __tagPCCOMM__, CANID_11_bits
	union	{
		u8		nData[8];
		u16		wData[4];
		u32		dwData[2];
//		u64		nLongLong;
	};
	u16		nChksum;	// wComm_b0..15 + wData[0..3]
} COMMPACKET, *LPCOMMPACKET;

#define 	UART_DATAQUE_SIZE   256
typedef struct _tagUART_DATAQUE_
{
  u16		nPushIndex;
  u16		nPopIndex;
  u8		nBuffer[UART_DATAQUE_SIZE];
} UART_DATAQUE, *LPUART_DATAQUE;

#define UART_START	0xAA
#define UART_ISTART 0x55

typedef struct {
  unsigned char  Start;        // 0xaa header
  unsigned char  iStart;       // 0x55 header
  unsigned char  nLength;
  unsigned char  nTxCanID;     // LSB's 8bit ID, tartget ID    
  unsigned char  nRxCanID;     // LSB's 8bit ID, tartget ID    
  unsigned char  nData[8];     // LSB first transfer
  unsigned char  nCheckSum;    // nCheckSum = dLength + nCanID + nData[0..7]
} DefTypeUART2CAN, *pDefTypeUART2CAN;

typedef struct {
  u8  fHeader;        // 0x55 header
  u8  sHeader;       // 0x55 header
  u8  mCode;       // Command code
  u16  nSocketID;
  char  nData[1024];     // data 

} DefTypePACKET, *pDefTypePACKET;


typedef struct {
      // '{' header
  char type;
  char nData[1024*20];     // data 
           // '}' end
} DefJSONPACKET, *pDefJSONPACKET;


#define 	CMD_DATAQUE_SIZE   256
#define 	CMD_DATAQUE_SIZE2   1024*20
typedef struct _tagCMD_DATAQUE_
{
  u16		nPushIndex;
  u16		nPopIndex;
  DefTypeUART2CAN	nBuffer[CMD_DATAQUE_SIZE];
} CMD_DATAQUE, *LPCMD_DATAQUE;

typedef struct _tagPACKET_DATAQUE_
{
  u16		nPushIndex;
  u16		nPopIndex;
  DefJSONPACKET	nBuffer[CMD_DATAQUE_SIZE];
} PACKET_DATAQUE, *LPPACKET_DATAQUE;

typedef struct {
  int			 code;        
  char  name[128];       
  char  ip[16];
  int            port;     
  unsigned char  canid;    
  int			status;
  int			isConnected;
  int	socketnum;
} DefTypeNodeInfo, *pDefTypeNodeInfo;


typedef struct {
	unsigned char  id[32];     // id 
	unsigned char  pass[32];     // ¾ÏÈ£ 
	unsigned char  mode;
} DefTypeLOGIN, *pDefTypeLOGIN;

typedef struct {
	unsigned char  id[32];
	unsigned char  datetime[64];     // id 
	unsigned char  value[32];
	unsigned char  addr[256];
} DefTypeDLog, *pDefTypeDLog;


typedef struct {
	char  icNo[32];
	int   farmNo;
	char  devType;
	char  devId[8];
	int   started;
	char  enable;
	int	  curcycle;
	int	  setcycle;
} DefCycleDevice, *pDefCycleDevice;

#define MAXNODECOUNT 16

#define FTEK_GETFILENAME	0x01
#define FTEK_FILESAVE		0x02
#define FTEK_FILECONTENT	0x03
#define FTEK_IDCHECK		0x04
#define FTEK_GETMODE		0x05
#define FTEK_DELIVERY_LOG	0x06

#define FTEK_CMDMODE_READY	0x01
#define FTEK_CMDMODE_FILE	0x02


#define FTEK_NORMAL_MODE	0x01
#define FTEK_GETBILL_MODE	0x02
#define FTEK_ABNORMAL_MODE	0x03

#endif 