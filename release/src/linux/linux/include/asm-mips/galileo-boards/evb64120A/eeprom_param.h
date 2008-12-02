#ifndef EEPROM_PARAM_
#define EEPROM_PARAM_
#define SDRAM_REGS 0xbf000000

unsigned int galileo_dl(void);
void (*boot_addr)(int argc, char **argv, char **envp);

#define NETWORK_BT_BIN 0
#define FLASH_BT 1
#define SERIAL_BT 2
#define NETWORK_BT_SREC 3

#define LINUX_OS 0
#define OTHER_OS 1

/********************************************************************
 *eeprom_parameters -
 *
 *This structure holds the eeprom parameters (usually stored on flash
 *memory)
 *The structure is all stored in flash memory except memory_size which
 *is probed each boot time for the real size of memory on the
 *evaluation board.
 *
 *The structure also holds information that is not used by all
 *evaluation board, such as the eth?_mac, which holds the MAC addresses
 *of the built in ethernet ports in the EVB96100 for example, but is
 *never used by EVB64120A.
 *
 *********************************************************************/

struct eeprom_parameters {
  unsigned int boot_source;
  unsigned int operating_system;

  /* network loader parametrs */
  unsigned int host_ip;
  unsigned int server_ip;
  char bootimage[64];

  /* Board parameters */
  char eth0_mac[6];
  char eth1_mac[6];
  char eth2_mac[6];
  char eth3_mac[6];

  /* Command Line (usually needed for Linux) */
  char os_command_line[256];
  unsigned int entry_point;
  unsigned memory_size;
};

#endif /* EEPROM_PARAM_ */
