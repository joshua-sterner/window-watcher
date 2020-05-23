#include <xcb/xcb.h>
#include <xcb/xcb_image.h>
#include <xcb/randr.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <limits.h>
#include <sys/ipc.h>
#include <sys/shm.h>

static int shm_id = -1;
static void *shm_address = NULL;

const int MAX_MONITOR_COUNT = 16;

xcb_gcontext_t setupGraphicsContext(xcb_connection_t *connection,
        xcb_window_t window) {
}

typedef struct MonitorInfo {
    int x, y, width, height;
} MonitorInfo;

MonitorInfo* retrieveMonitorInfo(xcb_connection_t *connection, xcb_window_t window, int *monitor_count) {

    xcb_generic_error_t *err;

    xcb_randr_get_screen_resources_current_cookie_t srcc;
    srcc = xcb_randr_get_screen_resources_current(connection, window);

    xcb_randr_get_screen_resources_current_reply_t *srcr;
    srcr = xcb_randr_get_screen_resources_current_reply(connection, srcc, &err);

    xcb_randr_crtc_t *crtcs;
    crtcs = xcb_randr_get_screen_resources_current_crtcs(srcr);
    int crtc_count = xcb_randr_get_screen_resources_current_crtcs_length(srcr);

    MonitorInfo* monitors = malloc(sizeof(MonitorInfo) * crtc_count);

    monitor_count = 0;

    for (int i= 0; i < crtc_count; ++i) {
    
        xcb_randr_get_crtc_info_cookie_t cic;
        cic = xcb_randr_get_crtc_info(connection, crtcs[i], srcr->timestamp);
        xcb_randr_get_crtc_info_reply_t *cir;
        cir = xcb_randr_get_crtc_info_reply(connection, cic, &err);

        if (cir->x != 0 && cir->y != 0) {
            monitors[i].x = cir->x;
            monitors[i].y = cir->y;
            monitors[i].width = cir->width;
            monitors[i].height = cir->height;
            *monitor_count = (*monitor_count) + 1;
        }
    
        //printf("crtc %d: x=%d, y=%d, width=%d, height=%d\n", i, cir->x, cir->y, cir->width, cir->height);

        free(cir);
    }

    free(srcr);
    
    return monitors;
}

typedef struct WindowInfo {
    xcb_window_t window;
    uint8_t depth;
    int16_t x, y;
    uint16_t width, height, border_width;
} WindowInfo;

/**
 * Updates the information about the window in window_info. This requires 
 * window_info to have a valid window member.
 * Returns true if successful, false if unsuccessful or window_info is NULL.*/
bool updateWindowInfo(xcb_connection_t *connection, WindowInfo *window_info) {
    if (window_info == NULL) {
        return false;
    }

    xcb_generic_error_t *error = NULL;

    xcb_get_geometry_cookie_t geometry_cookie = xcb_get_geometry(connection, window_info->window);
    xcb_get_geometry_reply_t *geometry_reply = xcb_get_geometry_reply(connection, geometry_cookie, &error);
    if (error) {
        free(geometry_reply);
        return false;
    }

    window_info->depth = geometry_reply->depth;
    window_info->x = geometry_reply->x;
    window_info->y = geometry_reply->y;
    window_info->width = geometry_reply->width;
    window_info->height = geometry_reply->height;
    window_info->border_width = geometry_reply->border_width;
    
    free(geometry_reply);
    return true;
}

/**
 * Fills in a WindowInfo struct from the root window.
 * Returns true if successful, false otherwise.
 */
bool findRootWindow(xcb_connection_t *connection, xcb_screen_t *screen, WindowInfo *window_info) {
    //TODO Find out if window managers can change the root window
    window_info->window = screen->root;
    return updateWindowInfo(connection, window_info);
}

/**
 * Fills in a WindowInfo struct from the window that currently has focus.
 * Returns true if successfull, false if unsuccessful (for instance, if there
 * is no window which has focus).
 */
bool findFocusedWindow(xcb_connection_t *connection, xcb_screen_t *screen, WindowInfo *window_info) {
    xcb_get_input_focus_cookie_t focus_cookie = xcb_get_input_focus(connection);
    xcb_generic_error_t *error = NULL;
    xcb_get_input_focus_reply_t *focus_reply = xcb_get_input_focus_reply(connection, focus_cookie, &error);
    if (error) {
        free(focus_reply);
        return false;
    }

    window_info->window = focus_reply->focus;
    free(focus_reply);

    return updateWindowInfo(connection, window_info);
}

