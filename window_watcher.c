#include <xcb/xcb.h>
#include <xcb/xcb_image.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

const int MAX_MONITOR_COUNT = 16;

xcb_gcontext_t setupGraphicsContext(xcb_connection_t *connection,
        xcb_window_t window) {
}

typedef struct MonitorInfo {
    int x, y, width, height;
} MonitorInfo;

void retrieveMonitorInfo(xcb_connection_t *connection, xcb_screen_t *screen,
        MonitorInfo monitors[], int *monitorCount) {
}

typedef struct WindowInfo {
    xcb_window_t id;
    uint8_t depth;
    int16_t x, y;
    uint16_t width, height, border_width;
} WindowInfo;

WindowInfo findRootWindow(xcb_connection_t *connection, xcb_screen_t *screen) {
}

WindowInfo findSourceWindow(xcb_connection_t *connection, xcb_screen_t *screen) {
}

void updateWindowInfo(xcb_connection_t *connection, WindowInfo *windowInfo) {
}



int main(int argc, char* argv[]) {
    // setup connection
    
    // retrieve screen dimensions

    // retrieve source window
    
    // retrieve/setup dest window

    // setup xcb_image_t image and xcb_shm_segment_info_t 

    // loop
    //   retrieve source window dimensions
    //
    //   copy source window image to shm
    //
    //   retrieve dest window dimensions/layout
    //
    //   write source window image to dest window
    
    // free connection
}
