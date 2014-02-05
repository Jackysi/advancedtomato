/*
 * upsimage - cgi program to create graphical ups information reports
 *
 * Original Author: Russell Kroll <rkroll@exploits.org>
 *
 * When used together, upsstats and upsimage create interesting looking web
 * pages with graphical representations of the battery capacity, utility
 * voltage, and UPS load. 
 *
 * This program utilizes the gd graphics library for snappy IMG generation.
 * I highly recommend this package for anyone doing similar graphics
 * "on the fly" in C.
 *
 * This binary needs to be installed some place where upsstats can find it.
 *
 * Modified:  Jonathan Benson <jbenson@technologist.com>
 *	    19/6/98 to suit apcupsd
 *	    23/6/98 added more graphs and menu options
 *
 * Modified by Kern Sibbald to include additional graphs as well as
 *  to adapt the graphs to varing conditions (voltage, ...). Also,
 *  consolidate a lot of code into subroutines.
 *
 * Modified by Riccardo Facchetti to support both GIF and PNG formats.
 *
 */

#include "apc.h"

#if !defined(SYS_IMGFMT_PNG) && !defined(SYS_IMGFMT_GIF) && !defined(IMGFMT_GIF)
# error "A graphic file format must be defined to compile this program."
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cgiconfig.h"
#include "cgilib.h"

static char    mycmd[16] = "";
static char    upsval[16] = "";
static char    upsval2[16] = "";
static char    upsval3[16] = "";

static int green, black, white, grey, darkgrey, red;

static void allocate_colors(gdImagePtr im)     
{
    black = gdImageColorAllocate (im, 0, 0, 0);
    green = gdImageColorAllocate (im, 0, 255, 0);
    white = gdImageColorAllocate (im, 255, 255, 255);
    grey = gdImageColorAllocate (im, 200, 200, 200);
    darkgrey = gdImageColorAllocate (im, 50, 50, 50);
    red = gdImageColorAllocate (im, 255, 0, 0);
}

static void DrawTickLines(gdImagePtr im)
{
    gdImageLine (im, 50,  60, 150,  60, darkgrey);
    gdImageLine (im, 50, 120, 150, 120, darkgrey);
    gdImageLine (im, 50, 180, 150, 180, darkgrey);
    gdImageLine (im, 50, 240, 150, 240, darkgrey);
    gdImageLine (im, 50, 300, 150, 300, darkgrey);
}

static void DrawText(gdImagePtr im, int min, int step)
{
    int next;
    char text[10];

    next = min;
    (void) snprintf(text, sizeof(text), "%d", next);
    gdImageString(im, gdFontLarge, 0, 295, (unsigned char *)text, black);

    next += step;
    (void) snprintf(text, sizeof(text), "%d", next);
    gdImageString(im, gdFontLarge, 0, 235, (unsigned char *)text, black);

    next += step;
    (void) snprintf(text, sizeof(text), "%d", next);
    gdImageString(im, gdFontLarge, 0, 175, (unsigned char *)text, black);

    next += step;
    (void) snprintf(text, sizeof(text), "%d", next);
    gdImageString(im, gdFontLarge, 0, 115, (unsigned char *)text, black);

    next += step;
    (void) snprintf(text, sizeof(text), "%d", next);
    gdImageString(im, gdFontLarge, 0, 55, (unsigned char *)text, black);

    next += step;
    (void) snprintf(text, sizeof(text), "%d", next);
    gdImageString(im, gdFontLarge, 0, 0, (unsigned char *)text, black);
}

static gdImagePtr InitImage(void)
{
    gdImagePtr im;

    im = gdImageCreate(150, 350);
    allocate_colors(im);
    gdImageColorTransparent (im, grey);
    gdImageFilledRectangle (im, 0, 0, 150, 350, grey);
    gdImageFilledRectangle (im, 50, 0, 150, 300, green);

    return im;
}


void parsearg(const char *var, const char *value) 
{
    if (strcmp(var, "display") == 0) {
	  strncpy (mycmd, value, sizeof(mycmd));
          mycmd[sizeof(mycmd) - 1] = '\0';

    } else if (strcmp(var, "value") == 0) {
	  strncpy (upsval, value, sizeof(upsval));
          upsval[sizeof(upsval) - 1] = '\0';

    } else if (strcmp(var, "value2") == 0) {
	  strncpy (upsval2, value, sizeof(upsval2));
          upsval2[sizeof(upsval2) - 1] = '\0';

    } else if (strcmp(var, "value3") == 0) {
	  strncpy (upsval3, value, sizeof(upsval3));
          upsval3[sizeof(upsval3) - 1] = '\0';
    }
}


static void imgheader (void)
{
#ifdef SYS_IMGFMT_PNG
    puts ("Content-Type: image/png");
#else
    puts ("Content-Type: image/gif");
#endif
    /*
     * Since this image is generated based on the parameters passed in
     * the URL, caching is acceptable. No need for Cache-Control.
     */
    puts ("");
}