/** Retrieves the screen identified by screen_number. */
xcb_screen_t *getScreen(xcb_connection_t *connection, const xcb_setup_t *setup, int screen_number) {
    xcb_screen_iterator_t screen_iterator = xcb_setup_roots_iterator(setup);
    for (int i = 0; i < screen_number; ++i) {
        xcb_screen_next(&screen_iterator);
    }
    return screen_iterator.data;
}

void setupShm(size_t shm_size) {
    // create a new shared memory segment (only accessible by the current user)
    shm_id = shmget(IPC_PRIVATE, shm_size, 0600);
    if (shm_id == -1) {
        perror("Could not allocate shared memory segment");
        exit(1);
    }

    // attach this process to the shared memory segment
    shm_address = shmat(shm_id, NULL, 0);
    if (shm_address == (void*) -1) {
        perror("Could not attach to shared memory segment");
        // attempt to mark shm segment for destruction anyways
        shmctl(shm_id, IPC_RMID, NULL); // No point error checking here
        exit(1);
    }
}

void cleanupShm() {
    if(shmctl(shm_id, IPC_RMID, NULL) == -1) {
        perror("Warning: Attempt to mark shared memory segment for destruction failed");
    }
    if (shmdt(shm_address) == -1) {
        perror("Warning: Attempt to detach shared memory segment failed");
    }
}

xcb_image_t setup_image(uint16_t width, uint16_t height, uint8_t *address) {
    xcb_image_t image;
    image.width = width;
    image.height = height;
    image.format = XCB_IMAGE_FORMAT_Z_PIXMAP;
    image.scanline_pad = 8;
    image.depth = 24; //TODO what if depth is not 24?
    image.bpp = 24;
    image.unit = 24;
    image.plane_mask = 0xffffffff;
    image.byte_order = XCB_IMAGE_ORDER_MSB_FIRST;
    image.bit_order = XCB_IMAGE_ORDER_MSB_FIRST;
    image.stride = image.width*(image.bpp/8);
    image.size = image.height * image.stride;
    image.base = NULL;
    image.data = address;
    return image;
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
    // Also allocating enough space for 2 images
    size_t shm_size = 2*width*height*4; //TODO check if space for the alpha channel is actually needed
    setupShm(shm_size);   
    
    WindowInfo source;
    if(!findFocusedWindow(connection, screen, &source)) {
        fprintf(stderr, "Could not find focused window.");
        cleanupShm();
        return 1;
    }
    
    WindowInfo root;
    if (!findRootWindow(connection, screen, &root)) {
        fprintf(stderr, "Could not find root window.");
        cleanupShm();
        return 1;
    }

    //xcb_gcontext_t graphics_context = setupGraphicsContext(connection, source.window);

    xcb_shm_segment_info_t shm_segment_info;
    shm_segment_info.shmseg = xcb_generate_id(connection);
    shm_segment_info.shmid = shm_id;
    shm_segment_info.shmaddr = shm_address;

    xcb_shm_attach(connection, shm_segment_info.shmseg, shm_id, 0);
    
    xcb_image_t root_image = setup_image(root.width, root.height, shm_address);

    xcb_image_t source_image = setup_image(source.width, source.height, shm_address + root_image.size);

    // loop
    
    if (!updateWindowInfo(connection, &source)) {
        fprintf(stderr, "Could not refresh source window dimensions.");
        xcb_shm_detach(connection, shm_segment_info.shmseg);
        cleanupShm();
        return 1;
    }

    if (!xcb_image_shm_get(connection, source.window, &source_image, shm_segment_info, 0, 0, 0xffffffff)) {
        fprintf(stderr, "Could not retrieve source image.");
        xcb_shm_detach(connection, shm_segment_info.shmseg);
        cleanupShm();
        return 1;
    }

    int monitor_count;
    MonitorInfo* monitors = retrieveMonitorInfo(connection, root.window, &monitor_count);

    free(monitors);

    //   write source window image to dest window
    
    xcb_shm_detach(connection, shm_segment_info.shmseg);
    cleanupShm();
    xcb_disconnect(connection);
}
