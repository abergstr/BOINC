// The contents of this file are subject to the Mozilla Public License
// Version 1.0 (the "License"); you may not use this file except in
// compliance with the License. You may obtain a copy of the License at
// http://www.mozilla.org/MPL/ 
// 
// Software distributed under the License is distributed on an "AS IS"
// basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See the
// License for the specific language governing rights and limitations
// under the License. 
// 
// The Original Code is the Berkeley Open Infrastructure for Network Computing. 
// 
// The Initial Developer of the Original Code is the SETI@home project.
// Portions created by the SETI@home project are Copyright (C) 2002
// University of California at Berkeley. All Rights Reserved. 
// 
// Contributor(s):
//

#include <Carbon/Carbon.h>
#include <CoreServices/CoreServices.h>

#ifdef __cplusplus
extern "C" {
#endif

pascal void mac_graphics_event_loop ( void *data );
pascal void GraphicsLoopProcessor(EventLoopTimerRef inTimer, void* timeData);
pascal void YieldProcessor(EventLoopTimerRef inTimer, void* timeData);
pascal OSStatus MainAppEventHandler(EventHandlerCallRef appHandler, EventRef theEvent, void* appData);

extern MPQueueID drawQueue;
extern int ok_to_draw;

#ifdef __cplusplus
}
#endif
