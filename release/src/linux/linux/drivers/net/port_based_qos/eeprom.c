#include <linux/delay.h>

#define EEDO_PIN	4
#define EECS_PIN	2
#define EECK_PIN	3
#define EEDI_PIN	5
#define RESET_PIN	0

static void SetCSToLowForEEprom(void);
//static void SetCSToHighForEEprom(void);
static void send1ToEEprom(void);
static void send0ToEEprom(void);
static void InitSerialInterface(void);
static void SerialPulse(void);
static void WriteDataToRegister(unsigned short RegNumber, unsigned short data);
void ReadDataFromRegister(unsigned short addr, unsigned short *hidata, unsigned short *lodata,int select_count);
static void WriteDataToEEprom(unsigned short addr, unsigned short data);
static void WriteCmdToEEprom(unsigned short cmd);
extern void gpio_line_set(int x, unsigned int value);
extern void gpio_line_get(int x, int *value);
extern void gpio_line_config_out(int x);
extern void gpio_line_config_in(int x);
extern void gpio_line_config_out_all(void);
//  ;cs     __--   ,bit 3
//  ;sck    ____   ,bit 4
//  ;di     ____   ,bit 5
//  ;do     ____   ,bit 6
//  ;

static void SetEEpromToSendState(void)
{        
	gpio_line_set(EECS_PIN, 0); 
        gpio_line_set(EECK_PIN, 0);       
	gpio_line_set(EEDI_PIN, 1);	
//	gpio_line_set(EEDO_PIN, 1);    /* high impedance */	
	
        mdelay(1);
        gpio_line_set(EECS_PIN, 1);
//	gpio_line_set(EEDO_PIN, 1);    /* high impedance */
}

#if 0
static void EEpromInit(void)
{
	gpio_line_set(EECS_PIN, 0); 
        gpio_line_set(EECK_PIN, 0);       
	gpio_line_set(EEDI_PIN, 1);	
	gpio_line_set(EEDO_PIN, 1);    /* high impedance */	
	
        mdelay(1);
}
   
//  ;cs     ____   ,bit 3  
//  ;sck    ____   ,bit 4
//  ;di     ____   ,bit 5
//  ;do     ____   ,bit 6 
//  ;
static void ResetEEpromToSendState(void)
{
	gpio_line_set(EECS_PIN, 0);
	gpio_line_set(EEDI_PIN, 0);
	//gpio_line_set(EEDO_PIN, 0);
	gpio_line_set(EECK_PIN, 0);
}
#endif /* 0 */
   
//  ;cs     ____   ,bit 3  
//  ;sck    _--_   ,bit 4
//  ;di     ____   ,bit 5
//  ;do     ____   ,bit 6 
//  ;
static void SetCSToLowForEEprom(void)
{
        /* minimum tcs is 1us */      
	gpio_line_set(EECS_PIN, 0);        
        gpio_line_set(EECS_PIN, 0); 	  

	gpio_line_set(EECK_PIN, 0);
	gpio_line_set(EECK_PIN, 1);
	gpio_line_set(EECK_PIN, 1);
	gpio_line_set(EECK_PIN, 1);
	gpio_line_set(EECK_PIN, 1);
	gpio_line_set(EECK_PIN, 0);

	gpio_line_set(EECS_PIN, 1);     

        udelay(10);
}

#if 0  
//  ;cs     ----   ,bit 3
//  ;sck    _--_   ,bit 4
//  ;di     ____   ,bit 5
//  ;do     ____   ,bit 6  
//  ;
static void SetCSToHighForEEprom(void)
{
	gpio_line_set(EECS_PIN, 1);
	
        /* min tskh and tskl is 1us */
	gpio_line_set(EECK_PIN, 1);
	udelay(2);
	gpio_line_set(EECK_PIN, 0); 
}
#endif /* 0 */
  
