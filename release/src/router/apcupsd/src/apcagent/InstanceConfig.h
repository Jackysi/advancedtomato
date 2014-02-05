/*
 * InstanceConfig.h
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

#define INSTANCES_PREF_KEY @"Instances"
#define ID_PREF_KEY        @"id"
#define HOSTNAME_PREF_KEY  @"host"
#define PORT_PREF_KEY      @"port"
#define REFRESH_PREF_KEY   @"refresh"
#define POPUPS_PREF_KEY    @"popups"

@interface InstanceConfig: NSObject
{
   NSMutableDictionary *config;
}

+ (InstanceConfig*)configWithDictionary:(NSDictionary*)dict;
+ (InstanceConfig*) configWithDefaults;
+ (void) removeConfigWithId:(NSString*)id;
- (void)dealloc;

- (NSString *)host;
- (int)port;
- (int)refresh;
- (BOOL)popups;
- (NSString *)id;

- (void)setHost:(NSString *)host;
- (void)setPort:(int)port;
- (void)setRefresh:(int)refresh;
- (void)setPopups:(BOOL)popupsEnabled;

- (void)save;

@end

