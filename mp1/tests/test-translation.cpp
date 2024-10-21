#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../emoji-translate.h"
#include "lib/catch.hpp"


TEST_CASE("translate - translation works with an empty emoji_t", "[weight=3][part=2]") {
  emoji_t emoji;
  emoji_init(&emoji);

  unsigned char *translation = (unsigned char *) emoji_translate_file_alloc(&emoji, "tests/txt/simple.txt");
  REQUIRE(translation != NULL);

  INFO("translation := " << translation);
  REQUIRE(strcmp((char *) translation, "I 🧡💙 Illinois!") == 0);
  free(translation);

  emoji_destroy(&emoji);
}

TEST_CASE("translate - translation with one translation", "[weight=3][part=2]") {
  emoji_t emoji;
  emoji_init(&emoji);

  emoji_add_translation(&emoji, (const unsigned char *) "🧡", (const unsigned char *) "heart");

  unsigned char *translation = (unsigned char *) emoji_translate_file_alloc(&emoji, "tests/txt/simple.txt");
  REQUIRE(translation != NULL);

  INFO("translation := " << translation);
  REQUIRE(strcmp((char *) translation, "I heart💙 Illinois!") == 0);
  free(translation);

  emoji_destroy(&emoji);
}

TEST_CASE("translate - translation with two translations", "[weight=3][part=2]") {
  emoji_t emoji;
  emoji_init(&emoji);

  emoji_add_translation(&emoji, (const unsigned char *) "🧡", (const unsigned char *) "heart");
  emoji_add_translation(&emoji, (const unsigned char *) "💙", (const unsigned char *) "&heart");

  unsigned char *translation = (unsigned char *) emoji_translate_file_alloc(&emoji, "tests/txt/simple.txt");
  REQUIRE(translation != NULL);

  INFO("translation := " << translation);
  REQUIRE(strcmp((char *) translation, "I heart&heart Illinois!") == 0);
  free(translation);

  emoji_destroy(&emoji);
}

TEST_CASE("translate - translation with text in between emojis", "[weight=3][part=2]") {
  emoji_t emoji;
  emoji_init(&emoji);

  emoji_add_translation(&emoji, (const unsigned char *)"😊", (const unsigned char *)":happy:");


  unsigned char *translation = (unsigned char *) emoji_translate_file_alloc(&emoji, "tests/txt/text-inbetween.txt");
  REQUIRE(translation != NULL);

  INFO("translation := " << translation);
  REQUIRE(strcmp((char *) translation, ":happy: CS340 :happy:") == 0);
  free(translation);

  emoji_destroy(&emoji);
}

TEST_CASE("translate - translation with only one emoji, result is a shorter string", "[weight=3][part=2]") {
  emoji_t emoji;
  emoji_init(&emoji);

  emoji_add_translation(&emoji, (const unsigned char *)"💙", (const unsigned char *)"<3");


  unsigned char *translation = (unsigned char *) emoji_translate_file_alloc(&emoji, "tests/txt/oneemoji.txt");
  REQUIRE(translation != NULL);

  INFO("translation := " << translation);
  REQUIRE(strcmp((char *) translation, "<3") == 0);
  free(translation);

  emoji_destroy(&emoji);
}

TEST_CASE("translate - translation with one emoji, result is a longer string", "[weight=3][part=2]") {
  emoji_t emoji;
  emoji_init(&emoji);

  emoji_add_translation(&emoji, (const unsigned char *)"💙", (const unsigned char *)"Taylor Swift");


  unsigned char *translation = (unsigned char *) emoji_translate_file_alloc(&emoji, "tests/txt/oneemoji.txt");
  REQUIRE(translation != NULL);

  INFO("translation := " << translation);
  REQUIRE(strcmp((char *) translation, "Taylor Swift") == 0);
  free(translation);

  emoji_destroy(&emoji);
}

TEST_CASE("translate - translation to an empty string", "[weight=3][part=2]") {
  emoji_t emoji;
  emoji_init(&emoji);

  emoji_add_translation(&emoji, (const unsigned char *)"😊", (const unsigned char *)"");

  unsigned char *translation = (unsigned char *) emoji_translate_file_alloc(&emoji, "tests/txt/text-inbetween.txt");
  REQUIRE(translation != NULL);

  INFO("translation := " << translation);
  REQUIRE(strcmp((char *) translation, " CS340 ") == 0);
  free(translation);

  emoji_destroy(&emoji);
}

TEST_CASE("translate - translation with tiebreaks in translation", "[weight=3][part=2]") {
  emoji_t emoji;
  emoji_init(&emoji);

  emoji_add_translation(&emoji, (const unsigned char *) "🧡", (const unsigned char *)"heart");
  emoji_add_translation(&emoji, (const unsigned char *)"💙", (const unsigned char *)"&heart");
  emoji_add_translation(&emoji, (const unsigned char *)"🧡💙", (const unsigned char *)":illini heart:");

  unsigned char *translation = (unsigned char *) emoji_translate_file_alloc(&emoji, "tests/txt/simple.txt");
  REQUIRE(translation != NULL);

  INFO("translation := " << translation);
  REQUIRE(strcmp((char *) translation, "I :illini heart: Illinois!") == 0);
  free(translation);

  emoji_destroy(&emoji);
}


