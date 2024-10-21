#include "http.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <netinet/in.h>
#include <pthread.h>

void *client_thread(void *vptr) {
  int fd = *((int *)vptr);
  HTTPRequest *req = calloc(sizeof(HTTPRequest), sizeof(char));
  char *res = (char *)calloc(6000000, sizeof(char));
  // int len = read(fd, res, sizeof(res));
  // httprequest_parse_headers(req, res, len);
  httprequest_read(req, fd);
  
  char *path = calloc(7 + strlen(req->path), 1);
  strcpy(path, "static");
  if (strcmp(req->path, "/") == 0) {
    strcat(path, "/index.html");
  } else {
    strcat(path, req->path);
  }

  FILE *file = fopen(path, "r");
  if (!file) {
    res = strdup("HTTP/1.1 404 File Not Found\r\n\r\n");
    write(fd, res, strlen(res));
    close(fd);
    httprequest_destroy(req);
    free(req);
    free(path);
    free(res);
    return NULL;
  }
  
  strcat(res, "HTTP/1.1 200 OK\r\n");
  fseek(file, 0, SEEK_END);
  int line = ftell(file);
  fseek(file, 0, SEEK_SET);
  char *payload = calloc(line + 1, 1);
  fread(payload, 1, line, file);
  fclose(file);

  char *content_length = calloc(20, 1);
  if (strstr(path, ".html")) {
    strcat(res, "Content-Type: text/html\r\n");
  } else if (strstr(path, ".png")) {
    strcat(res, "Content-Type: image/png\r\n");
  }
  sprintf(content_length, "Content-Length: %d\r\n", line);
  strcat(res, "\r\n");

  int wmq = strlen(res);
  memcpy(res + wmq, payload, line);
  write(fd, res, wmq + line);
  close(fd);

  httprequest_destroy(req);
  free(req);
  free(path);
  free(res);
  free(payload);
  return NULL;
}

int main(int argc, char *argv[]) {
  if (argc != 2) {
    printf("Usage: %s <port>\n", argv[0]);
    return 1;
  }
  int port = atoi(argv[1]);
  printf("Binding to port %d. Visit http://localhost:%d/ to interact with your server!\n", port, port);

  // socket:
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);

  // bind:
  struct sockaddr_in server_addr, client_address;
  memset(&server_addr, 0x00, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(port);  
  bind(sockfd, (const struct sockaddr *)&server_addr, sizeof(server_addr));

  // listen:
  listen(sockfd, 10);

  // accept:
  socklen_t client_addr_len;
  while (1) {
    int *fd = malloc(sizeof(int));
    client_addr_len = sizeof(struct sockaddr_in);
    *fd = accept(sockfd, (struct sockaddr *)&client_address, &client_addr_len);
    printf("Client connected (fd=%d)\n", *fd);

    pthread_t tid;
    pthread_create(&tid, NULL, client_thread, fd);
    pthread_detach(tid);
  }

  return 0;
}