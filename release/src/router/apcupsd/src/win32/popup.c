/* 
 * Dumb Windows program to put up a message box
 * containing the command line.  Any leading and
 * trailing quotes are stripped.
 * 
 *  Kern E. Sibbald
 *   July MM  
 */
#include "windows.h"
#include <stdio.h>

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, 
		   PSTR szCmdLine, int iCmdShow)
{
   int len = strlen(szCmdLine);
   char *msg, *wordPtr;

   // Funny things happen with the command line if the
   // execution comes from c:/Program Files/apcupsd/apcupsd.exe
   // We get a command line like: Files/apcupsd/apcupsd.exe" options
   // I.e. someone stops scanning command line on a space, not
   // realizing that the filename is quoted!!!!!!!!!!
   // So if first character is not a double quote and
   // the last character before first space is a double
   // quote, we throw away the junk.
   wordPtr = szCmdLine;
   while (*wordPtr && *wordPtr != ' ')
      wordPtr++;
   if (wordPtr > szCmdLine)	 // backup to char before space
      wordPtr--;
   // if first character is not a quote and last is, junk it
   if (*szCmdLine != '"' && *wordPtr == '"') {
      wordPtr++;
      while (*wordPtr && *wordPtr == ' ')
	 wordPtr++;		 /* strip leading spaces */
      szCmdLine = wordPtr;
      len = strlen(szCmdLine);
   }

   msg = szCmdLine;
   if (*szCmdLine == '"' && len > 0 && szCmdLine[len-1] == '"') {
      msg = szCmdLine + 1;
      szCmdLine[len-1] = 0;
   }

   // Now display the popup
   MessageBox(NULL, msg, "Apcupsd message", MB_OK);

   return 0;
}
