#include <stdlib.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#include "lib/catch.hpp"
#include "../lib/wallet.h"

#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif

size_t PORT = 12000;

int read_response(int fd) {
  // Read response:
  char buffer[4096];
  int bytes_read = recv(fd, buffer, 4096, 0);
  if(bytes_read == -1){
    Catch::throw_runtime_error("Read failed");
    return -1;
  }

  // Format as string:
  buffer[bytes_read - 1] = '\0';

  // Convert to a number:
  int value = atoi(buffer);

  // Check for an atoi error and return a non-zero error value:
  if (value == 0 && buffer[0] != '0' && buffer[1] != '\n') {
    return -99999;
  }

  return value;
}

int wallet_GET(int fd, const char* resource_name) {
  // Send GET:
  char* message = NULL;
  asprintf(&message, "GET %s\n", resource_name);
  int bytes_sent = send(fd, message, strlen(message), MSG_NOSIGNAL);
  if(bytes_sent == -1){
    Catch::throw_runtime_error("Send failed, socket/server is down.");
    return -1;
  }
  free(message);

  return read_response(fd);
}

void wallet_MOD_noread(int fd, const char* resource_name, int delta) {
  char* message = NULL;
  asprintf(&message, "MOD %s %d\n", resource_name, delta);
  send(fd, message, strlen(message), 0);
  free(message);
}

int wallet_MOD(int fd, const char* resource_name, int delta) {
  wallet_MOD_noread(fd, resource_name, delta);
  return read_response(fd);
}

void wallet_EXIT(int fd) {
  send(fd, "EXIT\n", 5, MSG_NOSIGNAL);
  shutdown(fd, SHUT_RDWR);
  close(fd);
}

int generate_client_socket(const size_t port) {
  // socket:
  int fd = socket(PF_INET, SOCK_STREAM, 0);
  if (fd == -1) {
    perror("socket");
    FAIL("Failed to create a socket.");
    return -1;
  }

  struct sockaddr_in my_addr;
  my_addr.sin_family = AF_INET;
  my_addr.sin_port = htons(port);
  my_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
  memset(my_addr.sin_zero, 0x00, sizeof my_addr.sin_zero);

  // connect:
  if (connect(fd, (sockaddr *)&my_addr, sizeof(sockaddr_in)) == -1) {
    perror("connect");
    FAIL("Failed to connect to :" << port);
    return -1;
  }

  return fd;
}

void * launch_server_thread(void *vptr_port) {
  int port = *((int *)vptr_port);

  system("make wallet-server");

  char cmd[100];
  sprintf(cmd, "./wallet-server -p%d", port);
  printf("Running: %s\n", cmd);
  system(cmd);
  return NULL;
}

pthread_t server_tid;
int launch_server(int port) {
  // Base case, retries keep failing and port number has grown.
  if (port > 50000) {
    FAIL("Failed to find a port.");
    return -1;
  }

  // Check if a process is using port:
  char cmd[100];
  sprintf(cmd, "lsof -i:%d -t", port);
  int returnValue = system(cmd);

  // port is in use, find another...
  if (returnValue == 0 /* at least one process is using the port */) {
    printf("Port :%d is in use. ", port);
    port += (rand() % 10000);
    printf("Trying :%d.\n", port);
    return port = launch_server(port);
  }

  pthread_create(&server_tid, NULL, launch_server_thread, &port);

  int attempts = 0;
  returnValue = 1;
  sprintf(cmd, "lsof -i:%d -t", port);
  while (returnValue != 0) {
    usleep(100);
    returnValue = system(cmd);
    attempts++;

    if (attempts > 50) {
      FAIL("wallet-server failed to be available on :" << port);
      return -1;
    }
  }

  return port;
}

void kill_server() {
  system("pkill -9 -f ./wallet-server");
  pthread_join(server_tid, NULL);
}


TEST_CASE("wallet-server - `GET nonexistent` returns zero", "[weight=5][part=3]"){
  size_t port = PORT;
  port = launch_server(port);

  int client_fd = generate_client_socket(port);

  // GET nonexistent
  CHECK( wallet_GET(client_fd, "nonexistent") == 0 );

  wallet_EXIT(client_fd);
  kill_server();
}

TEST_CASE("wallet-server - `MOD` returns the correct value", "[weight=5][part=3]"){
  size_t port = PORT + 1;
  port = launch_server(port);

  int client_fd = generate_client_socket(port);

  // MOD + GET
  CHECK( wallet_MOD(client_fd, "Money", 5) == 5 );

  wallet_EXIT(client_fd);
  kill_server();
}

TEST_CASE("wallet-server - `MOD` and `GET` work together with one resource", "[weight=5][part=3]"){
  size_t port = PORT + 2;
  port = launch_server(port);

  int client_fd = generate_client_socket(port);

  // MOD + GET
  CHECK( wallet_GET(client_fd, "Money") == 0 );
  CHECK( wallet_MOD(client_fd, "Money", 5) == 5);
  CHECK( wallet_GET(client_fd, "Money") == 5 );

  wallet_EXIT(client_fd);
  kill_server();
}


