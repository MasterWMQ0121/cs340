#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// PNG
struct _PNG {
  // Add any elements you need to store the PNG here:
  FILE *fp;
};
typedef struct _PNG PNG;


// ===
// === Note: Do not edit anything below this line.  This part of the library is fixed
// ===       in order to exist with the provided code using your library.
// ===

// PNG_Chunk
struct _PNG_Chunk {
  uint32_t len;
  char type[5];
  unsigned char *data;
  uint32_t crc;
};
typedef struct _PNG_Chunk PNG_Chunk;

// Reading PNG:
PNG * PNG_open(const char *filename, const char *mode);
size_t PNG_read(PNG *png, PNG_Chunk *chunk);
size_t PNG_write(PNG *png, PNG_Chunk *chunk);
void PNG_free_chunk(PNG_Chunk *chunk);
void PNG_close(PNG *png);

// Exit Codes
extern const int ERROR_INVALID_PARAMS;
extern const int ERROR_INVALID_FILE;
extern const int ERROR_INVALID_CHUNK_DATA;
extern const int ERROR_NO_UIUC_CHUNK;

#ifdef __cplusplus
}
#endif