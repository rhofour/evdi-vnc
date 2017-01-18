#include <dirent.h>
#include <stdbool.h>
#include <stdio.h>

#include <regex.h>

#include <evdi_lib.h>
#include <rfb/rfb.h>

// Bytes per pixel
#define BPP 4
// Screen width
#define SCREEN_WIDTH 1280
// Screen height
#define SCREEN_HEIGHT 720
// Hardcode an EDID from the Google Autotest project:
// https://chromium.googlesource.com/chromiumos/third_party/autotest/+/master/server/site_tests/display_Resolution/test_data/edids
static const char EDID[] = "f0ddf85ec47228bd9d3ea17a8932e07b841ce6db";

/* Count the number of cardX files in /sys/class/drm. */
int countCardEntries() {
  // Prepare regex to match cardX.
  regex_t regex;
  if (regcomp(&regex, "^card[0-9]*$", REG_NOSUB)) {
    fprintf(stderr, "Could not compile card regex.\n");
    return 0;
  }

  // Scan through /sys/class/drm
  int entries = 0;
  DIR *dir = opendir("/sys/class/drm");
  if (dir != NULL) {
    struct dirent *dirEntry;
    while (dirEntry = readdir(dir)) {
      if (regexec(&regex, dirEntry->d_name, 0, NULL, 0) == 0) {
        entries++;
      }
    }
  } else {
    fprintf(stderr, "Could not open /sys/class/drm\n");
  }
  return(entries);
}
/* Scan through potential EVDI devices until either one is available or we've probed all devices.
 * Return the index of the first available device or -1 if none is found.
 */
int findAvailableEvdiNode() {
  evdi_device_status status = UNRECOGNIZED;
  int i;
  int nCards = countCardEntries();
  for (i = 0; i < nCards; i++) {
    status = evdi_check_device(i);
    if (status == AVAILABLE) {
      return(i);
    }
  }
  return(-1);
}
/* Search and connect to the first available EVDI node.
 * If none exists create one and connect to it.
 * Returns an evdi_handle if successful and EVDI_INVALID_HANDLE if not.
 */
evdi_handle connectToEvdiNode() {
  // First, find an available node
  int nodeIndex = findAvailableEvdiNode();
  if (nodeIndex == -1) {
    // Create a new node instead
    if (evdi_add_device() == 0) {
      fprintf(stderr, "Failed to create a new EVDI node.\n");
      return EVDI_INVALID_HANDLE;
    }
    nodeIndex = findAvailableEvdiNode();
    if (nodeIndex == -1) {
      fprintf(stderr, "Failed to find newly created EVDI node.\n");
      return EVDI_INVALID_HANDLE;
    }
  }

  // Next, connect to the node we found
  evdi_handle nodeHandle = evdi_open(nodeIndex);
  if (nodeHandle == EVDI_INVALID_HANDLE) {
    fprintf(stderr, "Failed to open EVDI node: %d\n", nodeIndex);
    return EVDI_INVALID_HANDLE;
  }
  return nodeHandle;
}

/* Do initial VNC setup and start the server. 
 * Returns the rfbScreenInfoPtr created.
 */
rfbScreenInfoPtr startVncServer(int argc, char *argv[]) {
  rfbScreenInfoPtr screen = rfbGetScreen(&argc, argv, SCREEN_WIDTH, SCREEN_HEIGHT, 8, 3, BPP);
  if (screen == 0) {
    fprintf(stderr, "Error getting RFB screen.\n");
    return(screen);
  }
  screen->frameBuffer = (char*) malloc(SCREEN_WIDTH * SCREEN_HEIGHT * BPP);
  // Set the inital FB to all white.
  memset(screen->frameBuffer, 0xff, SCREEN_WIDTH * SCREEN_HEIGHT * BPP);
  rfbInitServer(screen);
  return(screen);
}

int main(int argc, char *argv[]) {
  // Setup EVDI
  evdi_handle evdiNode = connectToEvdiNode();
  if (evdiNode == EVDI_INVALID_HANDLE) {
    fprintf(stderr, "Failed to connect to an EVDI node.\n");
    return(1);
  }
  // Start up VNC server
  rfbScreenInfoPtr screen = startVncServer(argc, argv);
  if (screen == 0) {
    fprintf(stderr, "Failed to start VNC server.\n");
    return(1);
  }

  // Clean up
  evdi_close(evdiNode);
  return(0);
}