//  ;cs     ----   ,bit 3
//  ;sck    _--_   ,bit 4
//  ;di     ----   ,bit 5
//  ;do     ____   ,bit 6
//  ;       
static void send1ToEEprom(void)
{	   
//printf("send1ToEEprom(1)...");
	gpio_line_set(EEDI_PIN, 1);	    	     
	    
	gpio_line_set(EECK_PIN, 0);
        udelay(1);	    
	gpio_line_set(EECK_PIN, 1);             
	gpio_line_set(EECK_PIN, 1);	 
	gpio_line_set(EECK_PIN, 1);   	
        udelay(1);
	gpio_line_set(EECK_PIN, 0);
}

//  ;cs     ----   ,bit 3
//  ;sck    _--_   ,bit 4
//  ;di     ____   ,bit 5
//  ;do     ____   ,bit 6
//  ;       
static void send0ToEEprom(void)
{
//printf("send0ToEEprom(0)...");
	gpio_line_set(EEDI_PIN, 0);	    	     
	    
	gpio_line_set(EECK_PIN, 0);	    
	udelay(1);
	gpio_line_set(EECK_PIN, 1);             
	gpio_line_set(EECK_PIN, 1);	 
	gpio_line_set(EECK_PIN, 1);   	
	udelay(1);
	gpio_line_set(EECK_PIN, 0);
}

static void WriteDataToEEprom(unsigned short addr, unsigned short data) 
{
  unsigned short addr_mask, data_mask;
  
  SetEEpromToSendState();
  for (addr_mask = 0x400; addr_mask != 0; )
  {
    if (addr & addr_mask) 
      send1ToEEprom();
    else
      send0ToEEprom();  
    addr_mask = addr_mask >> 1;    
  }
  for (data_mask = 0x8000; data_mask != 0; )
  {
    if (data & data_mask) 
      send1ToEEprom();
    else
      send0ToEEprom();  
    data_mask = data_mask >> 1;    
  }
  SetCSToLowForEEprom();
}

static void WriteCmdToEEprom(unsigned short cmd) 
{
  unsigned short cmd_mask;
  
  SetEEpromToSendState();
  for (cmd_mask = 0x0400 ;cmd_mask != 0; )
  {
    if (cmd & cmd_mask) 
      send1ToEEprom();
    else
      send0ToEEprom();  
    cmd_mask = cmd_mask >> 1;
  }
   SetCSToLowForEEprom();
}

/*
 * Write data to configure registers through EEPROM interface, even we do not have
 * an external EEPROM connectted, ADM6996 got a virtual AT39C66 inside
 */
static void WriteDataToRegister(unsigned short RegNumber, unsigned short data)
{
    unsigned short cmd, addr;   

    printk("WriteDataToRegister(RegNumber=0x%x, data=0x%x)\n", RegNumber, data);
   
//  the write enable(WEN) instruction must be executed before any device
//  programming can be done
    cmd = 0x04c0;
    WriteCmdToEEprom(cmd); //00000001 0011000000B    

    addr = 0x0500 | (RegNumber & 0x0ff); //00000001 01dddddddd   
    WriteDataToEEprom(addr, data); //00000001 01dddddd
    
     
//  after all data has been written to EEprom , the write disable(WDS) 
//  instruction must be executed   
    cmd = 0x0400;
    WriteCmdToEEprom(cmd); //00000001 00000000B   
}

static void SerialDelay(int count)
{        
     udelay(count);
}

static void InitSerialInterface(void)
{
        gpio_line_set(EECK_PIN, 0);  
        gpio_line_set(EEDI_PIN, 0);  
}

static void SerialPulse(void)
{
	 gpio_line_set(EECK_PIN, 0);  
	 gpio_line_set(EECK_PIN, 1);  	
         SerialDelay(10);
         gpio_line_set(EECK_PIN, 1);  
         gpio_line_set(EECK_PIN, 0);  
}
/* 
 * Since there is no EEPROM is our board, read from EEPROM need to obey the timing alike
 *  MII interface, EECK = MDC, EEDI = MDIO, please refer to section 4.3 of ADM6996 datasheet
 */
