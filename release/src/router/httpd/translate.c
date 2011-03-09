/*

	Tomato Firmware
	(c) Emmanuel BEZAGU for Victek firmware. 2009.
	(c) Vicente Soriano (Victek) 2011. Tomato-RAF Modifications

*/

#include "tomato.h"
#include <stdio.h>
#include <string.h>

char* get_value(char* inkey)
{
    static  char value[BUFSIZ] = "";
    char filename [50];
    char* web_lang;
    char lang [10];
    web_lang = nvram_get("web_lang");
    if (web_lang == NULL)
    {
        strcpy(lang, "en_EN");
    }
    else
    {
        strcpy(lang, web_lang);
    }
    strcpy(filename, "www/");
    strcat(filename, lang);
    strcat(filename, ".txt");
    FILE *file = fopen(filename, "a+");
    if ( file )
    {
        char line [ BUFSIZ ];
        while ( fgets(line, sizeof line, file) )
        {
            //printf("\nline= %s\n", line);
            int i;
            for (i =0 ; (line[i] != '\0') && (inkey[i] != '\0') ; i++)
            {
                //printf(" line[%d]=%c inkey[%d]=%c\n",i,line[i],i,inkey[i]);
                if (line[i] != inkey[i])
                {
                    //printf(" inkey=%s\n i=%d\n",inkey,i);
                    break;
                }
            }
            if ((inkey[i] == '\0') && (line[i] == '='))
            {
                    if ((line[i] == '=') && (inkey[i] == '\0'))
                    {
                        strncpy(value, &line[i+1], strlen(line) - i - 2);
                        value[strlen(line) - i - 2]='\0';
                        //printf(" return %s\n",value);
                        fclose(file);
                        return value;
                    }
                    else
                    {
                        //printf("  key != inkey\n");
                        break;
                    }
            }
        }
    }
    //printf("append %s=%s\n\n",inkey,inkey);
    fprintf(file,"%s=%s\n",inkey,inkey);
    fclose(file);
    return inkey;
}

char *replace_str(char *str, char *orig, char *rep)
{
  static char buffer[4096];
  char *p;

  if(!(p = strstr(str, orig)))  // Is 'orig' even in 'str'?
    return str;

  strncpy(buffer, str, p-str); // Copy characters from 'str' start to 'orig' st$
  buffer[p-str] = '\0';

  sprintf(buffer+(p-str), "%s%s", rep, p+strlen(orig));

  return buffer;
}

char*   translate(int argc, char **argv)
{
    static  char translated_string[BUFSIZ];

    if (argc == 0)
    {
        return  "";
    }
    else
    {
        strcpy(translated_string,get_value(argv[0]));
        //printf("%s=%s\n",argv[0],translated_string);

        int i=0;
        char idx[5];
        for(i=1;i<argc;i++)
        {
            snprintf(idx,sizeof(idx),"[%d]",i);
            strcpy(translated_string,replace_str(translated_string,idx,argv[i]));
            //printf("%s=%s\n",argv[0],translated_string);
        }

        //printf("%s=%s\n",argv[0],translated_string);
        return  translated_string;
    }
}

void asp_translate(int argc, char **argv)
{
    web_puts(translate(argc,argv));
}

void asp_jstranslate(int argc, char **argv)
{
    char in[BUFSIZ];
    char out[BUFSIZ];
    strcpy(in,translate(argc,argv));
    int i=0;
    int j=0;
    while(in[i]!='\0')
    {
        if(in[i]=='\'')
        {
            out[j++]='\\';
        }
        out[j++]=in[i++];
    }
    out[j]='\0';
    //printf("%s=%s\n",argv[0],out);
    web_puts(out);
}

