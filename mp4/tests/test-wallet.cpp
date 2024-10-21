#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/times.h>

#include "../lib/wallet.h"
#include "lib/catch.hpp"

// Test 1
TEST_CASE("wallet_get - non-existant resource must return 0", "[weight=1][part=1]") {
  // Create and initialize the wallet
  wallet_t wallet;
  wallet_init(&wallet);

  // Get the amount of a resource
  int num_resources = wallet_get(&wallet, "resources");
  REQUIRE(num_resources == 0);

  // Destroy the wallet
  wallet_destroy(&wallet);
}


TEST_CASE("wallet_change_resource - returns value returns resource count", "[weight=2][part=1]") {
  // Create and initialize the wallet
  wallet_t wallet;
  wallet_init(&wallet);

  // Get the amount of a resource
  REQUIRE(wallet_change_resource(&wallet, "resources", 123) == 123);

  // Destroy the wallet
  wallet_destroy(&wallet);
}


TEST_CASE("wallet_change_resource - resource count changes correctly", "[weight=2][part=1]") {
  // Create and initialize the wallet
  wallet_t wallet;
  wallet_init(&wallet);

  // Get the amount of a resource
  REQUIRE(wallet_change_resource(&wallet, "resources", 123) == 123);
  REQUIRE(wallet_change_resource(&wallet, "resources", 321) == 444);

  // Destroy the wallet
  wallet_destroy(&wallet);
}


// Test 2
void * test_add_negative_delta(void * args) {
  wallet_t *wallet = (wallet_t *) args;
  wallet_change_resource(wallet, "cookies", -10);
  return NULL;
}

void * test_add_positive_delta(void * args) {
  wallet_t *wallet = (wallet_t *) args;
  wallet_change_resource(wallet, "cookies", 100);
  return NULL;
}

TEST_CASE("wallet_change_resource - initial negative delta", "[weight=3][part=1]") {
    // Create and initialize the wallet
    wallet_t wallet;
    wallet_init(&wallet);

    // Test what happens if the first thing your wallet sees is a negative delta
    pthread_t tids[2];
    pthread_create(&tids[0], NULL, test_add_negative_delta, &wallet);
    usleep(500);
    pthread_create(&tids[1], NULL, test_add_positive_delta, &wallet);
    pthread_join(tids[0], NULL);
    pthread_join(tids[1], NULL);

    // Get the final number of the resource
    int num_cookies = wallet_get(&wallet, "cookies");
    REQUIRE(num_cookies == 90);

    // Destroy the wallet
    wallet_destroy(&wallet);
}


// Test 3
TEST_CASE("wallet_t structures are independent (tests multiple concurrent wallets)","[weight=5][part=1]") {
  wallet_t wallet_1;
  wallet_t wallet_2;

  wallet_init(&wallet_1);
  wallet_init(&wallet_2);

  REQUIRE(wallet_get(&wallet_1, "cookies") == 0);
  pthread_t tids[4];
  pthread_create(&tids[0], NULL, test_add_positive_delta, &wallet_1);
  pthread_create(&tids[1], NULL, test_add_positive_delta, &wallet_2);
  pthread_create(&tids[2], NULL, test_add_positive_delta, &wallet_1);
  pthread_create(&tids[3], NULL, test_add_positive_delta, &wallet_1);

  for(int i=0;i<4;i++)
    pthread_join(tids[i],NULL);

  REQUIRE(wallet_get(&wallet_1, "cookies") == 300);
  REQUIRE(wallet_get(&wallet_2, "cookies") == 100);

  wallet_destroy(&wallet_1);
  wallet_destroy(&wallet_2);
}


// Test 4
void * test_add_large(void * args) {
  wallet_t *wallet = (wallet_t *) args;
  for (int i = 0; i < 100000; i++) {
    wallet_change_resource(wallet, "potatoes", 2);
  }
}

void * test_subtract_large(void * args) {
  wallet_t *wallet = (wallet_t *) args;
  for (int i = 0; i < 100000; i++) {
    wallet_change_resource(wallet, "potatoes", -1);
  }
}

TEST_CASE("wallet_change_resource - handles many concurrent requests", "[weight=5][part=2]") {
  // Create and initialize the wallet
  wallet_t wallet;
  wallet_init(&wallet);

  int num_threads = 100;
  pthread_t tids[num_threads];

  for (int i = 0; i < (num_threads/2); i++) {
    pthread_create(&tids[i], NULL, test_subtract_large, (void *) &wallet);
  }
  for (int i = (num_threads/2); i < num_threads; i++) {
    pthread_create(&tids[i], NULL, test_add_large, (void *) &wallet);
  }
  for (int i = 0; i < num_threads; i++) {
    pthread_join(tids[i], NULL);
  }

  int num_potatoes = wallet_get(&wallet, "potatoes");
  REQUIRE(num_potatoes == (100000 * (num_threads/2)));

  // Destroy the wallet
  wallet_destroy(&wallet);
}
