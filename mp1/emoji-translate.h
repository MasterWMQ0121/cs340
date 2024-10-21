#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _emoji_t {
    unsigned int count;
    unsigned char **source;
    unsigned char **translation;
} emoji_t;

void emoji_init(emoji_t *emoji);
void emoji_add_translation(emoji_t *emoji, const unsigned char *source, const unsigned char *translation);
const unsigned char *emoji_translate_file_alloc(emoji_t *emoji, const char *fileName);
void emoji_destroy(emoji_t *emoji);
unsigned char *replace(emoji_t *emoji, unsigned char *string);

#ifdef __cplusplus
}
#endif
