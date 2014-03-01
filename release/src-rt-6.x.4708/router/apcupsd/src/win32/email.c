/*
 * Dumb program to send an email message
 *
 * Called:
 *   email -s "Subject" address@somewhere.com -m "Body of text"
 *
 *   Kern Sibbald, October MM 
 */

#include   <windows.h>
#include   <mapi.h>
#include   <stdio.h>

#ifndef HAVE_MINGW
extern void mainCRTStartup();
void WinMainCRTStartup() { mainCRTStartup(); }
#endif

int main(int argc, char **argv)
{
   int i;
   ULONG err;
   MapiRecipDesc recip;
   char addr[100];
   MapiMessage emsg = {0, 
      "Apcupsd message",               /* default subject */
      "No text specified.\n",          /* default message text */
      NULL, NULL, NULL, 0, NULL,
      1, &recip, 0, NULL};

   recip.ulReserved = 0;
   recip.ulRecipClass = MAPI_TO;
   recip.lpszName = "root";           /* default name */
   recip.lpszAddress = "SMTP:root";   /* default address */
   recip.ulEIDSize = 0;
   recip.lpEntryID = NULL;

   for (i=1; i<argc; i++) {
      if (strcmp(argv[i], "-s") == 0) {        /* Subject */
         if (++i < argc)
            emsg.lpszSubject = argv[i];
      } else if (strcmp(argv[i], "-m") == 0) { /* Message text */
         if (++i < argc)
            emsg.lpszNoteText = argv[i];
      } else {				       /* address */
         strncpy(addr, "SMTP:", sizeof(addr));
         strncat(addr, argv[i], sizeof(addr));
         recip.lpszAddress = addr;
         recip.lpszName = argv[i];
      }
   }

   err = MAPISendMail(0L, 0L, &emsg, 0L, 0L);

   if (err != SUCCESS_SUCCESS) {
      char buf[100];
      snprintf(buf, sizeof(buf), "MAPI error code = %d", (int)err);

// Note, if we put up a dialogue box, this may stall the
// calling script, not a good thing.
//    MessageBox(NULL, buf, "Error", MB_OK);
      printf(buf);
      exit(1);
   }
   exit(0);
}
