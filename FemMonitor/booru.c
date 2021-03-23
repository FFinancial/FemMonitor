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
#include <pthread.h>
#include <string.h>

#include "booru.h"

#define START_ROW   2
#define START_COL   2

xmlDoc* doc = NULL;
WINDOW* window = NULL;
CURL* curl = NULL;
int width = 0;
int height = 0;

struct MemoryStruct
{
    char *memory;
    size_t size;
};
 
static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp)
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

void booru_init(WINDOW* win, int h, int w)
{
    // setup curl
    curl = curl_easy_init();
    width = w;
    height = h;
    
    window = win;
    mvwprintw(window, 0, 1, " Fembooru.jp ");
    mvwprintw(window, START_ROW, START_COL, "Pinging...");
    if (curl == NULL)
    {
        wattron(window, COLOR_PAIR(1));
        mvwprintw(window, START_ROW, START_COL, "ERROR");
        mvwprintw(window, START_ROW+1, START_COL, "Failed to initialize libcurl!");
        wattroff(window, COLOR_PAIR(1));
        curl_global_cleanup();
    }
    wrefresh(window);
}

void* booru_refresh(void* arg)
{
    curl_easy_reset(curl);
    
    struct MemoryStruct chunk;
    chunk.memory = malloc(1);   /* will be grown as needed by the realloc above */
    chunk.size = 0;             /* no data at this point */
    curl_easy_setopt(curl, CURLOPT_URL, "http://fembooru.jp/api/danbooru/find_posts");
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
    
    CURLcode res = curl_easy_perform(curl);
    int* status = malloc(sizeof(int));
    
    // clear status lines
    for (int i = START_ROW; i < START_ROW+1; ++i)
    {
        for (int j = START_COL; j < width - 1; ++j)
            mvwprintw(window, i, j, " ");
    }
    
    if (res != CURLE_OK)
    {
        wattron(window, COLOR_PAIR(2));
        if (res == CURLE_COULDNT_CONNECT)
        {
            mvwprintw(window, START_ROW, START_COL, "OFFLINE", res);
        }
        else
        {
            mvwprintw(window, START_ROW, START_COL, "ERROR");
            mvwprintw(window, START_ROW+1, START_COL, "CURL status %d", res);
        }
        wattroff(window, COLOR_PAIR(2));
        wrefresh(window);
        *status = 1;
        pthread_exit(status);
    }
    
    doc = xmlReadMemory(chunk.memory, chunk.size, "noname.xml", NULL, 0);
    if (doc == NULL)
    {
        wattron(window, COLOR_PAIR(2));
        mvwprintw(window, START_ROW, START_COL, "ERROR");
        mvwprintw(window, START_ROW+1, START_COL, "Server gave empty response");
        wattroff(window, COLOR_PAIR(2));
        wrefresh(window);
        *status = 2;
        pthread_exit(status);
    }
    
    xmlNode* el = xmlDocGetRootElement(doc);
    while (el->type != XML_ELEMENT_NODE || strcmp(el->name, "posts") != 0)
        el = el->next;
    
    el = el->children;
    while (el->type != XML_ATTRIBUTE_NODE || strcmp(el->name, "count") != 0)
        el = el->next;
    
    if (el == NULL)
    {
        wattron(window, COLOR_PAIR(1));
        mvwprintw(window, START_ROW, START_COL, "ONLINE");
        wattroff(window, COLOR_PAIR(1));
        wattron(window, COLOR_PAIR(3));
        mvwprintw(window, START_ROW+1, START_COL, "Couldn't retrieve post count");
        wattroff(window, COLOR_PAIR(3));
        wrefresh(window);
        *status = 3;
        pthread_exit(status);
    }
    
    // we have arrived at the count attribute of the posts tag
    wattron(window, COLOR_PAIR(1));
    mvwprintw(window, START_ROW, START_COL, "ONLINE");
    wattroff(window, COLOR_PAIR(1));
    mvwprintw(window, START_ROW+1, START_COL, "%s posts", el->content);
    wrefresh(window);
    
    *status = 0;
    pthread_exit(status);
}

void booru_destroy()
{
    if (doc != NULL)
        xmlFreeDoc(doc);
    if (curl != NULL)
        curl_easy_cleanup(curl);
}
