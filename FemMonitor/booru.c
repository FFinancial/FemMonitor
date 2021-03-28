//
//  booru.c
//  FemMonitor
//
//  Created by James Shiffer on 3/22/21.
//  Copyright © 2021 FemboyFinancial. All rights reserved.
//

#include <curl/curl.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#include "booru.h"
#include "common.h"

static xmlDoc* doc = NULL;
static WINDOW* window = NULL;
static CURL* curl = NULL;
static float* response_times = NULL;
static int iter = 0;
static int width = 0;
static int height = 0;
static struct Graph graph;

void booru_init(WINDOW* win, int h, int w)
{
    // setup curl
    curl = curl_easy_init();
    width = w;
    height = h;
    window = win;
    
    graph.win = win;
    graph.row = START_ROW + 4;
    graph.col = START_COL + 1;
    graph.width = w - graph.col - 2;
    graph.height = h - graph.row - 1;
    
    mvwprintw(window, 0, START_COL, " Fembooru.jp ");
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

void* booru_refresh(void* arg)
{
    for ( ; ; sleep(1))
    {
        // record the response time
        struct timeval start, end;
        gettimeofday(&start, NULL);
        
        CURLcode res;
        struct MemoryStruct posts_data = geturl(curl, "http://fembooru.jp/api/danbooru/find_posts", &res);
        
        if (res != CURLE_OK)
        {
            on_curl_error(window, res);
            goto cleanup;
        }
        
        struct MemoryStruct tags_data = geturl(curl, "http://fembooru.jp/api/danbooru/find_tags", &res);
        
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
        
        for (int i = START_ROW; i < START_ROW+1; ++i)
        {
            for (int j = START_COL; j < width - 1; ++j)
                mvwprintw(window, i, j, " ");
        }
        
        // clear graph if necessary
        if (index == 0)
            gclear(graph);
        
        // print status graph
        gdrawbar(graph, index, response_times[index] / 100.0f);
        
        // get post count
        doc = xmlReadMemory(posts_data.memory, posts_data.size, "noname.xml", NULL, 0);
        if (doc == NULL)
        {
            on_xml_error(window);
            goto cleanup;
        }
        
        xmlNode* el = xmlDocGetRootElement(doc);
        while (el != NULL && (el->type != XML_ELEMENT_NODE || strcmp(el->name, "posts") != 0))
            el = el->next;
        
        xmlAttr* attr = el == NULL ? NULL : el->properties;
        while (attr != NULL && (attr->type != XML_ATTRIBUTE_NODE || strcmp(attr->name, "count") != 0))
            attr = attr->next;
        
        if (el == NULL || attr == NULL)
        {
            wattron(window, COLOR_PAIR(COLORS_SUCCESS));
            mvwprintw(window, 0, TITLE_START_COL, " ONLINE ");
            wattroff(window, COLOR_PAIR(COLORS_SUCCESS));
            wattron(window, COLOR_PAIR(COLORS_WARNING));
            mvwprintw(window, START_ROW, START_COL, "Couldn't retrieve post count");
            wattroff(window, COLOR_PAIR(COLORS_WARNING));
            wrefresh(window);
            goto cleanup;
        }
        
        // we have arrived at the count attribute of the posts tag
        wattron(window, COLOR_PAIR(COLORS_SUCCESS));
        mvwprintw(window, 0, TITLE_START_COL, " ONLINE ");
        wattroff(window, COLOR_PAIR(COLORS_SUCCESS));
        mvwprintw(window, START_ROW, START_COL, "%s posts", attr->children->content);
        
        // get first 3 tags
        xmlFreeDoc(doc);
        doc = xmlReadMemory(tags_data.memory, tags_data.size, "noname.xml", NULL, 0);
        if (doc == NULL)
        {
            on_xml_error(window);
            goto cleanup;
        }
        
        el = xmlDocGetRootElement(doc)->children->next;
        
        mvwprintw(window, START_ROW+1, START_COL, "Most recent tag:");
        // find tag name
        attr = el->properties;
        while (attr != NULL && (attr->type != XML_ATTRIBUTE_NODE || strcmp(attr->name, "name") != 0))
            attr = attr->next;
        wattron(window, WA_BOLD);
        mvwprintw(window, START_ROW+1, START_COL+16, " %s", attr->children->content);
        wattroff(window, WA_BOLD);
        
        // refresh view
        wrefresh(window);

    cleanup:
        if (doc != NULL)
            xmlFreeDoc(doc);
        free(posts_data.memory);
        free(tags_data.memory);
    }
}

void booru_destroy()
{
    if (curl != NULL)
        curl_easy_cleanup(curl);
    free(response_times);
}
