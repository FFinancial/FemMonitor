//
//  howfeed.c
//  FemMonitor
//
//  Created by James Shiffer on 3/22/21.
//  Copyright © 2021 FemboyFinancial. All rights reserved.
//

#include <curl/curl.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#include "common.h"
#include "howfeed.h"

static xmlDoc* doc = NULL;
static WINDOW* window = NULL;
static CURL* curl = NULL;
static struct Graph graph;
static float* response_times;
static int width;
static int height;

void howfeed_init(WINDOW* win, int h, int w)
{
    curl = curl_easy_init();
    window = win;
    width = w;
    height = h;
    
    graph.win = win;
    graph.row = START_ROW + 5;
    graph.col = START_COL + 1;
    graph.width = w - graph.col - 2;
    graph.height = h - graph.row - 1;
    
    mvwprintw(window, 0, START_COL, " Howfeed.biz ");
    mvwprintw(window, START_ROW, START_COL, "Pinging...");
    mvwprintw(window, graph.row - 1, START_COL, "Response time (x100ms)");
    if (curl == NULL)
    {
        wattron(window, COLOR_PAIR(COLORS_FAILURE));
        mvwprintw(window, 0, TITLE_START_COL, " ERROR ");
        mvwprintw(window, START_ROW, START_COL, "Failed to initialize libcurl!");
        wattroff(window, COLOR_PAIR(COLORS_FAILURE));
        curl_global_cleanup();
    }
    
    response_times = malloc(sizeof(float) * graph.width);
    
    gdrawylabels(graph);
    wrefresh(window);
}

void* howfeed_refresh(void* arg)
{
    int iter = 0;
    for ( ; ; sleep(1))
    {
        // record the response time
        struct timeval start, end;
        gettimeofday(&start, NULL);
        
        CURLcode res;
        struct MemoryStruct article_data = geturl(curl, "http://howfeed.biz/rss.xml", &res);
        
        if (res != CURLE_OK)
        {
            on_curl_error(window, res);
            goto cleanup;
        }
        
        gettimeofday(&end, NULL);
        int index = iter++ % graph.width;
        response_times[index] = (end.tv_sec - start.tv_sec) * 1000.0f + (end.tv_usec - start.tv_usec) / 1000.0f;
        
        // clear status lines
        for (int i = TITLE_START_COL - 1; i < TITLE_START_COL + 7; ++i)
            mvwprintw(window, 0, i, " ");
        
        for (int i = START_ROW; i < START_ROW+2; ++i)
        {
            for (int j = START_COL; j < width - 1; ++j)
                mvwprintw(window, i, j, " ");
        }
        
        // clear graph if necessary
        if (index == 0)
            gclear(graph);
        
        // print status graph
        gdrawbar(graph, index, response_times[index] / 100.0f);
        
        // get article count
        doc = xmlReadMemory(article_data.memory, article_data.size, "noname.xml", NULL, 0);
        if (doc == NULL)
        {
            on_xml_error(window);
            goto cleanup;
        }
        
        xmlNode* el = xmlDocGetRootElement(doc)->children->children;
        while (el != NULL && (el->type != XML_ELEMENT_NODE || strcmp(el->name, "item") != 0))
            el = el->next;
        
        int articles_count = 0;
        while (el != NULL && el->next != NULL && el->next->type == XML_ELEMENT_NODE && strcmp(el->next->name, "item") == 0)
        {
            ++articles_count;
            el = el->next;
        }
        ++articles_count;
        
        if (el != NULL)
            el = el->children;
        
        while (el != NULL && (el->type != XML_ELEMENT_NODE || strcmp(el->name, "title") != 0))
            el = el->next;
        
        char* title;
        if (el != NULL)
            title = el->children->content;
        
        while (el != NULL && (el->type != XML_ELEMENT_NODE || strcmp(el->name, "pubDate") != 0))
            el = el->next;
        
        struct tm pubdate;
        if (el != NULL)
            strptime(el->children->content, "%a, %d %b %Y %H:%M:%S %Z", &pubdate);

        wattron(window, COLOR_PAIR(COLORS_SUCCESS));
        mvwprintw(window, 0, TITLE_START_COL, " ONLINE ");
        wattroff(window, COLOR_PAIR(COLORS_SUCCESS));
        
        if (el == NULL)
        {
            wattron(window, COLOR_PAIR(COLORS_WARNING));
            mvwprintw(window, START_ROW, START_COL, "Failed to get latest article");
            wattroff(window, COLOR_PAIR(COLORS_WARNING));
        }
        else
        {
            mvwprintw(window, START_ROW, START_COL, "%d articles", articles_count);
            mvwprintw(window, START_ROW+1, START_COL, "Latest article:");
            wattron(window, WA_BOLD);
            mvwprintw(window, START_ROW+1, START_COL+15, " %s", title);
            wattroff(window, WA_BOLD);
            
            double days_elapsed = difftime(time(NULL), timelocal(&pubdate)) / (3600.0 * 24.0);
            
            int colorpair;
            if (days_elapsed < 7)
                colorpair = COLORS_SUCCESS;
            else if (days_elapsed < 14)
                colorpair = COLORS_WARNING;
            else
                colorpair = COLORS_FAILURE;
            
            wattron(window, WA_BOLD);
            wattron(window, COLOR_PAIR(colorpair));
            mvwprintw(window, START_ROW+2, START_COL, "%.0f days", days_elapsed);
            wattroff(window, COLOR_PAIR(colorpair));
            wattroff(window, WA_BOLD);
            
            mvwprintw(window, START_ROW+2, START_COL+8, " since last article");
        }
        
        // refresh view
        wrefresh(window);

    cleanup:
        if (doc != NULL)
            xmlFreeDoc(doc);
        free(article_data.memory);
    }
}

void howfeed_destroy(void)
{
    if (curl != NULL)
        curl_easy_cleanup(curl);
    free(response_times);
}
