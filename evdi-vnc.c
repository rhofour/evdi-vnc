#include <dirent.h>
#include <poll.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>

#include <regex.h>

#include <evdi_lib.h>
#include <rfb/rfb.h>

// *** Constants ***

// Bytes per pixel
#define BPP 4
// Screen width
#define SCREEN_WIDTH 1280
// Screen height
#define SCREEN_HEIGHT 720
// Hardcode an EDID from the Google Autotest project:
// https://chromium.googlesource.com/chromiumos/third_party/autotest/+/master/server/site_tests/display_Resolution/test_data/edids
// Dumped with xxd --include
static const unsigned const char EDID[] = {
  0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x4e, 0x84, 0x5d, 0x00,
  0x01, 0x00, 0x00, 0x00, 0x01, 0x15, 0x01, 0x03, 0x80, 0x31, 0x1c, 0x78,
  0x2a, 0x0d, 0xc9, 0xa0, 0x57, 0x47, 0x98, 0x27, 0x12, 0x48, 0x4c, 0x20,
  0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
  0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x1d, 0x00, 0x72, 0x51, 0xd0,
  0x1e, 0x20, 0x46, 0x28, 0x55, 0x00, 0xe8, 0x12, 0x11, 0x00, 0x00, 0x18,
  0x8c, 0x0a, 0xd0, 0x8a, 0x20, 0xe0, 0x2d, 0x10, 0x10, 0x3e, 0x96, 0x00,
  0xe8, 0x12, 0x11, 0x00, 0x00, 0x18, 0x00, 0x00, 0x00, 0xfc, 0x00, 0x48,
  0x44, 0x4d, 0x49, 0x20, 0x54, 0x56, 0x0a, 0x20, 0x20, 0x20, 0x20, 0x20,
  0x00, 0x00, 0x00, 0xfd, 0x00, 0x31, 0x3d, 0x0f, 0x2e, 0x08, 0x00, 0x0a,
  0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x01, 0x8e, 0x02, 0x03, 0x1d, 0x71,
  0x47, 0x01, 0x02, 0x03, 0x84, 0x11, 0x12, 0x13, 0x23, 0x09, 0x07, 0x07,
  0x83, 0x01, 0x00, 0x00, 0x68, 0x03, 0x0c, 0x00, 0x10, 0x00, 0xb8, 0x2d,
  0x00, 0x01, 0x1d, 0x00, 0x72, 0x51, 0xd0, 0x1e, 0x20, 0x6e, 0x28, 0x55,
  0x00, 0xe8, 0x12, 0x11, 0x00, 0x00, 0x1e, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x39
};
#define N_BUFFERS 1

// *** Globals ***

int connectedClients = 0;
rfbScreenInfoPtr screen;

evdi_handle evdiNode;
bool buffersAllocated = false;
int nextBuffer = 0;
evdi_buffer buffers[N_BUFFERS];
evdi_mode currentMode;

// *** Signal Handler ***
void handleSignal(int signal) {
  if (signal == SIGINT) {
    fprintf(stdout, "Shutting down VNC server.\n");
    rfbShutdownServer(screen, true);
  }
}

// *** EVDI Hooks ***

void dpmsHandler(int dpmsMode, void *userData) {
  fprintf(stdout, "TODO: Handle DPMS mode changes\n");
}

void modeChangedHandler(evdi_mode mode, void *userData) {
  fprintf(stdout, "Mode changed to %dx%d @ %dHz\n", mode.width, mode.height, mode.refresh_rate);
  currentMode = mode;

  // Unregister old buffers if necessary
  if (buffersAllocated) {
    for (int i = 0; i < N_BUFFERS; i++) {
      free(buffers[i].buffer);
      evdi_unregister_buffer(evdiNode, buffers[i].id);
    }
  }
  // Register new buffers for this mode
  for (int i = 0; i < N_BUFFERS; i++) {
    buffers[i].id = i;
    buffers[i].width = mode.width;
    buffers[i].height = mode.height;
    buffers[i].stride = mode.bits_per_pixel/8 * mode.width;
    buffers[i].buffer = malloc(buffers[i].height * buffers[i].stride);
    evdi_register_buffer(evdiNode, buffers[i]);
  }
  buffersAllocated = true;
}

void updateReadyHandler(int bufferId, void *userData) {
  fprintf(stdout, "TODO: Handle EVDI updates\n");
  fprintf(stdout, "Got update on buffer %d\n", bufferId);
}

