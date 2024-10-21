#pragma once
#include <stdio.h>
#include <stdlib.h>
#include "lib/png.h"
#include <string.h>
#include <errno.h>
#ifdef __cplusplus
extern "C" {
#endif

int png_extractGIF(const char *png_filename, const char *gif_filename);

#ifdef __cplusplus
}
#endif