TEST_CASE("wallet-server - `MOD` blocks request when resources are unavailable", "[weight=5][part=3]"){
  size_t port = PORT + 3;
  port = launch_server(port);

  int client1_fd = generate_client_socket(port);
  int client2_fd = generate_client_socket(port);

  wallet_MOD_noread(client1_fd, "Money", -10);
  usleep(100);
  
  // `client1_fd` must be block since Money is not available:
  char c;
  int bytes_received = recv(client1_fd, &c, 1, MSG_DONTWAIT);
  CHECK(bytes_received == -1); // No data should be available.

  // `client2_fd` adds the money:
  wallet_MOD(client2_fd, "Money", 20);

  // `client1_fd` can unblock since -10 + 20 = 10:
  CHECK(wallet_GET(client1_fd, "Money") == 10);

  wallet_EXIT(client1_fd);
  wallet_EXIT(client2_fd);
  kill_server();
}


typedef struct _connection_data_t {
  int port;
  int delta;
  const char* resource_name;
} connection_data_t;

void* connection_thread_test(void* conn_data_vptr) {
  connection_data_t* con_ptr = (connection_data_t*) conn_data_vptr;

  int client_fd = generate_client_socket(con_ptr->port);
  wallet_MOD(client_fd, con_ptr->resource_name, con_ptr->delta);
  wallet_EXIT(client_fd);

  free(conn_data_vptr);
  return NULL;
}

TEST_CASE("wallet-server - handles many concurrent connections", "[weight=5][part=3]"){
  size_t port = PORT + 4;
  port = launch_server(port);

  size_t num_conns = 60;
  int scaling_factor = 100;

  pthread_t threads[num_conns];
  int cur_money = 0;

  for(size_t i = 0; i < num_conns; i++){
    connection_data_t* con_data = (connection_data_t*) malloc(sizeof(connection_data_t));

    int delta = scaling_factor * i;
    if (i % 3 == 0) { delta *= -1; }

    con_data->port = port;    
    con_data->delta = delta;
    con_data->resource_name = "Money";

    pthread_t pthread;
    pthread_create(&pthread, NULL, connection_thread_test, con_data);
    threads[i] = pthread;
    cur_money += delta;
  }

  for(size_t i = 0; i < num_conns; i++){
    pthread_join(threads[i], NULL);
  }

  int client_fd = generate_client_socket(port);
  CHECK( wallet_GET(client_fd, "Money") == cur_money );
  wallet_EXIT(client_fd);

  kill_server();
}


TEST_CASE("wallet-server - `MOD` blocks request when resources become unavailable", "[weight=5][part=3]"){
  size_t port = PORT + 5;
  port = launch_server(port);

  int client_fd = generate_client_socket(port);
  int count = 10;

  wallet_MOD(client_fd, "Notes", count);
  CHECK( wallet_GET(client_fd, "Notes") == count );

  for(int i = 1; i < 10; ++i) {
    wallet_MOD(client_fd, "Notes", -1 * i);
   
    count -= i;
    if(count > 0) {
      CHECK( wallet_GET(client_fd, "Notes") == count);
    } else {
      sleep(1);

      char c;
      int bytes_received = recv(client_fd, &c, 1, MSG_DONTWAIT);
      CHECK(bytes_received == -1); // No data should be available.
      break;
    }
  }

  wallet_EXIT(client_fd);
  kill_server();
}


void recur(int depth, int client_fd) {
  if(depth == 0) {
    wallet_MOD(client_fd, "Mustard", 1);
    return;
  }
  wallet_MOD(client_fd, "Mustard", -1);
  recur(depth -  1, client_fd);
  recur(depth -  1, client_fd);
}

TEST_CASE("wallet-server - single connection with many requests", "[weight=5][part=3]"){
  size_t port = PORT + 6;
  port = launch_server(port);
  
  int client_fd = generate_client_socket(port);

  wallet_MOD(client_fd, "Mustard", 10);
  recur(10, client_fd);
  CHECK( wallet_GET(client_fd, "Mustard") == 11 );

  wallet_EXIT(client_fd);
  kill_server();
}



int clients[10];

void recur_worker_thread(int fd, int id, int depth) {
   if(depth == 0){
    wallet_MOD(fd, "Coffee", 1);
    return;
   }
   if(fd % 2 == 0) {
    wallet_MOD(fd, "Coffee", 1);
   } else {
    wallet_MOD(fd, "Coffee", -1);
   }
   recur_worker_thread(fd, id, depth - 1);
}

void *worker_thread(void *fd_) {
  int fd = *((int *)fd_);
  int id = 0;
  for (int i = 0; i < 10; i++) {
    if (clients[i] == fd) {
      id = i;
      break;
    }
  }
  recur_worker_thread(fd, id, 10);
  return NULL;
}

TEST_CASE("wallet-server - concurrent connections with many requests", "[weight=5][part=3]"){
  size_t port = PORT + 7;
  port = launch_server(port);
   
  pthread_t pthreads[10];
  for(int i = 0; i < 10; i++){
    clients[i] = generate_client_socket(port);
    pthread_create(&pthreads[i], NULL, worker_thread, &clients[i]);
  }

  for(int i = 0; i < 10; ++i) {
    pthread_join(pthreads[i], NULL);
  }
  
  CHECK(wallet_GET(clients[1], "Coffee") == 10);

  for(int i = 0; i < 10; ++i) {
    wallet_EXIT(clients[i]);
  }

  kill_server();
}
