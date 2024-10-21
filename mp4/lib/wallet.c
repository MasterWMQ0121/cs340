#include <stdio.h>
#include <stdlib.h>
#include <string.h>  
#include "wallet.h"

/**
 * Initializes an empty wallet.
 */
void wallet_init(wallet_t *wallet) {
  // Implement `wallet_init`
  wallet->resources = NULL;
  pthread_mutex_init(&wallet->lock, NULL);
  pthread_cond_init(&wallet->cond, NULL);
}

/**
 * Returns the amount of a given `resource` in the given `wallet`.
 */
int wallet_get(wallet_t *wallet, const char *resource) {
  // Implement `wallet_get`
  pthread_mutex_lock(&wallet->lock);
  wallet_resources_t *current = wallet->resources;
  while (current != NULL) {
    if (strcmp(current->name, resource) == 0) {
      int amount = current->amount;
      pthread_mutex_unlock(&wallet->lock);
      return amount;
    }
    current = current->next;
  }
  pthread_mutex_unlock(&wallet->lock);
  return 0;
}

/**
 * Modifies the amount of a given `resource` in a given `wallet by `delta`.
 * - If `delta` is negative, this function MUST NOT RETURN until the resource can be satisfied.
 *   (Ths function MUST BLOCK until the wallet has enough resources to satisfy the request.)
 * - Returns the amount of resources in the wallet AFTER the change has been applied.
 */
int wallet_change_resource(wallet_t *wallet, const char *resource, const int delta) {
  // Implement `wallet_change_resource`
  pthread_mutex_lock(&wallet->lock);
  wallet_resources_t *current = wallet->resources;
  while (current != NULL && strcmp(current->name, resource) != 0) {
    current = current->next;
  }
  if (current == NULL) {
    current = (wallet_resources_t *)calloc(sizeof(wallet_resources_t), 1);
    current->name = strdup(resource);
    current->amount = 0;
    current->next = wallet->resources;
    wallet->resources = current;
  }

  while (current->amount + delta < 0) {
    pthread_cond_wait(&wallet->cond, &wallet->lock);
  }

  current->amount += delta;

  int amount = current->amount;
  
  pthread_cond_broadcast(&wallet->cond);
  pthread_mutex_unlock(&wallet->lock);
  return amount;
}

/**
 * Destroys a wallet, freeing all associated memory.
 */
void wallet_destroy(wallet_t *wallet) {
  // Implement `wallet_destroy`
  wallet_resources_t *temp;
  while (wallet->resources) {
    temp = wallet->resources;
    wallet->resources = wallet->resources->next;
    free(temp->name);
    free(temp);
  }
  pthread_mutex_destroy(&wallet->lock);
  pthread_cond_destroy(&wallet->cond);
}