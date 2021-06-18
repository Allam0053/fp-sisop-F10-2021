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
void authenticate(const char*, const char* );
void send_to_server(const void*, int);
void read_from_server(void*, int, int);

int main(int argc, char const *argv[]) {
  const int USER = 2;
  const int PASSWORD = 4;
  const int DB_NAME = 5;
  const int ROOT_ID = 0;

  bool is_root = getuid() == ROOT_ID;
  bool keep_reading = false;
  bool db_exists = true;

  char user[50];
  char password[50];
  char request[50];
  char message[256];

  client_socket = connect_to_server(SERVER_PORT);

  strcpy(user, is_root ? "root" : argv[USER]);
  strcpy(password, is_root ? "" : argv[PASSWORD]);

  // Program akan terminate jika gagal authentication
  authenticate(user, password);

  sprintf(request, "reliability %s", argv[DB_NAME]);
  send_to_server(request, STRING);

  read_from_server(&db_exists, sizeof(db_exists), BOOLEAN);

  if (!db_exists) {
    printf("db does not exist!\n");
    close(client_socket);

    return 0;
  }

  do {
    read_from_server(&keep_reading, sizeof(keep_reading), BOOLEAN);

    if (!keep_reading) break;

    read_from_server(message, sizeof(message), STRING);
    printf("%s", message);
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

/* Helper */
void authenticate(const char* user, const char* password) {
  bool is_root = strcmp(user, "root") == 0;
  bool is_authenticated = false;

  send_to_server(&is_root, BOOLEAN);

  if (is_root) return;

  send_to_server(user, STRING);
  send_to_server(password, STRING);

  read_from_server(&is_authenticated, sizeof(is_authenticated), BOOLEAN);

  if (!is_authenticated) {
    printf("Invalid user or password!\n");
    close(client_socket);

    exit(EXIT_FAILURE);
  }
}

void send_to_server(const void* data, int mode) {
  int length = 0;

  switch (mode) {
    case BOOLEAN:
      send(client_socket, data, sizeof(bool), 0);
      return;
      
    case INTEGER:
      send(client_socket, data, sizeof(int), 0);
      return;

    case STRING:
      length = strlen(data);
      send(client_socket, &length, sizeof(int), 0);
      send(client_socket, data, length, 0);
      return;

    default:
      printf("Unknown sending mode!\n");
  }
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