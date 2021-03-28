//
//  main.c
//  FemMonitor
//
//  Created by James Shiffer on 3/21/21.
//  Copyright © 2021 FemboyFinancial. All rights reserved.
//

#include <ctype.h>
#include <curl/curl.h>
#include <libxml/parser.h>
#include <ncurses.h>
#include <pthread.h>
#include <time.h>

#include "booru.h"
#include "common.h"
#include "company.h"
#include "howfeed.h"
#include "wiki.h"

#define KEY_QUIT    'q'


WINDOW* create_newwin(int height, int width, int starty, int startx)
{
    WINDOW* local_win;

    local_win = newwin(height, width, starty, startx);
    box(local_win, 0, 0);
    wrefresh(local_win);
    
    return local_win;
}

void destroy_win(WINDOW* local_win)
{
    /* box(local_win, ' ', ' '); : This won't produce the desired
     * result of erasing the window. It will leave its four corners,
     * an ugly remnant of the window.
     */
    wborder(local_win, ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ');
    wrefresh(local_win);
    delwin(local_win);
}

int main(int argc, const char* argv[])
{
    LIBXML_TEST_VERSION
    
    curl_global_init(CURL_GLOBAL_DEFAULT);
    
    // setup ncurses
    initscr();
    halfdelay(20);
    keypad(stdscr, TRUE);
    noecho();
    refresh();
    
    start_color();
    init_pair(COLORS_SUCCESS, COLOR_GREEN, COLOR_BLACK);
    init_pair(COLORS_FAILURE, COLOR_RED, COLOR_BLACK);
    init_pair(COLORS_WARNING, COLOR_YELLOW, COLOR_BLACK);
    init_pair(COLORS_GRAPH, COLOR_CYAN, COLOR_BLACK);
    
    // setup various status windows
    // the bottom windows will be one row shorter to fit the status line at the end
    WINDOW* topleft = create_newwin(LINES/2, COLS/2, 0, 0);
    WINDOW* topright = create_newwin(LINES/2, COLS/2, 0, COLS/2);
    WINDOW* botleft = create_newwin(LINES/2 - 1, COLS/2, LINES/2, 0);
    WINDOW* botright = create_newwin(LINES/2 - 1, COLS/2, LINES/2, COLS/2);
    
    company_init(topleft, LINES/2, COLS/2);
    howfeed_init(topright, LINES/2, COLS/2);
    booru_init(botleft, LINES/2 - 1, COLS/2);
    mvwprintw(botright, 0, 1, " FemWiki ");
    wrefresh(botright);

    // start refresh status threads
    pthread_t booru_th;
    pthread_create(&booru_th, NULL, booru_refresh, NULL);
    pthread_t howfeed_th;
    pthread_create(&howfeed_th, NULL, howfeed_refresh, NULL);
    pthread_t company_th;
    pthread_create(&company_th, NULL, company_refresh, NULL);
    
    time_t t;
    struct tm tm;
    char ch;
    while ((ch = tolower(getch())) != KEY_QUIT)
    {
        time(&t);
        tm = *localtime(&t);
        mvprintw(LINES - 1, 0, "Last updated: %d-%02d-%02d %02d:%02d:%02d\n",
                 tm.tm_year + 1900,
                 tm.tm_mon + 1,
                 tm.tm_mday,
                 tm.tm_hour,
                 tm.tm_min,
                 tm.tm_sec);
        mvprintw(LINES - 1, COLS - 15, "Press Q to quit");
        wrefresh(stdscr);
    }
    
    // cleanup everything
    pthread_cancel(booru_th);
    booru_destroy();
    
    pthread_cancel(howfeed_th);
    howfeed_destroy();
    
    pthread_cancel(company_th);
    company_destroy();
    
    destroy_win(topleft);
    destroy_win(topright);
    destroy_win(botleft);
    destroy_win(botright);
    
    endwin();
    xmlCleanupParser();
    curl_global_cleanup();
    return 0;
}
