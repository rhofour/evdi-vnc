#include <stdbool.h>
#include <stdio.h>

#include <evdi_lib.h>
#include <rfb/rfb.h>

// Bytes per pixel
#define BPP 1
// Screen width
#define SCREEN_WIDTH 1280
// Screen height
#define SCREEN_HEIGHT 720

/* Scan through potential EVDI devices until either one is available or we've probed all devices.
 * Return the index of the first available device or -1 if none is found.
 */
int findAvailableEvdiNode() {
  evdi_device_status status = UNRECOGNIZED;
  int i;
  for (i = 0; status == UNRECOGNIZED; i++) {
    status = evdi_check_device(i);
  }
  if (status == AVAILABLE) {
    return(i);
  } else {
    return(-1);
  }
}
/* Search and connect to the first available EVDI node.
 * If none exists create one and connect to it.
 * Returns an an evdi_handle if successful and EVDI_INVALID_HANDLE if not.
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

int main(int argc, char *argv[]) {
  // Setup EVDI
  if (connectToEvdiNode() == EVDI_INVALID_HANDLE) {
    fprintf(stderr, "Failed to connect to an EVDI node.\n");
    return(1);
  }
  // Start up VNC server
  rfbScreenInfoPtr screen = rfbGetScreen(&argc, argv, SCREEN_WIDTH, SCREEN_HEIGHT, 8, 3, BPP);
  screen->frameBuffer = (char*) malloc(SCREEN_WIDTH * SCREEN_HEIGHT * BPP);
  rfbInitServer(screen);
  return(0);
}
