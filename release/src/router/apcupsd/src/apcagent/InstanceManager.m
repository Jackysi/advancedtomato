/*
 * InstanceManager.m
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

#import "InstanceConfig.h"
#import "AppController.h"
#include "LoginItemsAE.h"

@implementation InstanceManager

//******************************************************************************
// PRIVATE helper methods
//******************************************************************************

-(NSURL*)appURL
{
   // Get application URL (10.4 lacks NSBundle::bundleURL so get the path
   // and then convert to a file URL)
   return [NSURL fileURLWithPath:[[NSBundle mainBundle] bundlePath]];
}

-(int)loginItemIndex
{
   // Fetch current user login items
   NSArray *loginItems = NULL;
   OSStatus stat = LIAECopyLoginItems((CFArrayRef*)&loginItems);

   // Search list for our URL
   unsigned int i;
   if (stat == 0)
   {
      NSURL *appUrl = [self appURL];
      for (i = 0; i < [loginItems count]; i++)
      {
         NSURL *url = [[loginItems objectAtIndex:i] objectForKey:(NSString*)kLIAEURL];
         if ([url isEqual:appUrl])
            break;
      }

      if (i == [loginItems count])
         i = -1;

      [loginItems release];
   }
   else
      i = -1;

   return i;
}

- (void) instantiateMonitor:(InstanceConfig*)config
{
   // Instantiate the NIB for this monitor
   NSArray *objs;
   [nib instantiateNibWithOwner:self topLevelObjects:&objs];
   [instmap setObject:objs forKey:[config id]];

   // Locate the AppController object and activate it
   for (unsigned int j = 0; j < [objs count]; j++)
   {
      if ([[objs objectAtIndex:j] isMemberOfClass:[AppController class]])
      {
         AppController *ctrl = [objs objectAtIndex:j];
         [ctrl activateWithConfig:config manager:self];
         break;
      }
   }
}

//******************************************************************************
// PUBLIC methods
//******************************************************************************

- (InstanceManager *)init
{
   self = [super init];
   if (!self) return nil;

   instmap = [[NSMutableDictionary alloc] init];
   nib = [[NSNib alloc] initWithNibNamed:@"MainMenu" bundle:nil];

   [self loginItemIndex];

   return self;
}

- (void)dealloc
{
   [nib release];
   [instmap release];
   [super dealloc];
}

- (void) createMonitors
{
   NSUserDefaults *prefs = [NSUserDefaults standardUserDefaults];
   InstanceConfig *config;

   // Fetch instances array from preferences
   NSArray *instances = [prefs arrayForKey:INSTANCES_PREF_KEY];

   // If instance array does not exist or is empty, add a default entry
   if (!instances || [instances count] == 0)
   {
      config = [InstanceConfig configWithDefaults];
      [config save];
      instances = [prefs arrayForKey:INSTANCES_PREF_KEY];

      // Add login item if not already there
      int idx = [self loginItemIndex];
      if (idx == -1)
         LIAEAddURLAtEnd((CFURLRef)[self appURL], NO);
   }

   // Instantiate monitors
   for (unsigned int i = 0; i < [instances count]; i++)
   {
      config = [InstanceConfig configWithDictionary:[instances objectAtIndex:i]];
      [self instantiateMonitor:config];
   }
}

-(IBAction)add:(id)sender
{
   // Create a new default monitor and save it to prefs
   InstanceConfig *config = [InstanceConfig configWithDefaults];
   [config save];

   // Instantiate the new monitor
   [self instantiateMonitor:config];
}

-(IBAction)remove:(id)sender
{
   NSLog(@"%s:%d %@", __FUNCTION__, __LINE__, [[sender menu] delegate]);

   // Find AppController instance which this menu refers to
   AppController *ac = [[sender menu] delegate];

   // Remove the config from prefs for this monitor
   [InstanceConfig removeConfigWithId:[ac id]];

   // Instruct the instance to close
   [ac close];

   // Remove our reference to the instance and all of its NIB objects
   [instmap removeObjectForKey:[ac id]];

   // Hack! There's an extra reference to AppController *somewhere* but
   // I can't find it. So manually force a release here. This will result
   // in a crash if the mystery reference ever gets used...
   [ac release];

   // If all instances have been removed, terminate the app
   if ([instmap count] == 0)
      [self removeAll:sender];
}

-(IBAction)removeAll:(id)sender
{
   // Remove all instances from preferences
   NSUserDefaults *prefs = [NSUserDefaults standardUserDefaults];
   [prefs removeObjectForKey:INSTANCES_PREF_KEY];

   // Remove user login item
   int idx = [self loginItemIndex];
   if (idx != -1)
      LIAERemove(idx);

   // Terminate the app
   [[NSApplication sharedApplication] terminate:self];
}

-(BOOL)isStartAtLogin
{
   return [self loginItemIndex] != -1;
}

-(IBAction)startAtLogin:(id)sender
{
   int idx = [self loginItemIndex];
   if (idx != -1)
   {
      LIAERemove(idx);
      [sender setState:NSOffState];
   }
   else
   {
      LIAEAddURLAtEnd((CFURLRef)[self appURL], NO);
      [sender setState:NSOnState];
   }
}

@end
