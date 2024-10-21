#pragma once
#include <stdio.h>
#include <stdlib.h>
#include "lib/png.h"
#include <string.h>
#ifdef __cplusplus
extern "C" {
#endif

int png_hideGIF(const char *png_filename_source, const char *gif_filename, const char *png_filename_out);

#ifdef __cplusplus
}
#endif