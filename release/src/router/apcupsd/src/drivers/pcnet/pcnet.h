/*
 * pcnet.h
 *
 * Public header file for the pcnet driver.
 */

#ifndef _PCNET_H
#define _PCNET_H

/*********************************************************************/
/* Function ProtoTypes                                               */
/*********************************************************************/

extern int pcnet_ups_get_capabilities(UPSINFO *ups);
extern int pcnet_ups_read_volatile_data(UPSINFO *ups);
extern int pcnet_ups_read_static_data(UPSINFO *ups);
extern int pcnet_ups_kill_power(UPSINFO *ups);
extern int pcnet_ups_check_state(UPSINFO *ups);
extern int pcnet_ups_open(UPSINFO *ups);
extern int pcnet_ups_close(UPSINFO *ups);
extern int pcnet_ups_setup(UPSINFO *ups);
extern int pcnet_ups_program_eeprom(UPSINFO *ups, int command, const char *data);
extern int pcnet_ups_entry_point(UPSINFO *ups, int command, void *data);

#endif   /* _PCNET_H */