TEST_CASE("translate - translation with tiebreaks (reverse order)", "[weight=3][part=2]") {
  emoji_t emoji;
  emoji_init(&emoji);

  emoji_add_translation(&emoji, (const unsigned char *)"🧡💙", (const unsigned char *)":illini heart:");
  emoji_add_translation(&emoji, (const unsigned char *)"🧡", (const unsigned char *)"heart");
  emoji_add_translation(&emoji, (const unsigned char *)"💙", (const unsigned char *)"&heart");
  
  unsigned char *translation = (unsigned char *) emoji_translate_file_alloc(&emoji, "tests/txt/simple.txt");
  REQUIRE(translation != NULL);

  INFO("translation := " << translation);
  REQUIRE(strcmp((char *) translation, "I :illini heart: Illinois!") == 0);
  free(translation);

  emoji_destroy(&emoji);
}

TEST_CASE("translate - translation with nested tiebreaks", "[weight=3][part=2]") {
  emoji_t emoji;
  emoji_init(&emoji);

  emoji_add_translation(&emoji, (const unsigned char *)"🧡💙🧡", (const unsigned char *)":illini double-orange heart:");
  emoji_add_translation(&emoji, (const unsigned char *)"💙🧡", (const unsigned char *)":illini heart:");
  emoji_add_translation(&emoji, (const unsigned char *)"🧡", (const unsigned char *)"heart");
  emoji_add_translation(&emoji, (const unsigned char *)"💙", (const unsigned char *)"&heart");

  unsigned char *translation = (unsigned char *) emoji_translate_file_alloc(&emoji, "tests/txt/nested-simple.txt");
  REQUIRE(translation != NULL);

  INFO("translation := " << translation);
  REQUIRE(strcmp((char *) translation, "I :illini double-orange heart:&heart Illinois!") == 0);
  free(translation);

  emoji_destroy(&emoji);
}

TEST_CASE("translate - translation with nested tiebreaks (more tests)", "[weight=3][part=2]") {
  emoji_t emoji;
  emoji_init(&emoji);

  emoji_add_translation(&emoji, (const unsigned char *)"💙🧡💙", (const unsigned char *)":illini double-blue heart:");
  emoji_add_translation(&emoji, (const unsigned char *)"🧡💙💙", (const unsigned char *)":illini heart:");
  emoji_add_translation(&emoji, (const unsigned char *)"🧡", (const unsigned char *)"heart");
  emoji_add_translation(&emoji, (const unsigned char *)"💙", (const unsigned char *)"&heart");

  unsigned char *translation = (unsigned char *) emoji_translate_file_alloc(&emoji, "tests/txt/nested-simple.txt");
  REQUIRE(translation != NULL);

  INFO("translation := " << translation);
  REQUIRE(strcmp((char *) translation, "I heart:illini double-blue heart: Illinois!") == 0);
  free(translation);

  emoji_destroy(&emoji);
}

TEST_CASE("translate - translation works with empty emoji_t and long text", "[weight=3][part=2]") {
  emoji_t emoji;
  emoji_init(&emoji);

  unsigned char *translation = (unsigned char *) emoji_translate_file_alloc(&emoji, "tests/txt/noemoji.txt");

  INFO("translation := " << translation);
  REQUIRE(strcmp((char *) translation, "CS340 has the best students ever!") == 0);
  free(translation);

  emoji_destroy(&emoji);
}

TEST_CASE("translate - translation works with empty emoji_t and short text", "[weight=3][part=2]") {
  emoji_t emoji;
  emoji_init(&emoji);

  unsigned char *translation = (unsigned char *) emoji_translate_file_alloc(&emoji, "tests/txt/noemoji-short.txt");

  INFO("translation := " << translation);
  REQUIRE(strcmp((char *) translation, "CS340") == 0);
  free(translation);

  emoji_destroy(&emoji);
}

TEST_CASE("translate long text - translation with many translations", "[weight=4][part=2]") {
  emoji_t emoji;
  emoji_init(&emoji);

  emoji_add_translation(&emoji, (const unsigned char *)"🙅", (const unsigned char *)"no, ");
  emoji_add_translation(&emoji, (const unsigned char *)"🖐", (const unsigned char *)"but ");
  emoji_add_translation(&emoji, (const unsigned char *) "🙅👄💬🧑🔮", (const unsigned char *)"We don't talk about Bruno, ");
  emoji_add_translation(&emoji, (const unsigned char *)"💒👰💍🤵", (const unsigned char *)"It was my wedding day ");
  emoji_add_translation(&emoji, (const unsigned char *)"💄💅💇🙅🌤🌄", (const unsigned char *)"We were getting ready, and there wasn't a cloud in the sky ");
  emoji_add_translation(&emoji, (const unsigned char *)"🧑🔮🚶😈", (const unsigned char *)"Bruno walks in with a mischievous grin ");
  emoji_add_translation(&emoji, (const unsigned char *)"😠🗯🗨💢🤫", (const unsigned char *)"You telling this story or am I?");

  const char* expected = "We don't talk about Bruno, no, no, no, \n"
  "We don't talk about Bruno, but \n"
  "It was my wedding day \n"
  "We were getting ready, and there wasn't a cloud in the sky \n"
  "Bruno walks in with a mischievous grin \n"
  "You telling this story or am I?";

  unsigned char *translation = (unsigned char *) emoji_translate_file_alloc(&emoji, "tests/txt/long.txt");
  REQUIRE(translation != NULL);
  
  INFO("translation := " << translation);
  REQUIRE(strcmp((char *) translation, expected) == 0);
  free(translation);

  emoji_destroy(&emoji);
}
