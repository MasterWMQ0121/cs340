#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lib/catch.hpp"

#include "../png-extractGIF.h"

TEST_CASE("png_extractGIF - GIF extracted with correct file size (waf)", "[weight=10][part=3]") {
  system("cp tests/files/waf.png TEST.png");

  int result = png_extractGIF("TEST.png", "TEST.gif");
  REQUIRE(result == 0);

  FILE *f = fopen("TEST.gif", "r");
  fseek(f, 0, SEEK_END);
  int length = ftell(f);
  REQUIRE(length == 3044143);

  fclose(f);

  system("rm -f TEST.png TEST.gif");
}

TEST_CASE("png_extractGIF - GIF extracted successfully and content correct (natalia)", "[weight=10][part=3]") {
  system("cp tests/files/natalia.png TEST.png");

  int result = png_extractGIF("TEST.png", "TEST.gif");
  REQUIRE(result == 0);

  REQUIRE(system("diff TEST.gif tests/files/natalia_test.gif") == 0);

  system("rm -f TEST.png TEST.gif");
}

TEST_CASE("png_extractGIF - Fails with error code on a PNG without a uiuc chunk", "[weight=10][part=3]") {
  system("cp tests/files/340.png TEST.png");

  int result = png_extractGIF("TEST.png", "TEST.gif");
  REQUIRE(result != 0);

  system("rm -f TEST.png TEST.gif");
}
