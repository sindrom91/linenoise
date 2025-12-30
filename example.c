#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "linenoise.h"

void completion(const char *buf, linenoiseCompletions *lc) {
    if (buf[0] == 'h') {
        linenoiseAddCompletion(lc,"hello");
        linenoiseAddCompletion(lc,"hello there");
    }
}

char *hints(const char *buf, int *color, int *bold) {
    if (!_stricmp(buf,"hello")) {
        *color = 35;
        *bold = 0;
        return " World";
    }
    return NULL;
}

int main(int argc, char **argv) {
    char *line;
    char *prgname = argv[0];
    int async = 0;

    /* Parse options, with --multiline we enable multi line editing. */
    while(argc > 1) {
        argc--;
        argv++;
        if (!strcmp(*argv,"--multiline")) {
            linenoiseSetMultiLine(1);
            printf("Multi-line mode enabled.\n");
        } else if (!strcmp(*argv,"--keycodes")) {
            linenoisePrintKeyCodes();
            exit(0);
        } else if (!strcmp(*argv,"--async")) {
            async = 1;
        } else {
            fprintf(stderr, "Usage: %s [--multiline] [--keycodes] [--async]\n", prgname);
            exit(1);
        }
    }

    /* Set the completion callback. This will be called every time the
     * user uses the <tab> key. */
    linenoiseSetCompletionCallback(completion);
    linenoiseSetHintsCallback(hints);

    /* Load history from file. The history file is just a plain text file
     * where entries are separated by newlines. */
    linenoiseHistoryLoad("history.txt"); /* Load the history at startup */

    /* Now this is the main loop of the typical linenoise-based application.
     * The call to linenoise() will block as long as the user types something
     * and presses enter.
     *
     * The typed string is returned as a malloc() allocated string by
     * linenoise, so the user needs to free() it. */

    while(1) {
        if (!async) {
            line = linenoise("hello> ");
            if (line == NULL) break;
        } else {
            /* Asynchronous mode using the multiplexing API: wait for
             * data on stdin, and simulate async data coming from some source
             * using the select(2) timeout. */
            struct linenoiseState ls;
            char buf[1024];
            linenoiseEditStart(&ls,-1,-1,buf,sizeof(buf),"hello> ");
            while(1) {
                HANDLE h = GetStdHandle(STD_INPUT_HANDLE);
                DWORD retval = WaitForSingleObject(h, 1000);
                if (retval == WAIT_OBJECT_0) {
                  INPUT_RECORD record;
                  DWORD read;
                  if (PeekConsoleInput(h, &record, 1, &read) && read > 0) {
                      if (record.EventType == KEY_EVENT && record.Event.KeyEvent.bKeyDown) {
                        line = linenoiseEditFeed(&ls);
                        /* A NULL return means: line editing is continuing.
                         * Otherwise the user hit enter or stopped editing
                         * (CTRL+C/D). */
                        if (line != linenoiseEditMore) break;
                      } else {
                          // It was a mouse move, key release, etc. Flush it and move on.
                          ReadConsoleInput(h, &record, 1, &read);
                      }
                  }
                } else {
                  // Timeout occurred
                  static int counter = 0;
                  linenoiseHide(&ls);
                  printf("Async output %d.\n", counter++);
                  linenoiseShow(&ls);
                }
            }
            linenoiseEditStop(&ls);
            if (line == NULL) exit(0); /* Ctrl+D/C. */
        }

        /* Do something with the string. */
        if (line[0] != '\0' && line[0] != '/') {
            printf("echo: '%s'\n", line);
            linenoiseHistoryAdd(line); /* Add to the history. */
            linenoiseHistorySave("history.txt"); /* Save the history on disk. */
        } else if (!strncmp(line,"/historylen",11)) {
            /* The "/historylen" command will change the history len. */
            int len = atoi(line+11);
            linenoiseHistorySetMaxLen(len);
        } else if (!strncmp(line, "/mask", 5)) {
            linenoiseMaskModeEnable();
        } else if (!strncmp(line, "/unmask", 7)) {
            linenoiseMaskModeDisable();
        } else if (line[0] == '/') {
            printf("Unreconized command: %s\n", line);
        }
        free(line);
    }
    return 0;
}
