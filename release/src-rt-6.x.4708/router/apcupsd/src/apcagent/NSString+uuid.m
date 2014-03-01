/*
 * NSString+uuid.m
 *
 * UUID category for NSString
 *
 * Everyone seems to create their own version of this (hint, hint, Apple!)
 * This one is courtesy of Vincent Gable
 * <http://vgable.com/blog/2008/02/24/creating-a-uuid-guid-in-cocoa/>
 */

#include <CoreFoundation/CoreFoundation.h>
#import "NSString+uuid.h"

@implementation NSString(uuid)

+ (NSString*) stringWithUUID {
   CFUUIDRef uuidObj = CFUUIDCreate(nil);
   NSString *uuidString = (NSString*)CFUUIDCreateString(nil, uuidObj);
   CFRelease(uuidObj);
   return [uuidString autorelease];
}

@end
