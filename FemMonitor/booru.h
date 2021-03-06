//
//  booru.h
//  FemMonitor
//
//  Created by James Shiffer on 3/22/21.
//  Copyright © 2021 FemboyFinancial. All rights reserved.
//

#ifndef booru_h
#define booru_h

#include <ncurses.h>

void booru_init(WINDOW* win, int h, int w);
void* booru_refresh(void* arg);
/* Note: this function will not free the window! */
void booru_destroy(void);

#endif /* booru_h */
