#include <rfb/rfb.h>

// Bytes per pixel
#define BPP 1
// Screen width
#define SCREEN_WIDTH 1280
// Screen height
#define SCREEN_HEIGHT 720

int main(int argc, char *argv[]) {
  rfbScreenInfoPtr screen = rfbGetScreen(&argc, argv, SCREEN_WIDTH, SCREEN_HEIGHT, 8, 3, BPP);
  screen->frameBuffer = (char*) malloc(SCREEN_WIDTH * SCREEN_HEIGHT * BPP);
  rfbInitServer(screen);
  return(0);
}
