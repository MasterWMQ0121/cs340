#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>

#include "http.h"


/**
 * httprequest_parse_headers
 * 
 * Populate a `req` with the contents of `buffer`, returning the number of bytes used from `buf`.
 */
ssize_t httprequest_parse_headers(HTTPRequest *req, char *buffer, ssize_t buffer_len) {
  req->headers = NULL;
  req->payload = NULL;

  char *val;
  char *tmp;
  char *each_line = strtok_r(buffer, "\r\n", &val);

  char *line = strtok_r(each_line, " ", &tmp);
  req->action = strdup(line);

  line = strtok_r(NULL, " ", &tmp);
  req->path = strdup(line);

  line = strtok_r(NULL, "\r\n", &tmp);
  req->version = strdup(line);

  HTTP *prev = NULL;
  char *valtmp;
  
  while (*(val + 1) != '\r' || *(val + 2) != '\n') {
    char *lines = strtok_r(NULL, "\r\n", &val);
    HTTP *wmq = calloc(1, sizeof(HTTP));
    wmq->key = strdup(strtok_r(lines, ":", &valtmp));
    wmq->value = strdup(valtmp + 1);
    if (prev == NULL) {
      req->headers = wmq;
    } else {
      prev->next = wmq;
    }
    prev = wmq;
  }

  const char *content_length_str = httprequest_get_header(req, "Content-Length");
  if (content_length_str != NULL && atoi(content_length_str) > 0) {
    int len = atoi(content_length_str);
    req->payload = calloc(len, sizeof(char));
    if (req->payload != NULL) {
      memcpy((void*)req->payload, buffer + buffer_len - len, len);
    }
  }
  return -1;
}


/**
 * httprequest_read
 * 
 * Populate a `req` from the socket `sockfd`, returning the number of bytes read to populate `req`.
 */
ssize_t httprequest_read(HTTPRequest *req, int sockfd) {
  // char *buffer = calloc(6000000, sizeof(char));
  // ssize_t bytes_read = read(sockfd, buffer, 6000000);
  // httprequest_parse_headers(req, buffer, bytes_read);
  // free(buffer);
  // return bytes_read;
  // char *buffer = calloc(6000000, sizeof(char));
  // ssize_t bytes_received = 0;
  // while (bytes_received < 6000000) {
  //   ssize_t len = read(sockfd, buffer + bytes_received, 6000000 - bytes_received);
  //   if (len <= 0) {
  //     break;
  //   }
  //   bytes_received += len;
  // }
  // httprequest_parse_headers(req, buffer, bytes_received);
  // free(buffer);
  // return bytes_received;
  // char *buffer = calloc(6000000, sizeof(char));
  // ssize_t total_bytes_read = 0;
  // ssize_t bytes_read;
  // while ((bytes_read = read(sockfd, buffer + total_bytes_read, 65536)) > 0) {
  //   total_bytes_read += bytes_read;
  // }
  // httprequest_parse_headers(req, buffer, total_bytes_read);
  // free(buffer);
  // return total_bytes_read;
  char *buffer = calloc(6000000, sizeof(char));
  ssize_t total_bytes_read = 0;
  ssize_t bytes_read;
  while ((bytes_read = read(sockfd, buffer + total_bytes_read, 65536)) > 0) {
    total_bytes_read += bytes_read;
    if (total_bytes_read >= 4 && strcmp(buffer + total_bytes_read - 4, "\r\n\r\n") == 0) {
      break;
    }
  }
  httprequest_parse_headers(req, buffer, total_bytes_read);
  free(buffer);
  return total_bytes_read;
}


/**
 * httprequest_get_action
 * 
 * Returns the HTTP action verb for a given `req`.
 */
const char *httprequest_get_action(HTTPRequest *req) {
  return req->action;
}


/**
 * httprequest_get_header
 * 
 * Returns the value of the HTTP header `key` for a given `req`.
 */
const char *httprequest_get_header(HTTPRequest *req, const char *key) {
  HTTP *header = req->headers;
  while (header != NULL) {
    if (strcmp(header->key, key) == 0) {
      return header->value;
    }
    header = header->next;
  }
  return NULL;
}


/**
 * httprequest_get_path
 * 
 * Returns the requested path for a given `req`.
 */
const char *httprequest_get_path(HTTPRequest *req) {
  return req->path;
}


/**
 * httprequest_destroy
 * 
 * Destroys a `req`, freeing all associated memory.
 */
void httprequest_destroy(HTTPRequest *req) {
  free(req->action);
  free(req->path);
  free(req->version);
  HTTP *header = req->headers;
  while (header != NULL) {
    HTTP *next = header->next;
    free(header->key);
    free(header->value);
    free(header);
    header = next;
  }
  if (req->payload != NULL) {
    free(req->payload);
  }
}