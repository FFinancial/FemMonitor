//
//  company.h
//  FemMonitor
//
//  Created by James Shiffer on 3/22/21.
//  Copyright © 2021 FemboyFinancial. All rights reserved.
//

#ifndef company_h
#define company_h

#include <ncurses.h>

void company_init(WINDOW* win, int h, int w);
void* company_refresh(void* arg);
/* Note: this function will not free the window! */
void company_destroy(void);

#endif /* company_h */
