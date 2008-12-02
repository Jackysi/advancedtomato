#include "Atan.h"
#include "c47xx.h"

extern void conf_gpio(int x);

/*----------------------------------------------------------------------------
Write specified data to eeprom
entry:	*src	= pointer to specified data to write
		len		= number of short(2 bytes) to be written
*/
void write_eeprom( short RegNumber, unsigned short *data, int len )
{
    int i2;
    unsigned short s_addr, s_data;
    unsigned short *src;

    src = data;
    SetEEpromToSendState();
//  the write enable(WEN) instruction must be executed before any device
//  programming can be done

    s_data = 0x04c0;
    SendAddrToEEprom(s_data); //00000001 0011000000B
    SetCSToLowForEEprom();

    s_addr = 0x0500 | (RegNumber & 0x0ff); //00000001 01dddddddd
    s_data = *src;

    for (i2 = len; i2 > 0 ; i2 --)
    {
      SendAddrToEEprom(s_addr); //00000001 01dddddd
      SendDataToEEprom(s_data); //dddddddd dddddddd
      SetCSToLowForEEprom();
      SetCSToLowForEEprom();
      //WriteWait();     
      s_addr ++;
      src ++;
      s_data = *src;
    }
//  after all data has been written to EEprom , the write disable(WDS) 
//  instruction must be executed
    SetCSToHighForEEprom();
    s_data = 0x0400;
    SendAddrToEEprom(s_data); //00000001 00000000B
    SetCSToLowForEEprom();
    SetCSToLowForEEprom();
}

void SetEEpromToSendState()
{
  conf_gpio(0x0);
  conf_gpio(0x0);
  conf_gpio(B_ECS);
  conf_gpio(B_ECS);
  
//  ;cs     __--   ,bit 2
//  ;sck    ____   ,bit 3
//  ;di     ____   ,bit 5
//  ;do     ____   ,bit 4
//  ;
}

void ResetEEpromToSendState()
{
  conf_gpio(0x0);
  conf_gpio(0x0);
  conf_gpio(0x0);
  conf_gpio(0x0);
  
//  ;cs     ____   ,bit 2  
//  ;sck    ____   ,bit 3
//  ;di     ____   ,bit 5
//  ;do     ____   ,bit 4 
//  ;
}

void SetCSToLowForEEprom()
{
  conf_gpio(0x0);
  conf_gpio(B_ECK);
  conf_gpio(B_ECK);
  conf_gpio(0x0);
  
//  ;cs     ____   ,bit 2
//  ;sck    _--_   ,bit 3
//  ;di     ____   ,bit 5
//  ;do     ____   ,bit 4 
//  ;
}

void SetCSToHighForEEprom()
{
  conf_gpio(B_ECS);
  conf_gpio(B_ECS|B_ECK);
  conf_gpio(B_ECS|B_ECK);
  conf_gpio(B_ECS);
  
//  ;cs     ----   ,bit 2
//  ;sck    _--_   ,bit 3
//  ;di     ____   ,bit 5
//  ;do     ____   ,bit 4  
//  ;
}

void send_1ToEEprom()
{
  conf_gpio(B_ECS|B_EDI);
  conf_gpio(B_ECS|B_ECK|B_EDI);
  conf_gpio(B_ECS|B_ECK|B_EDI);
  conf_gpio(B_ECS|B_EDI);
  
//  ;cs     ----   ,bit 2
//  ;sck    _--_   ,bit 3
//  ;di     ----   ,bit 5
//  ;do     ____   ,bit 4
//  ;       
}

void send_0ToEEprom()
{
  conf_gpio(B_ECS);
  conf_gpio(B_ECS|B_ECK);
  conf_gpio(B_ECS|B_ECK);
  conf_gpio(B_ECS);
  
//  ;cs     ----   ,bit 2
//  ;sck    _--_   ,bit 3
//  ;di     ____   ,bit 5
//  ;do     ____   ,bit 4
//  ;       
}

#if 0
void WriteWait()
{
  unsigned int status;
  
  SetCSToLowForEEprom();
  SetCSToHighForEEprom();
  do {
  	SetCSToHighForEEprom();
  	//status = ReadGPIOData(EDO);
  	status = gpio & B_EDO;	// read EDO bit
  }
  while (!status);   // wait for write - ready
  SetCSToLowForEEprom();
}
#endif

void SendDataToEEprom(short s_data) 
{
  int data_mask;
  
  for (data_mask = 0x8000; data_mask != 0; )
  {
    if (s_data & data_mask) 
      send_1ToEEprom();
    else
      send_0ToEEprom();  
    data_mask = data_mask >> 1;
  }
}

void SendAddrToEEprom(short s_data) 
{
  int data_mask;
  
  for (data_mask = 0x0400 ;data_mask != 0; )
  {
    if (s_data & data_mask) 
      send_1ToEEprom();
    else
      send_0ToEEprom();  
    data_mask = data_mask >> 1;
  }
}

