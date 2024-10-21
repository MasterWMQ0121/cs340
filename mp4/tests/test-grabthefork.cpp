#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/times.h>
#include <sys/time.h>

#include "../lib/wallet.h"
#include "lib/catch.hpp"

static bool pingpongtimeup = false;

// elves enjoy eating with one fork and one knife
void *hungry_elf(void *args) {
  wallet_t *wallet = (wallet_t *) args;
  for (int i = 0; i < 100; i++) {
    wallet_change_resource(wallet, "fork", -1);
    wallet_change_resource(wallet, "knife", -1);
    wallet_change_resource(wallet, "food", -1);
    wallet_change_resource(wallet, "fork", 1);
    wallet_change_resource(wallet, "knife", 1);
    fprintf(stderr, "E");
  }
  return NULL;
}

// human enjoy eating with two forks
void *hungry_human(void *args) {
  wallet_t *wallet = (wallet_t *) args;
  for (int i = 0; i < 100; i++) {
    wallet_change_resource(wallet, "fork", -2);
    wallet_change_resource(wallet, "food", -1);
    wallet_change_resource(wallet, "fork", 2);
    fprintf(stderr, "H");
  }
  return NULL;
}

// orcs enjoy eating with two forks
void *hungry_orc(void *args) {
  wallet_t *wallet = (wallet_t *) args;
  for (int i = 0; i < 100; i++) {
    wallet_change_resource(wallet, "knife", -2);
    wallet_change_resource(wallet, "food", -1);
    wallet_change_resource(wallet, "knife", 2);
    fprintf(stderr, "O");
  }
  return NULL;
}

TEST_CASE("test-grabthefork", "[weight=5][part=1]") {
  // Create and initialize wallets
  wallet_t wallet;
  wallet_init(&wallet);
  wallet_change_resource(&wallet, "fork", 2);
  wallet_change_resource(&wallet, "knife", 2);
  wallet_change_resource(&wallet, "food", 300);
  pthread_t tids[3];
  pthread_create(&tids[0], NULL, hungry_human,    &wallet);
  pthread_create(&tids[1], NULL, hungry_elf,    &wallet);
  pthread_create(&tids[2], NULL, hungry_orc, &wallet);

  for (int i = 0; i < 3; i++) {
    pthread_join(tids[i], NULL);
  }

  CHECK( wallet_get(&wallet, "fork") == 2 );
  CHECK( wallet_get(&wallet, "knife") == 2 );
  CHECK( wallet_get(&wallet, "food") == 0 );

  // Destroy the wallets
  wallet_destroy(&wallet);
}