void crtcStateHandler(int state, void *userData) {
  fprintf(stdout, "TODO: Handle CRTC state changes\n");
}

// *** Other EVDI functions ***

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
    while ((dirEntry = readdir(dir))) {
      if (regexec(&regex, dirEntry->d_name, 0, NULL, 0) == 0) {
        entries++;
      }
    }
  } else {
    fprintf(stderr, "Could not open /sys/class/drm\n");
  }
  return entries;
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
      return i;
    }
  }
  return -1;
}

/* Search and open the first available EVDI node.
 * If none already exists create one and open it.
 * Returns an evdi_handle if successful and EVDI_INVALID_HANDLE if not.
 */
evdi_handle openEvdiNode() {
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

/* Connect to the given EVDI node.
 */
void connectToEvdiNode(evdi_handle nodeHandle) {
  fprintf(stdout, "Sent EDID of size: %lu\t%s\n", sizeof(EDID), EDID);
  evdi_connect(nodeHandle, EDID, sizeof(EDID), NULL, 0);
  fprintf(stdout, "Connected to EVDI node.\n");
}

void disconnectFromEvdiNode(evdi_handle nodeHandle) {
  evdi_disconnect(nodeHandle);
  fprintf(stdout, "Disconnected from EVDI node.\n");
}

// *** VNC Hooks ***
static void clientGone(rfbClientPtr cl)
{
  cl->clientData = NULL;
  connectedClients--;
  if (connectedClients == 0) {
    disconnectFromEvdiNode(evdiNode);
  }
}

static enum rfbNewClientAction newClient(rfbClientPtr client)
{
  if (connectedClients == 0) {
    connectToEvdiNode(evdiNode);
  }
  client->clientGoneHook = clientGone;
  connectedClients++;
  return RFB_CLIENT_ACCEPT;
}


// *** Other VNC functions ***

/* Do initial VNC setup and start the server. 
 * Returns the rfbScreenInfoPtr created.
 */
rfbScreenInfoPtr startVncServer(int argc, char *argv[]) {
  rfbScreenInfoPtr screen = rfbGetScreen(&argc, argv, SCREEN_WIDTH, SCREEN_HEIGHT, 8, 3, BPP);
  if (screen == 0) {
    fprintf(stderr, "Error getting RFB screen.\n");
    return screen;
  }
  screen->frameBuffer = (char*) malloc(SCREEN_WIDTH * SCREEN_HEIGHT * BPP);
  screen->newClientHook = newClient;
  // Set the inital FB to all white.
  memset(screen->frameBuffer, 0xff, SCREEN_WIDTH * SCREEN_HEIGHT * BPP);
  rfbInitServer(screen);
  return screen;
}

void cleanUpVncServer(rfbScreenInfoPtr screen) {
  free(screen->frameBuffer);
  rfbScreenCleanup(screen);
}


int main(int argc, char *argv[]) {
  // Catch Ctrl-C (SIGINT)
  struct sigaction sa;
  sa.sa_handler = handleSignal;
  sigaction(SIGINT, &sa, NULL);

  // Setup EVDI
  evdiNode = openEvdiNode();
  if (evdiNode == EVDI_INVALID_HANDLE) {
    fprintf(stderr, "Failed to connect to an EVDI node.\n");
    return 1;
  }
  evdi_selectable evdiFd = evdi_get_event_ready(evdiNode);
  struct pollfd pollfds[1];
  pollfds[0].fd = evdiFd;
  pollfds[0].events = POLLIN;
  evdi_event_context evdiCtx;
  evdiCtx.dpms_handler = dpmsHandler;
  evdiCtx.mode_changed_handler = modeChangedHandler;
  evdiCtx.update_ready_handler = updateReadyHandler;
  evdiCtx.crtc_state_handler = crtcStateHandler;

  // Start up VNC server
  screen = startVncServer(argc, argv);
  if (screen == 0) {
    fprintf(stderr, "Failed to start VNC server.\n");
    return 1;
  }

  // Run event loop
  fprintf(stdout, "Starting event loop.\n");
  while (rfbIsActive(screen)) {
    // Check for VNC events for 1.0ms
    rfbProcessEvents(screen, 1000);
    // Poll for EVDI updates for 1.0ms
    if (poll(pollfds, 1, 1)) {
      // Figure out which update we received
      evdi_handle_events(evdiNode, &evdiCtx);
    }
  }

  // Clean up
  fprintf(stdout, "Cleaning up...\n");
  cleanUpVncServer(screen);
  evdi_close(evdiNode);

  return 0;
}
