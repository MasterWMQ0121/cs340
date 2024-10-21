#include "png-hideGIF.h" 

int png_hideGIF(const char *png_filename_source, const char *gif_filename, const char *png_filename_out) {
  PNG* png = PNG_open(png_filename_source, "r");
  if (!png) { return ERROR_INVALID_FILE; }
  
  FILE* gif = fopen(gif_filename, "r");
  fseek(gif, 0, SEEK_END);
  size_t gif_size = ftell(gif);
  fseek(gif, 0, SEEK_SET);
  char* gif_data = malloc(gif_size);
  fread(gif_data, 1, gif_size, gif);
  fclose(gif);
  
  PNG *out = PNG_open(png_filename_out, "w");
  if (!out) { return ERROR_INVALID_FILE; }

  size_t bytesWritten;
  printf("PNG Header written.\n");

  while (1) {
    PNG_Chunk chunk;
    if (PNG_read(png, &chunk) == 0) {
      PNG_close(png);
      PNG_close(out);
      return ERROR_INVALID_CHUNK_DATA;
    }

    bytesWritten = PNG_write(out, &chunk);
    printf("PNG chunk %s written (%lu bytes)\n", chunk.type, bytesWritten);

    if (strcmp(chunk.type, "IHDR") == 0) {
      PNG_Chunk uiuc;
      uiuc.type[0] = 'u';
      uiuc.type[1] = 'i';
      uiuc.type[2] = 'u';
      uiuc.type[3] = 'c';
      uiuc.type[4] = '\0';
      uiuc.len = gif_size;
      uiuc.data = gif_data;
      uiuc.crc = 0;

      bytesWritten = PNG_write(out, &uiuc);
      printf("PNG chunk %s written (%lu bytes)\n", uiuc.type, bytesWritten);
      PNG_free_chunk(&uiuc);
    }
    
    if (strcmp(chunk.type, "IEND") == 0) {
      PNG_free_chunk(&chunk);
      break;  
    }
    PNG_free_chunk(&chunk);
  }
  PNG_close(out);
  PNG_close(png);
  return 0;  // Change the to a zero to indicate success, when your implementaiton is complete.
}