static void TermImage(gdImagePtr im)
{
    DrawTickLines(im);
    imgheader();
#ifdef SYS_IMGFMT_PNG
    gdImagePng (im, stdout);
#else
    gdImageGif (im, stdout);
#endif
    gdImageDestroy (im);
}  

static void drawbattcap(const char *battcaps, const char *minbchgs)
{
    gdImagePtr	    im;
    char	   batttxt[16];
    int 	   battpos;
    double	   battcap;
    int 	   minbchgpos;
    double	   minbchg;

    battcap = strtod(battcaps, NULL);
    minbchg = strtod(minbchgs, NULL);

    im = InitImage();

    DrawText(im, 0, 20);

    minbchgpos = (int)(300 - (minbchg * 3));
    gdImageFilledRectangle(im, 50, minbchgpos, 150, 300, red);

    battpos = (int)(300 - (battcap * 3));
    gdImageFilledRectangle(im, 75, battpos, 125, 300, black);

    (void) snprintf(batttxt, sizeof(batttxt), "%.1f %%", battcap);
    gdImageString(im, gdFontLarge, 70, 320, (unsigned char *)batttxt, black);

    TermImage(im);
}

static void drawbattvolt(const char *battvolts, const char *nombattvs) 
{
    gdImagePtr	    im;
    char	   batttxt[16];
    int 	   battpos;
    int 	   hipos, lowpos;
    double	   battvolt;
    double	   nombattv;
    double	   hip, lowp;	   /* hi and low red line conditions */
    int 	   minv, maxv, deltav;

    im = InitImage();

    battvolt = strtod(battvolts, NULL);
    nombattv = strtod(nombattvs, NULL);

    /* NOTE, if you tweek minv and maxv, ensure that the difference
     * is evenly divisible by 5 or the scales will be wrong!!!	 
     */
    switch ((int)nombattv) {
       case 12:
	  minv = 3;
	  maxv = 18;
	  hip = 12 + 3; 	   /* high redline -- guess */
	  lowp = 12 - 3;	   /* low redline -- guess */
	  break;
       case 24: 
	  minv = 15;
	  maxv = 30;
	  hip = 24 + 5;
	  lowp = 24 - 5;
	  break;
       case 48:
	  minv = 30;
	  maxv = 60;
	  hip = 48 + 7;
	  lowp = 48 - 7;
	  break;
       default:
	  minv = 0;
	  maxv = (int)(battvolt/10 + 1) * 10;
	  hip = battvolt + 5;
	  lowp = battvolt - 5;
	  break;
    }
    deltav = maxv - minv;

    DrawText(im, minv, (deltav)/5);


    /* Do proper scaling of battery voltage and redline positions */
    battpos = (int)(300 - (((battvolt - minv) / deltav ) * 300));
    hipos = (int)( 300 - (((hip - minv) / deltav) * 300) );
    lowpos = (int)( 300 - (((lowp - minv) / deltav) * 300) );

    gdImageFilledRectangle (im, 50, 0, 150, hipos, red);
    gdImageFilledRectangle (im, 50, lowpos, 150, 300, red);


    gdImageFilledRectangle (im, 75, battpos, 125, 300, black);

    (void) snprintf (batttxt, sizeof(batttxt), "%.1f VDC", battvolt);
    gdImageString(im, gdFontLarge, 70, 320, (unsigned char *)batttxt, black);

    TermImage(im);
}

#if 0
static void noimage (void)
{
    gdImagePtr	    im;

    im = gdImageCreate (150, 350);

    allocate_colors(im);

    gdImageColorTransparent (im, grey);

    gdImageFilledRectangle (im, 0, 0, 150, 300, grey);

    gdImageString (im, gdFontLarge, 0, 0, (unsigned char *)"Data not available", black);

    imgheader();
#ifdef SYS_IMGFMT_PNG
    gdImagePng (im, stdout);
#else
    gdImageGif (im, stdout);
#endif
    gdImageDestroy (im);
}
#endif

static void drawupsload(const char *upsloads) 
{
    gdImagePtr	    im;
    char	   loadtxt[16];
    int 	   loadpos;
    double	   upsload;

    upsload = strtod(upsloads, NULL);

    im = InitImage();

    DrawText(im, 0, 25);

    gdImageFilledRectangle (im, 50, 0, 150, 60, red);
    gdImageFilledRectangle (im, 50, 60, 150, 300, green); 

    loadpos = (int)(300 - ((upsload / 125) * 300));
    gdImageFilledRectangle(im, 75, loadpos, 125, 300, black);

    (void) snprintf(loadtxt, sizeof(loadtxt), "%.1f %%", upsload);
    gdImageString(im, gdFontLarge, 70, 320, (unsigned char *)loadtxt, black);

    TermImage(im);
}

