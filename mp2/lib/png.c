#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>

#include "crc32.h"
#include "png.h"

const int ERROR_INVALID_PARAMS = 1;
const int ERROR_INVALID_FILE = 2;
const int ERROR_INVALID_CHUNK_DATA = 3;
const int ERROR_NO_UIUC_CHUNK = 4;
const unsigned char UIUC_SIGNATURE[8] = { 0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a };


/**
 * Opens a PNG file for reading (mode == "r" or mode == "r+") or writing (mode == "w").
 * 
 * (Note: The function follows the same function prototype as `fopen`.)
 * 
 * When the file is opened for reading this function must verify the PNG signature.  When opened for
 * writing, the file should write the PNG signature.
 * 
 * This function must return NULL on any errors; otherwise, return a new PNG struct for use
 * with further fuctions in this library.
 */
PNG * PNG_open(const char *filename, const char *mode) {
  FILE *fp = fopen(filename, mode);
  if (fp == NULL) {
    return NULL;
  }
  if (strcmp(mode, "r") == 0 || strcmp(mode, "r+") == 0) {
    unsigned char signature[8];
    fread(signature, 1, 8, fp);
    if (memcmp(signature, UIUC_SIGNATURE, 8) != 0) {
      fclose(fp);
      return NULL;
    }
  } else if (strcmp(mode, "w") == 0) {
    fwrite(UIUC_SIGNATURE, 1, 8, fp);
  } else {
    fclose(fp);
    return NULL;
  }
  PNG *png = calloc(sizeof(PNG), 1);
  png->fp = fp;
  return png;
}


/**
 * Reads the next PNG chunk from `png`.
 * 
 * If a chunk exists, a the data in the chunk is populated in `chunk` and the
 * number of bytes read (the length of the chunk in the file) is returned.
 * Otherwise, a zero value is returned.
 * 
 * Any memory allocated within `chunk` must be freed in `PNG_free_chunk`.
 * Users of the library must call `PNG_free_chunk` on all returned chunks.
 */
size_t PNG_read(PNG *png, PNG_Chunk *chunk) {
  size_t size = 0;
  chunk->len = 0;
  size += fread(&chunk->len, sizeof(u_int32_t), 1, png->fp) * sizeof(uint32_t);
  chunk->len = ntohl(chunk->len);
  for (int i = 0; i < 5; i++) {
    chunk->type[i] = '\0';
  }
  size += fread(chunk->type, sizeof(char), 4, png->fp);
  chunk->type[4] = '\0';
  if (chunk->len > 0) {
    chunk->data = calloc(chunk->len, 1);
    size += fread(chunk->data, sizeof(char), chunk->len, png->fp);
  } else {
    chunk->data = NULL;
  }
  size += fread(&chunk->crc, sizeof(u_int32_t), 1, png->fp) * sizeof(uint32_t);
  chunk->crc = ntohl(chunk->crc);
  return size;
}


/**
 * Writes a PNG chunk to `png`.
 * 
 * Returns the number of bytes written. 
 */
size_t PNG_write(PNG *png, PNG_Chunk *chunk) {
  size_t size = 0;
  u_int32_t len = htonl(chunk->len);
  size += fwrite(&len, sizeof(u_int32_t), 1, png->fp) * sizeof(uint32_t);
  size += fwrite(chunk->type, sizeof(char), 4, png->fp);
  size += fwrite(chunk->data, sizeof(char), chunk->len, png->fp);
  u_int32_t crc = 0;
  unsigned char *crc_data = calloc(4 + chunk->len, 1);
  memcpy(crc_data, chunk->type, 4);
  memcpy(crc_data + 4, chunk->data, chunk->len);
  crc32(crc_data, 4 + chunk->len, &crc);
  free(crc_data);
  crc = htonl(crc);
  size += fwrite(&crc, sizeof(u_int32_t), 1, png->fp) * sizeof(uint32_t);
  return size;
}

/**
 * Frees all memory allocated by this library related to `chunk`.
 */
void PNG_free_chunk(PNG_Chunk *chunk) {
  free(chunk->data);
}

/**
 * Closes the PNG file and frees all memory related to `png`.
 */
void PNG_close(PNG *png) {
  fclose(png->fp);
  free(png);
}