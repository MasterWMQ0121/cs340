#include "lib/catch.hpp"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>

pthread_t tid;
char *cmd;

void *launch_server(void *vptr_cmd) {
  system(cmd);
  free(cmd);
}

void kill_server() {
  system("pkill -9 -f ./server");
  pthread_join(tid, NULL);
}

int launch_server() {
  // Ensure the server is up to date:
  REQUIRE( system("make -s") == 0 );

  // Find a port:
  time_t t;
  srand((unsigned) time(&t));
  int port = 10000 + (rand() % 40000);
  asprintf(&cmd, "./server %d &", port);

  pthread_create(&tid, NULL, launch_server, (void *)cmd);
  sleep(1);

  return port;
}


TEST_CASE("server - loads `/`", "[weight=5][part=3]") {
  int port = launch_server();

  char curl[1000];
  sprintf(curl, "curl --max-time 15 -f http://localhost:%d/ >TEST_out.txt", port);

  CAPTURE( curl );
  REQUIRE( system(curl) == 0 );
  REQUIRE( system("diff TEST_out.txt static/index.html") == 0 );
  system("rm TEST_out.txt");

  kill_server();
}

TEST_CASE("server - loads `/getaway.html`", "[weight=10][part=3]") {
  int port = launch_server();

  char curl[1000];
  sprintf(curl, "curl --max-time 15 -f http://localhost:%d/getaway.html >TEST_out.txt", port);

  CAPTURE(curl);
  REQUIRE( system(curl) == 0 );
  REQUIRE( system("diff TEST_out.txt static/getaway.html") == 0 );
  system("rm TEST_out.txt");
  
  kill_server();
}

TEST_CASE("server - fails `/not-found.html`", "[weight=10][part=3]") {
  int port = launch_server();

  char curl[1000];
  sprintf(curl, "curl --max-time 15 -f http://localhost:%d/not-found.html", port);

  CAPTURE(curl);
  REQUIRE( system(curl) != 0 );

  kill_server();
}

TEST_CASE("server - loads `/340.png`", "[weight=15][part=3]") {
  int port = launch_server();

  char curl[1000];
  sprintf(curl, "curl --max-time 15 -f http://localhost:%d/340.png >TEST_out.png", port);

  CAPTURE(curl);
  REQUIRE( system(curl) == 0 );
  REQUIRE( system("diff TEST_out.png static/340.png") == 0 );
  system("rm TEST_out.png");

  kill_server();
}