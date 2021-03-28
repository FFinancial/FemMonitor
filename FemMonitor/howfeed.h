//
//  howfeed.h
//  FemMonitor
//
//  Created by James Shiffer on 3/22/21.
//  Copyright © 2021 FemboyFinancial. All rights reserved.
//

#ifndef howfeed_h
#define howfeed_h

#include <ncurses.h>

void howfeed_init(WINDOW* win, int h, int w);
void* howfeed_refresh(void* arg);
/* Note: this function will not free the window! */
void howfeed_destroy(void);

#endif /* howfeed_h */
