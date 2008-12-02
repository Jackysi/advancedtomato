#ifndef _ATAN_H_
#define _ATAN_H_

#define SINGLECOLOUR				0x1
#define DUALCOLOUR					0x2

void write_eeprom(short,unsigned short *,int);
void SetEEpromToSendState(void);
void ResetEEpromToSendState(void); 
void SetCSToLowForEEprom(void);
void SetCSToHighForEEprom(void);  
void send_1ToEEprom(void);
void send_0ToEEprom(void);
//void WriteWait(void);
void SendAddrToEEprom(short);
void SendDataToEEprom(short);

#endif
