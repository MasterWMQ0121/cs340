#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "emoji.h"
#include "emoji-translate.h"


void emoji_init(emoji_t *emoji) {
    emoji->count = 0;
    emoji->source = NULL;
    emoji->translation = NULL;
}

void emoji_add_translation(emoji_t *emoji, const unsigned char *source, const unsigned char *translation) {
    unsigned char *sour = (unsigned char*) malloc(strlen(source) + 1);
    unsigned char *tran = (unsigned char*) malloc(strlen(translation) + 1);
    strcpy(sour, source);
    strcpy(tran, translation);
    emoji->count++;
    emoji->source = (unsigned char**) realloc(emoji->source, emoji->count * sizeof(unsigned char*));
    emoji->translation = (unsigned char**) realloc(emoji->translation, emoji->count * sizeof(unsigned char*));
    emoji->source[emoji->count - 1] = sour;
    emoji->translation[emoji->count - 1] = tran;
}

// Translates the emojis contained in the file `fileName`.
const unsigned char *emoji_translate_file_alloc(emoji_t *emoji, const char *fileName) {
    FILE *file = fopen(fileName, "r");
    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);
    unsigned char *fileContent = (unsigned char *) malloc(fileSize + 1);
    fread(fileContent, 1, fileSize, file);
    fileContent[fileSize] = '\0';
    fclose(file);
    unsigned char *translatedFileContent = replace(emoji, fileContent);
    free(fileContent);
    return translatedFileContent;
}

unsigned char *replace(emoji_t *emoji, unsigned char *string) {
    unsigned char *str = (unsigned char *)malloc(strlen(string) + 1);
    memcpy(str, string, strlen(string));
    str[strlen(string)] = '\0';
    for (unsigned int i = 0; i < strlen(str); i++) {
        int j = i;
        int count = 0;
        while (str[j] == 0xF0 && str[j + 1] == 0x9F && (unsigned)(j + 3) < strlen(str)) {
            j += 4;
            count++;
        }
        int found = 0;
        while (!found && count > 0) {
            unsigned char *substring = (unsigned char *)malloc(j - i + 1);
            memcpy(substring, str + i, j - i);
            substring[j - i] = '\0';
            for (unsigned int k = 0; k < emoji->count; k++) {
                if (strcmp(substring, emoji->source[k]) == 0) {
                    found = 1;
                    break;
                }
            }
            free(substring);
            if (!found) {
                j -= 4;
                count--;
            }
        }
        if (found) {
            unsigned char *substr = (unsigned char *)malloc(j - i + 1);
            memcpy(substr, str + i, j - i);
            substr[j - i] = '\0';
            int k = 0;
            while (strcmp(substr, emoji->source[k]) != 0) {
                k++;
            }
            unsigned char *translation = (unsigned char *)malloc(strlen(emoji->translation[k]) + 1);
            memcpy(translation, emoji->translation[k], strlen(emoji->translation[k]));
            translation[strlen(emoji->translation[k])] = '\0';
            unsigned char *strr = (unsigned char *)malloc(strlen(str) - (j - i) + strlen(translation) + 1);
            memcpy(strr, str, i);
            memcpy(strr + i, translation, strlen(translation));
            memcpy(strr + i + strlen(translation), str + j, strlen(str) - j);
            strr[strlen(str) - (j - i) + strlen(translation)] = '\0';
            free(str);
            free(substr);
            str = strr;
            i += strlen(translation) - 1;
            free(translation);
        }
    }
    return str;
}

void emoji_destroy(emoji_t *emoji) {
    for (unsigned int i = 0; i < emoji->count; i++) {
        free(emoji->source[i]);
        free(emoji->translation[i]);
    }
    free(emoji->source);
    free(emoji->translation);
}
