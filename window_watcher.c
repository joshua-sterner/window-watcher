#include <xcb/xcb.h>
#include <xcb/xcb_image.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <limits.h>
#include <sys/ipc.h>
#include <sys/shm.h>

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

/** Retrieves the screen identified by screen_number. */
xcb_screen_t *getScreen(xcb_connection_t *connection, const xcb_setup_t *setup, int screen_number) {
    xcb_screen_iterator_t screen_iterator = xcb_setup_roots_iterator(setup);
    for (int i = 0; i < screen_number; ++i) {
        xcb_screen_next(&screen_iterator);
    }
    return screen_iterator.data;
}

int main(int argc, char* argv[]) {

    // setup connection
    
    int screen_number;
    xcb_connection_t *connection = xcb_connect(NULL, &screen_number);
    if (xcb_connection_has_error(connection)) {
        fprintf(stderr, "Could not connect to X.");
        return 1;
    }

    const xcb_setup_t *setup = xcb_get_setup(connection);

    xcb_screen_t *screen = getScreen(connection, setup, screen_number);

    uint16_t width = screen->width_in_pixels;
    uint16_t height = screen->height_in_pixels;
    uint8_t screen_depth = screen->root_depth;

    if (screen_depth != 24) {
        fprintf(stderr, "Only 24-bit displays are supported.");
        return 1;
    }

    // Allocating enough space for an alpha channel (assuming a 24-bit display)
    size_t shm_size = width*height*4; //TODO check if space for the alpha channel is actually needed
    // create a new shared memory segment (only accessible by the current user)
    int shm_id = shmget(IPC_PRIVATE, shm_size, 0600);
    if (shm_id == -1) {
        perror("Could not allocate shared memory segment");
        return 1;
    }

    // attach this process to the shared memory segment
    void *shm_address = shmat(shm_id, NULL, 0);
    if (shm_address == (void*) -1) {
        perror("Could not attach to shared memory segment");
        // attempt to mark shm segment for destruction anyways
        shmctl(shm_id, IPC_RMID, NULL); // No point error checking here
        return 1;
    }

    // retrieve source window
    
    // retrieve/setup dest window

    // setup graphics context

    // setup xcb_image_t image and xcb_shm_segment_info_t (attach shm?)

    // loop
    //   retrieve source window dimensions
    //
    //   copy source window image to shm
    //
    //   retrieve dest window dimensions/layout
    //
    //   write source window image to dest window

    if(shmctl(shm_id, IPC_RMID, NULL) == -1) {
        perror("Warning: Attempt to mark shared memory segment for destruction failed");
    }
    
    if (shmdt(shm_address) == -1) {
        perror("Warning: Attempt to detach shared memory segment failed");
    }

    xcb_disconnect(connection);
}
