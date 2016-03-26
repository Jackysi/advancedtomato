/******************************************************************************
 * $Id: InfoWindowController.h 14685 2016-02-27 20:29:57Z mikedld $
 *
 * Copyright (c) 2006-2012 Transmission authors and contributors
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *****************************************************************************/

#import <Cocoa/Cocoa.h>
#import <Quartz/Quartz.h>

@protocol InfoViewController;
@class InfoGeneralViewController;
@class InfoActivityViewController;
@class InfoTrackersViewController;
@class InfoPeersViewController;
@class InfoFileViewController;
@class InfoOptionsViewController;

@interface InfoWindowController : NSWindowController
{
    NSArray * fTorrents;
    
    CGFloat fMinWindowWidth;
    
    NSViewController <InfoViewController> * fViewController;
    NSInteger fCurrentTabTag;
    IBOutlet NSMatrix * fTabMatrix;
    
    InfoGeneralViewController * fGeneralViewController;
    InfoActivityViewController * fActivityViewController;
    InfoTrackersViewController * fTrackersViewController;
    InfoPeersViewController * fPeersViewController;
    InfoFileViewController * fFileViewController;
    InfoOptionsViewController * fOptionsViewController;

    IBOutlet NSImageView * fImageView;
    IBOutlet NSTextField * fNameField, * fBasicInfoField, * fNoneSelectedField;
}

- (void) setInfoForTorrents: (NSArray *) torrents;
- (void) updateInfoStats;
- (void) updateOptions;

- (void) setTab: (id) sender;

- (void) setNextTab;
- (void) setPreviousTab;

- (NSArray *) quickLookURLs;
- (BOOL) canQuickLook;
- (NSRect) quickLookSourceFrameForPreviewItem: (id <QLPreviewItem>) item;

@end
