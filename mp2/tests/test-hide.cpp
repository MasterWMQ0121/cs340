#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lib/catch.hpp"

#include "../png-hideGIF.h"
#include "../png-extractGIF.h"

TEST_CASE("png_hideGIF - Hidden PNG chunk layout is correct", "[weight=20][part=4]") {
  system("cp tests/files/test_hide.gif TEST.gif");
  system("cp tests/files/340.png TEST_340.png");

  // Hide the TEST.gif inside of 340.png:
  int result = png_hideGIF("TEST_340.png", "TEST.gif", "TEST_hiddenGIF.png");
  REQUIRE(result == 0);

  PNG* png = PNG_open("TEST_hiddenGIF.png", "r");
  PNG_Chunk* chunks = (PNG_Chunk*)malloc(50 * sizeof(PNG_Chunk));
  
  int chunkCount = 0;
  size_t read_size = PNG_read(png, &chunks[chunkCount++]);
  while (read_size != 0 && chunkCount < 50) {
    read_size = PNG_read(png, &chunks[chunkCount++]);
  };
  PNG_close(png);

  int seenChunk_IHDR = 0, seenChunk_uiuc = 0, seenChunk_IEND = 0;
  for (int i = 0; i < chunkCount - 1; i++) {
    if(strncmp(chunks[i].type, "IHDR", 4) == 0) {
      seenChunk_IHDR = 1;
    }

    if(strncmp(chunks[i].type, "uiuc", 4) == 0) {
      REQUIRE(seenChunk_IHDR == 1);
      seenChunk_uiuc = 1;
    }

    if(strncmp(chunks[i].type, "IEND", 4) == 0) {
      REQUIRE(seenChunk_IHDR == 1);
      REQUIRE(seenChunk_uiuc == 1);
      seenChunk_IEND = 1;
    }
  }

  REQUIRE(seenChunk_IHDR == 1);
  REQUIRE(seenChunk_uiuc == 1);
  REQUIRE(seenChunk_IEND == 1);

  for(int i = 0; i < chunkCount - 1; i++) {
    PNG_free_chunk(&chunks[i]);
  }
  free(chunks);

  system("rm -f TEST_hiddenGIF.gif TEST_340.png TEST.gif");
}


TEST_CASE("png_hideGIF - Round trip with png_extractGIF returns original GIF", "[weight=30][part=4]") {
  system("cp tests/files/test_hide.gif TEST.gif");
  system("cp tests/files/340.png TEST_340.png");

  REQUIRE(png_hideGIF("TEST_340.png", "TEST.gif", "TEST_hiddenGIF.png") == 0);
  REQUIRE(png_extractGIF("TEST_hiddenGIF.png", "TEST_EXTRACT.gif") == 0);

  REQUIRE(system("diff tests/files/test_hide.gif TEST_EXTRACT.gif") == 0);
  system("rm -f TEST_hiddenGIF.gif TEST_EXTRACT.gif TEST_340.png TEST.gif");
}
