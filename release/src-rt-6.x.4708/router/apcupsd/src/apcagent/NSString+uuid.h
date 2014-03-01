/*
 * NSString+uuid.h
 *
 * UUID category for NSString
 *
 * Everyone seems to create their own version of this (hint, hint, Apple!)
 * This one is courtesy of Vincent Gable
 * <http://vgable.com/blog/2008/02/24/creating-a-uuid-guid-in-cocoa/>
 */

#import <Cocoa/Cocoa.h>

@interface NSString(uuid)

+ (NSString *)stringWithUUID;

@end
