/*
 * InstanceConfig.m
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
#import "NSString+uuid.h"

#define DEFAULT_HOSTNAME   @"127.0.0.1"
#define DEFAULT_PORT       3551
#define DEFAULT_REFRESH    5
#define DEFAULT_POPUPS     YES

@implementation InstanceConfig

+ (InstanceConfig*) configWithDictionary:(NSDictionary*)dict
{
   InstanceConfig *cfg = [[[InstanceConfig alloc] init] autorelease];
   cfg->config = [[NSMutableDictionary dictionaryWithDictionary:dict] retain];
   return cfg;
}

+ (InstanceConfig*) configWithDefaults
{
   InstanceConfig *cfg = [[[InstanceConfig alloc] init] autorelease];
   cfg->config = [[NSMutableDictionary dictionary] retain];

   // Establish defaults
   [cfg->config setObject:[NSString stringWithUUID] forKey:ID_PREF_KEY];
   [cfg->config setObject:DEFAULT_HOSTNAME forKey:HOSTNAME_PREF_KEY];
   [cfg->config setObject:[NSNumber numberWithInt:DEFAULT_PORT] forKey:PORT_PREF_KEY];
   [cfg->config setObject:[NSNumber numberWithInt:DEFAULT_REFRESH] forKey:REFRESH_PREF_KEY];
   [cfg->config setObject:[NSNumber numberWithBool:DEFAULT_POPUPS] forKey:POPUPS_PREF_KEY];

   return cfg;
}

+ (void) removeConfigWithId:(NSString*)id
{
   // Fetch instances array from preferences and make a mutable copy
   NSUserDefaults *prefs = [NSUserDefaults standardUserDefaults];
   NSArray *tmp = [prefs arrayForKey:INSTANCES_PREF_KEY];
   if (!tmp) tmp = [NSArray array];
   NSMutableArray *instances = [NSMutableArray arrayWithArray:tmp];

   // Remove existing config from array
   for (unsigned int i = 0; i < [instances count]; i++)
   {
      InstanceConfig *tmpconfig = 
         [InstanceConfig configWithDictionary:[instances objectAtIndex:i]];

      // If id was found, remove that config
      if ([id isEqualToString:[tmpconfig id]])
         [instances removeObjectAtIndex:i];
   }

   // Write array back to prefs
   [prefs setObject:instances forKey:INSTANCES_PREF_KEY];
   [prefs synchronize];
}

- (void)dealloc
{
   [config release];
   [super dealloc];
}

- (NSString *)host
{
   return [[[config objectForKey:HOSTNAME_PREF_KEY] retain] autorelease];
}

- (int)port
{
   return [(NSNumber*)[config objectForKey:PORT_PREF_KEY] intValue];
}

- (int)refresh
{
   return [(NSNumber*)[config objectForKey:REFRESH_PREF_KEY] intValue];
}

- (BOOL)popups
{
   return [(NSNumber*)[config objectForKey:POPUPS_PREF_KEY] boolValue];
}

- (NSString *)id
{
   return [[[config objectForKey:ID_PREF_KEY] retain] autorelease];
}

- (void)setHost:(NSString *)host
{
   [config setObject:host forKey:HOSTNAME_PREF_KEY];
}

- (void)setPort:(int)port;
{
   [config setObject:[NSNumber numberWithInt:port] forKey:PORT_PREF_KEY];
}

- (void)setRefresh:(int)refresh;
{
   [config setObject:[NSNumber numberWithInt:refresh] forKey:REFRESH_PREF_KEY];
}

- (void)setPopups:(BOOL)popupsEnabled;
{
   [config setObject:[NSNumber numberWithBool:popupsEnabled] forKey:POPUPS_PREF_KEY];
}

- (void) save
{
   // Fetch instances array from preferences and make a mutable copy
   NSUserDefaults *prefs = [NSUserDefaults standardUserDefaults];
   NSArray *tmp = [prefs arrayForKey:INSTANCES_PREF_KEY];
   if (!tmp) tmp = [NSArray array];
   NSMutableArray *instances = [NSMutableArray arrayWithArray:tmp];

   // Search for matching config
   unsigned int i;
   for (i = 0; i < [instances count]; i++)
   {
      InstanceConfig *tmpconfig = 
         [InstanceConfig configWithDictionary:[instances objectAtIndex:i]];

      // If id was found replace with new config
      if ([[self id] isEqualToString:[tmpconfig id]])
      {
         [instances replaceObjectAtIndex:i withObject:config];
         break;
      }
   }

   // If config did not yet exist in array, add it at the end
   if (i == [instances count])
      [instances addObject:config];

   // Write array back to prefs
   [prefs setObject:instances forKey:INSTANCES_PREF_KEY];
   [prefs synchronize];
}

@end
