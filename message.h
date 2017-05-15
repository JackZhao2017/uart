#ifndef _MESSAGE_H_
#define _MESSAGE_H_

#define SYN_SIGN		0x55
#define VEHICLESTATUS   0x30
#define SYSCONTROL_RX   0x20
#define SYSCONTROL_TX   0x21
#define LDWSTATUS	    0x40
#define FCWSTATUS		0x41

#define LDW_MESSAGESIZE 13
#define FCW_MESSAGESIZE 13
//commad
#define SHUT_DOWN       0x10
#define WAKE_UP			0x11
#define CALIBRATION		0x80

#define VEHICLESTATUS_VALID  4
#define SYSCONTROL_RX_VALID  4

#define SPEED_CONEFFICIENT 0.05625
#define STEERINGANGLE_CONEFFICIENT 0.1

#define u8  unsigned char
#define u16 unsigned short
#define u32 unsigned int


typedef struct{
	float speed;
	char  headlightstatus;
	char  ldwenabled;
	char  fcwenabled;
}VEHICLESTATUS_INFO;

typedef struct LDW_INFO
{
	int 		ldwCred;		//ÖÃÐÅ¶È£º0£º²»¿ÉÓÃ£»1£º×ó£»2£ºÓÒ
	int         Errorcode;
	float 		ldwDis;			//Ô½½ç¾àÀë
	float		ldwTime;		    //Ô½½çÊ±¼ä
	float		CurveRadius;
	int 		LaneWidth;
	int 		LaneType;
}LDW_OUTINFO;

typedef struct FCW_INFO
{
	int 		fcwCred;		//ÖÃÐÅ¶È£º0£º²»¿ÉÓÃ£»1£º¿ÉÓÃ
	int  		Errorcode;
	float 		fcwDis;			//Ç°³µ¾àÀë
	float		fcwTtc;			//Ïà¶ÔÅö×²Ê±¼ä
	float		fcwAttc;		//¾ø¶ÔÅö×²Ê±¼ä
}FCW_OUTINFO;

typedef struct {
	int Seqnum;
	int Commad;
	int datalen;
	char *data;
}SYS_CTRLINFO;

typedef struct 
{
	LDW_OUTINFO  ldw_info;
	FCW_OUTINFO	 fcw_info;
	SYS_CTRLINFO ctrl_info;
	VEHICLESTATUS_INFO vehicle_info;
}WARNNIG_CENTER;

typedef struct {
	u8 *addr;
	int len;
}BUFINFO;

void message_creator(WARNNIG_CENTER center,int m,BUFINFO bufinfo);

void getVehiclestatusInfo(VEHICLESTATUS_INFO *vehicleInfo);

int  getCommand(int *cmd);
void cleanCommand(void);


#endif
