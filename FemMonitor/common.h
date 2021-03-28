//
//  common.h
//  FemMonitor
//
//  Created by James Shiffer on 3/27/21.
//  Copyright © 2021 FemboyFinancial. All rights reserved.
//

#ifndef common_h
#define common_h

#include <curl/curl.h>
#include <ncurses.h>


// GRAPHICS //

#define START_ROW       2
#define START_COL       2
#define TITLE_START_COL 15
#define COLORS_SUCCESS  1
#define COLORS_FAILURE  2
#define COLORS_WARNING  3
#define COLORS_GRAPH    4

struct Graph
{
    WINDOW* win;
    int row;
    int col;
    int width;
    int height;
};

void gclear(struct Graph graph);
void gdrawbar(struct Graph graph, int x, int barheight);
void gdrawylabels(struct Graph graph);


// DOWNLOADS //

struct MemoryStruct
{
    char *memory;
    size_t size;
};

/* You must free the returned memory yourself */
struct MemoryStruct geturl(CURL* curl, char* url, CURLcode* res_out);

void on_curl_error(WINDOW* window, CURLcode res);
void on_xml_error(WINDOW* window);


#endif /* common_h */
