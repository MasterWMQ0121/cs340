#include "png-extractGIF.h"

int png_extractGIF(const char *png_filename, const char *gif_filename) {
  PNG* png = PNG_open(png_filename, "r");
  if (png == NULL) {
    return 1;
  }
  FILE* gif = fopen(gif_filename, "w");
  if (gif == NULL) {
    return 2;
  }
  PNG_Chunk chunk;
  int found = 0;
  while (PNG_read(png, &chunk) != 0) {
    if (strcmp(chunk.type, "uiuc") == 0) {
      found = 1;
      break;
    }
    if (strcmp(chunk.type, "IEND") == 0) {
      PNG_close(png);
      fclose(gif);
      break;
    }
    PNG_free_chunk(&chunk);
  }
  if (found == 0) {
    return 4;
  }
  fwrite(chunk.data, 1, chunk.len, gif);
  fclose(gif);
  PNG_free_chunk(&chunk);
  PNG_close(png);
  return 0; // Change the to a zero to indicate success, when your implementaiton is complete.
}