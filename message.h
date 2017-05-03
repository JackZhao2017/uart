#ifndef _MESSAGE_H_
#define _MESSAGE_H_


#define VEHICLESTATUS   0x30
#define SYSCONTROL_RX   0x20

#define SYSCONTROL_TX   0x21
#define LDWSTATUS	    0x40
#define FCWSTATUS		0x41

#define LDW_MESSAGESIZE 11
#define FCW_MESSAGESIZE 13


typedef struct{
	float speed;
	float steeringangle;
	char  steeringanglevalid;
	char  leftturnon;
	char  rightturnon;
	char  frontwiperlevel;
	char  headlightstatus;
	char  ldwenabled;
	char  fcwenabled;
	char  ldwsensitivity;
	char  fcwsensitivity;
	char  breaklevel;
	char  gearshift;

}VEHICLESTATUS_INFO;

typedef struct LDW_INFO
{
	int 		ldwCred;		//ÖÃÐÅ¶È£º0£º²»¿ÉÓÃ£»1£º×ó£»2£ºÓÒ
	float 		ldwDis;			//Ô½½ç¾àÀë
	float		ldwV;		//Ô½½çÊ±¼ä
}LDW_OUTINFO;

typedef struct FCW_INFO
{
	int 		fcwCred;		//ÖÃÐÅ¶È£º0£º²»¿ÉÓÃ£»1£º¿ÉÓÃ
	float 		fcwDis;			//Ç°³µ¾àÀë
	float		fcwTtc;			//Ïà¶ÔÅö×²Ê±¼ä
	float		fcwAttc;		//¾ø¶ÔÅö×²Ê±¼ä
}FCW_OUTINFO;

typedef struct 
{
	LDW_OUTINFO ldw_info;
	FCW_OUTINFO	fcw_info;
}WARNNIG_CENTER;

typedef struct {
	char *addr;
	int len;
}BUFINFO;

#endif
