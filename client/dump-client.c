#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include <limits.h>
#include <libgen.h>

#define SERVER_PORT 8080

#define BOOLEAN 1
#define INTEGER 2
#define STRING 3

typedef struct sockaddr_in SA_IN;
typedef struct sockaddr SA;

int client_socket;

int connect_to_server(int);
void check(int , char* );
void read_from_server(void*, int, int);


int main(int argc, char const *argv[]) {
  bool keep_reading = false;
  char message[256];

  client_socket = connect_to_server(SERVER_PORT);

  do {
    read_from_server(&keep_reading, sizeof(keep_reading), BOOLEAN);

    if (!keep_reading) break;

    read_from_server(message, sizeof(message), STRING);
    printf("%s\n", message);

  } while (keep_reading);
  

  close(client_socket);
  return 0;
}

/* Basic connection */
int connect_to_server(int port) {
  int client_socket;
  SA_IN server_address;

  check((client_socket = socket(AF_INET, SOCK_STREAM, 0)), "Socket failed!");
  memset(&server_address, '0', sizeof(server_address));

  server_address.sin_family = AF_INET;
  server_address.sin_port = htons(port);

  check(
    inet_pton(AF_INET, "127.0.0.1", &server_address.sin_addr), 
    "Invalid address or address not supported"
  );

  check(
    connect(client_socket, (SA* ) &server_address, sizeof(server_address)),
    "Connection failed!"
  );

  return client_socket;
}

void check(int result, char* error_message) {
  const int ERROR = -1;
  if (result != ERROR) return;

  perror(error_message);
  exit(EXIT_FAILURE);
}

void read_from_server(void* data, int data_size, int mode) {
  int length = 0;

  switch (mode) {
    case BOOLEAN:
      read(client_socket, data, data_size);
      return;
      
    case INTEGER:
      read(client_socket, data, data_size);
      return;

    case STRING:
      bzero(data, data_size);
      read(client_socket, &length, sizeof(int));
      read(client_socket, data, length);
      return;

    default:
      printf("Unknown sending mode!\n");
  }
}