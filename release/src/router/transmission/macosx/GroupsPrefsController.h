/******************************************************************************
 * $Id: GroupsPrefsController.h 14699 2016-03-02 07:55:37Z mikedld $
 *
 * Copyright (c) 2007-2012 Transmission authors and contributors
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

@interface GroupsPrefsController : NSObject
{
    IBOutlet NSTableView * fTableView;
    IBOutlet NSSegmentedControl * fAddRemoveControl;
    
    IBOutlet NSColorWell * fSelectedColorView;
    IBOutlet NSTextField * fSelectedColorNameField;
    IBOutlet NSButton * fCustomLocationEnableCheck;
    IBOutlet NSPopUpButton * fCustomLocationPopUp;
    
    IBOutlet NSButton * fAutoAssignRulesEnableCheck;
    IBOutlet NSButton * fAutoAssignRulesEditButton;
    
    IBOutlet NSWindow * fGroupRulesSheetWindow;
    IBOutlet NSPredicateEditor * fRuleEditor;

    IBOutlet NSLayoutConstraint * fRuleEditorHeightConstraint;
}

- (void) addRemoveGroup: (id) sender;

- (IBAction) toggleUseCustomDownloadLocation: (id) sender;
- (IBAction) customDownloadLocationSheetShow: (id) sender;

- (IBAction) toggleUseAutoAssignRules: (id) sender;
- (IBAction) orderFrontRulesSheet: (id) sender;
- (IBAction) cancelRules: (id) sender;
- (IBAction) saveRules: (id) sender;
@end
