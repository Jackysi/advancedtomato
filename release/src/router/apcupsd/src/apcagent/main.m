/*
 * main.m
 *
 * Apcupsd monitoring applet for Mac OS X
 */

/*
 * Copyright (C) 2009 Adam Kropelin
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of version 2 of the GNU General
 * Public License as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the Free
 * Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
 * MA 02111-1307, USA.
 */

#import <Cocoa/Cocoa.h>
#import <Growl/Growl.h>
#import "InstanceManager.h"

int main(int argc, char *argv[])
{
   NSAutoreleasePool *p = [[NSAutoreleasePool alloc] init];
   [GrowlApplicationBridge setGrowlDelegate:@""];
   [[[InstanceManager alloc] init] createMonitors];
   [p release]; // run is never coming back, so free the pool now
   [[NSApplication sharedApplication] run];
}
