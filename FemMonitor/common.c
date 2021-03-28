//
//  common.c
//  FemMonitor
//
//  Created by James Shiffer on 3/27/21.
//  Copyright © 2021 FemboyFinancial. All rights reserved.
//

#include <stdlib.h>
#include <string.h>

#include "common.h"


static size_t write_mem_callback(void *contents, size_t size, size_t nmemb, void *userp)
{
    size_t realsize = size * nmemb;
    struct MemoryStruct *mem = (struct MemoryStruct *)userp;

    char *ptr = realloc(mem->memory, mem->size + realsize + 1);
    if (ptr == NULL)
    {
        /* out of memory! */
        printf("not enough memory (realloc returned NULL)\n");
        return 0;
    }

    mem->memory = ptr;
    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;

    return realsize;
}

struct MemoryStruct geturl(CURL* curl, char* url, CURLcode* res_out)
{
    curl_easy_reset(curl);
    
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_mem_callback);
    
    struct MemoryStruct data;
    data.memory = malloc(1);   /* will be grown as needed by the realloc above */
    data.size = 0;             /* no data at this point */
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&data);

    CURLcode res = curl_easy_perform(curl);
    if (res_out != NULL)
        *res_out = res;
    return data;
}

void gdrawbar(struct Graph graph, int x, int barheight)
{
    wattron(graph.win, COLOR_PAIR(COLORS_GRAPH));
    for (int i = 0; i < graph.height && i < barheight; ++i)
        mvwprintw(graph.win, graph.row + graph.height - 1 - i, graph.col + x, "*");
    wattroff(graph.win, COLOR_PAIR(COLORS_GRAPH));
}

void gdrawylabels(struct Graph graph)
{
    for (int i = 0; i < graph.height; ++i)
        mvwprintw(graph.win, graph.row + i, graph.col - 1, "%1d", graph.height - i);
}

void gclear(struct Graph graph)
{
    for (int i = 0; i < graph.width; ++i)
        for (int j = 0; j < graph.height; ++j)
            mvwprintw(graph.win, graph.row + j, graph.col + i, " ");
}

void on_curl_error(WINDOW* window, CURLcode res)
{
    wattron(window, COLOR_PAIR(COLORS_FAILURE));
    if (res == CURLE_COULDNT_CONNECT)
    {
        mvwprintw(window, 0, TITLE_START_COL, " OFFLINE ", res);
    }
    else
    {
        mvwprintw(window, 0, TITLE_START_COL, " ERROR ");
        mvwprintw(window, START_ROW, START_COL, "CURL status %d", res);
    }
    wattroff(window, COLOR_PAIR(COLORS_FAILURE));
    wrefresh(window);
}

void on_xml_error(WINDOW* window)
{
    wattron(window, COLOR_PAIR(COLORS_FAILURE));
    mvwprintw(window, 0, TITLE_START_COL, " ERROR ");
    mvwprintw(window, START_ROW, START_COL, "Server gave empty response");
    wattroff(window, COLOR_PAIR(COLORS_FAILURE));
    wrefresh(window);
}