/*
 * Input Voltage */
static void drawutility (const char *utilitys, const char *translos,
    const char *transhis) 
{
    gdImagePtr	    im;
    char	   utiltxt[16];
    int 	   utilpos, translopos, transhipos;
    double	   utility, translo, transhi;
    int 	   minv, deltav;

    utility = strtod(utilitys, NULL);
    translo = strtod(translos, NULL);
    transhi = strtod(transhis, NULL);

    im = InitImage();

    if (utility > 180) {	      /* Europe 230V */
       minv = 200;
       deltav = 75;
    } else if (utility > 110) {       /* US 110-120 V */
       minv = 90;
       deltav = 50;
    } else if (utility > 95) {	      /* Japan 100V */
       minv = 80;
       deltav = 50;
    } else {			      /* No voltage */
       minv = 0;
       deltav = 50;
    }

    DrawText(im, minv, deltav/5);
    utilpos = (int)(300 - (((utility - minv) / deltav) * 300) );
    translopos = (int)(300 - (((translo - minv) / deltav) * 300) );
    transhipos = (int)(300 - (((transhi - minv) / deltav) * 300) );

    gdImageFilledRectangle(im, 50, 0, 150, transhipos, red);
    gdImageFilledRectangle(im, 50, translopos, 150, 300, red);

    gdImageFilledRectangle (im, 75, utilpos, 125, 300, black);

    (void) snprintf (utiltxt, sizeof(utiltxt), "%.1f VAC", utility);
    gdImageString (im, gdFontLarge, 65, 320, (unsigned char *)utiltxt, black); 

    TermImage(im);
}

/*
 * Output Voltage
 */
static void drawupsout (const char *upsouts) 
{
    gdImagePtr	    im;
    char	   utiltxt[16];
    int 	   uoutpos;
    double	   upsout;
    int 	   minv, deltav;

    upsout = strtod(upsouts, NULL);

    im = InitImage();

    if (upsout > 180) {
       minv = 200;
       deltav = 75;
    } else if (upsout > 110) {
       minv = 90;
       deltav = 50;
    } else if (upsout > 95) {
       minv = 80;
       deltav = 50;
    } else {
       minv = 0;
       deltav = 50;
    }
    
    DrawText(im, minv, deltav/5);
    uoutpos = (int)(300 - (((upsout - minv) / deltav) * 300) );

    gdImageFilledRectangle(im, 75, uoutpos, 125, 300, black);

    (void) snprintf(utiltxt, sizeof(utiltxt), "%.1f VAC", upsout);
    gdImageString(im, gdFontLarge, 65, 320, (unsigned char *)utiltxt, black); 

    TermImage(im);
}

static void drawruntime (const char *upsrunts, const char *lowbatts)
{
    gdImagePtr	    im;
    char	   utiltxt[16];
    int 	   uoutpos, lowbattpos;
    double	   upsrunt;
    double	   lowbatt;
    int step, maxt;

    upsrunt = strtod(upsrunts, NULL);
    lowbatt = strtod(lowbatts, NULL);

    im = InitImage();

    step = (int)(upsrunt + 4) / 5;
    if (step <= 0)
       step = 1;		   /* make sure we have a positive step */
    DrawText(im, 0, step);

    maxt = step * 5;
    uoutpos = 300 - (int)(upsrunt * 300 ) / maxt;
    lowbattpos = 300 - (int)(lowbatt * 300) / maxt;

    gdImageFilledRectangle(im, 50, lowbattpos, 150, 300, red);

    gdImageFilledRectangle(im, 75, uoutpos, 125, 300, black);

    (void) snprintf(utiltxt, sizeof(utiltxt), "%.1f mins", upsrunt);
    gdImageString(im, gdFontLarge, 65, 320, (unsigned char *)utiltxt, black); 
 
    TermImage(im);
}


int main (int argc, char **argv)
{
#ifdef WIN32
    setmode(fileno(stdout), O_BINARY);
#endif

    (void) extractcgiargs();

    if (strcmp(mycmd, "upsload") == 0) {
	drawupsload(upsval);

    } else if (strcmp(mycmd, "battcap") == 0) {
	drawbattcap(upsval, upsval2);

    } else if (strcmp(mycmd, "battvolt") == 0) {
	drawbattvolt(upsval, upsval2);

    } else if (strcmp(mycmd, "utility") == 0) {
	drawutility(upsval, upsval2, upsval3);

    } else if (strcmp(mycmd, "outputv") == 0) {
	drawupsout(upsval);

    } else if (strcmp(mycmd, "runtime") == 0) {
	drawruntime(upsval, upsval2);

    } else {
        puts("Status: 400 Bad request");
        puts("Content-Type: text/plain; charset=utf-8\n");
        puts("400 Bad request");
	exit(EXIT_FAILURE);
    }
    exit(EXIT_SUCCESS);
}
