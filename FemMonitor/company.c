//
//  company.c
//  FemMonitor
//
//  Created by James Shiffer on 3/22/21.
//  Copyright © 2021 FemboyFinancial. All rights reserved.
//

#include <curl/curl.h>
#include <libxml/HTMLparser.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>
#include <string.h>
#include <unistd.h>

#include "common.h"
#include "company.h"


static CURL* curl = NULL;
static WINDOW* window = NULL;
static htmlDocPtr doc = NULL;
static xmlXPathContext* ctx = NULL;
static xmlXPathObject* xpathObj = NULL;
static int height;
static int width;

void company_init(WINDOW* win, int h, int w)
{
    curl = curl_easy_init();
    window = win;
    height = h;
    width = w;
    
    mvwprintw(window, 0, START_COL, " Company ");
    if (curl == NULL)
    {
        wattron(window, COLOR_PAIR(COLORS_FAILURE));
        mvwprintw(window, 0, TITLE_START_COL, " ERROR ");
        mvwprintw(window, START_ROW, START_COL, "Failed to initialize libcurl!");
        wattroff(window, COLOR_PAIR(COLORS_FAILURE));
        curl_global_cleanup();
    }
    
    wrefresh(window);
}

// I don't need a whole library to parse a json file that only holds one thing lul
bool parse_meet_time(const char* json, struct tm* time)
{
    const char* prop = "\"LastMeetingTime\":\"";
    json = strstr(json, prop);
    if (json == NULL)
        return FALSE;
    json += strlen(prop);
    strptime(json, "%Y-%m-%dT%H:%M:%S.", time);
    return TRUE;
}

void* company_refresh(void* arg)
{
    for ( ; ; sleep(5))
    {
        CURLcode res;
        // check if femfinancial is up
        struct MemoryStruct ff_data = geturl(curl, "http://ff.howfeed.biz", &res);
        
        // get random reference from booru
        struct MemoryStruct booru_data = geturl(curl, "http://fembooru.jp", NULL);
        
        // get most recent meeting time
        struct MemoryStruct meet_data = geturl(curl, "http://howfeed.biz/api/meet?token=1445", NULL);
        
        doc = htmlReadMemory(booru_data.memory, booru_data.size, "nopath.xml", NULL, HTML_PARSE_NOERROR | HTML_PARSE_NOWARNING);
        if (doc == NULL)
        {
            on_xml_error(window);
            goto cleanup;
        }
        
        /* Create xpath evaluation context */
        ctx = xmlXPathNewContext(doc);
        if (ctx == NULL)
        {
            on_xml_error(window);
            goto cleanup;
        }
        
        xpathObj = xmlXPathEvalExpression("//div[@id=\"tips\"]", ctx);
        if (xpathObj == NULL)
        {
            on_xml_error(window);
            goto cleanup;
        }
        
        htmlNodePtr div = xpathObj->nodesetval->nodeTab[0];
        if (div == NULL)
        {
            on_xml_error(window);
            goto cleanup;
        }
        else
        {
            char* tagline = div->children->content;
            // clear line
            for (int i = START_COL; i < width - 1; ++i)
                mvwprintw(window, START_ROW+5, i, " ");
            // print the funny reference
            mvwprintw(window, START_ROW+5, START_COL, "%s", tagline);
        }
        
        
        struct tm lastmeeting;
        mvwprintw(window, START_ROW+1, START_COL, "Last meeting:");
        wattron(window, WA_BOLD);
        if (parse_meet_time(meet_data.memory, &lastmeeting))
        {
            double diff_min = difftime(time(NULL), timegm(&lastmeeting)) / 60.0;
            double diff_hr = diff_min / 60.0;
            mvwprintw(window, START_ROW+1, START_COL+13, " %.0fh %.0fm ago", diff_hr, diff_min);
        }
        else
        {
            mvwprintw(window, START_ROW+1, START_COL+13, " a long time ago");
        }
        wattroff(window, WA_BOLD);
        
        mvwprintw(window, START_ROW+2, START_COL, "FemboyFinancial website:");
        if (res == CURLE_OK)
        {
            wattron(window, COLOR_PAIR(COLORS_SUCCESS));
            mvwprintw(window, START_ROW+2, START_COL+24, " ONLINE");
            wattroff(window, COLOR_PAIR(COLORS_SUCCESS));
        }
        else
        {
            wattron(window, COLOR_PAIR(COLORS_FAILURE));
            mvwprintw(window, START_ROW+2, START_COL+24, " OFFLINE");
            wattroff(window, COLOR_PAIR(COLORS_FAILURE));
        }
        
        wrefresh(window);
        
    cleanup:
        if (xpathObj != NULL)
            xmlXPathFreeObject(xpathObj);
        if (ctx != NULL)
            xmlXPathFreeContext(ctx);
        if (doc != NULL)
            xmlFreeDoc(doc);
        free(ff_data.memory);
        free(booru_data.memory);
        free(meet_data.memory);
    }
}

void company_destroy()
{
    if (curl != NULL)
        curl_easy_cleanup(curl);
}