void ReadDataFromRegister(unsigned short addr, unsigned short *hidata, unsigned short *lodata, int select_count) 
{
  	unsigned short addr_mask, data_mask;
  	int value, i;
        unsigned char StartBits, Opcode, TAbits;
	
	gpio_line_config_out_all();
	mdelay(1);	
	/* initialize serial interface */
        gpio_line_set(EECS_PIN, 0);  
	InitSerialInterface();        

        /* Preamble, 35 bits */
        gpio_line_set(EECK_PIN, 0); 
        gpio_line_set(EEDI_PIN, 1);
        for (i = 0; i < 35; i++)        
        {    
            gpio_line_set(EECK_PIN, 1); 	                
            SerialDelay(10);
            gpio_line_set(EECK_PIN, 0); 
	    SerialDelay(10);
        }
 
        /* Start bits, 2-bit(01b) */
        InitSerialInterface();
        StartBits = 0x01;
        for (i = 0; i < 2; i++)
        {
             value = (StartBits & 2) ? 1 : 0;
             gpio_line_set(EEDI_PIN, value);  
             SerialDelay(1);
             SerialPulse();
             StartBits <<= 1;
         }
          
         /* Opcode, read = 10b */
         InitSerialInterface();
         Opcode = 0x02;
         for (i = 0; i < 2; i++)
         {
             value = (Opcode & 0x02) ? 1 : 0;
             gpio_line_set(EEDI_PIN, value);  
	     SerialDelay(1);
             SerialPulse();
             Opcode <<= 1;
         }         

         /* 10 bits register address */
         /* 1-bit Table Select, 2-bit Device Address, 7-bit Register Address  */         
         InitSerialInterface();
	 if (select_count)
	 	addr = (addr & 0x7f) | 0x200; 
	 else
	 	addr = addr & 0x7f ; 
         for (addr_mask = 0x200; addr_mask != 0; addr_mask >>= 1)
         {
             value = (addr & addr_mask) ? 1 : 0;
             gpio_line_set(EEDI_PIN, value);  
	     SerialDelay(1);
             SerialPulse();           
         }      	

         /* TA, turnaround 2-bit */
         InitSerialInterface();
         TAbits = 0x02;	
          gpio_line_config_in(EEDI_PIN);
         for (i = 0; i < 2; i++)
         {
             gpio_line_set(EECK_PIN, 1);  	     
	     SerialDelay(4);  	     
	     gpio_line_get(EEDI_PIN, &value); 
	     SerialDelay(4);  
             TAbits <<= 1;            
	     gpio_line_set(EECK_PIN, 1);
         }
 
	
         /* Latch data from serial management EEDI pin */
         *hidata = 0;
         gpio_line_set(EECK_PIN, 0);  
	 for (data_mask = 0x8000; data_mask != 0; data_mask >>= 1)       
         {
             SerialDelay(4);             
             gpio_line_set(EECK_PIN, 1);  
             gpio_line_get(EEDI_PIN, &value); 
             if (value)
             {
                 *hidata |= data_mask;                 
	     }
             gpio_line_set(EECK_PIN, 0); 
	     SerialDelay(4);
         }
	 *lodata = 0;
         gpio_line_set(EECK_PIN, 0);  
	 for (data_mask = 0x8000; data_mask != 0; data_mask >>= 1)       
         {
             SerialDelay(4);           
             gpio_line_set(EECK_PIN, 1);  
             gpio_line_get(EEDI_PIN, &value); 
             if (value)
             {
                 *lodata |= data_mask;                 
	     }
             gpio_line_set(EECK_PIN, 0); 
	     SerialDelay(4);
         }
  	
         SerialDelay(2);

         /* Idle, EECK must send at least one clock at idle time */
         SerialPulse();
         gpio_line_set(EECK_PIN, 0);  
         SerialDelay(10);
         gpio_line_set(EECK_PIN, 1);  
         SerialDelay(10);
         gpio_line_set(EECK_PIN, 0);  
         SerialPulse();

         gpio_line_config_out(EEDI_PIN);
	 gpio_line_set(EECS_PIN, 1);

   //printk("ReadDataFromRegister(addr=0x%x, hidata=0x%x, lodata=0x%x)\n", addr, *hidata, *lodata);
}
