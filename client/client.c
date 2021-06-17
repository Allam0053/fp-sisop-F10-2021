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

#define REJECTED -1
#define FINISHED 0

#define CREATE_USER 100
#define CONNECT_DB 101
#define GRANT_PERMISSION 102

#define CREATE_DB 200
#define CREATE_TABLE 201
#define DROP_DB 202
#define DROP_TABLE 203
#define DROP_COLUMN 204

#define INSERT 300
#define UPDATE 301
#define DELETE 302
#define SELECT 303

int client_socket;
bool keep_connected;
bool is_root;
bool is_db_connected;
char connected_db[50];

typedef struct sockaddr_in SA_IN;
typedef struct sockaddr SA;

/* Basic connection */
int connect_to_server(int);
void check(int , char* );
void handle_connection(int, const char*, const char* );

/* Handler function */
void connect_db_handler();
void select_handler();

/* Helper function */
void send_to_server(const void*, int);
void read_from_server(void*, int, int);
bool contains(const char* , const char* );
void split_string(char[][100], char*, const char* );


int main(int argc, char const *argv[]) {
  const int ROOT_ID = 0;
  is_root = getuid() == ROOT_ID;

  if (!is_root && argc != 5) {
    printf("Error: Invalid argument of command\n");
    return 0;
  }

  client_socket = connect_to_server(SERVER_PORT); 

  // Do whatever
  if (is_root) {
    handle_connection(client_socket, "root", NULL);
  } else {
    const int USERNAME = 2;
    const int PASSWORD = 4;

    handle_connection(client_socket, argv[USERNAME], argv[PASSWORD]);
  }

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

void handle_connection(int client_socket, const char* user, const char* password) {
  send_to_server(&is_root, BOOLEAN);

  /* Authentication for user */
  if (!is_root) {
    // Bukan root -> harus diautentikasi
    send_to_server(user, STRING);
    send_to_server(password, STRING);

    bool is_authenticated = false;
    read_from_server(&is_authenticated, sizeof(is_authenticated), BOOLEAN);

    if (!is_authenticated) {
      printf("Incorrect username or password!\n");
      keep_connected = false;
    }
  }
  
  strcpy(connected_db, "");
  
  is_db_connected = false;
  keep_connected = true;

  char request[256];
  char message[50];
  int response;

  while (keep_connected) {
    is_db_connected 
      ? printf("%s=%s -> ", connected_db, user)
      : printf("%s -> ", user);
    
    scanf("%[^\n]%*c", request);

    send_to_server(request, STRING);
    read_from_server(&response, sizeof(response), INTEGER);

    switch (response) {
      case CREATE_USER:
      case GRANT_PERMISSION:
      case CREATE_DB:
      case CREATE_TABLE:
      case DROP_DB:
      case DROP_TABLE:
      case DROP_COLUMN:
      case INSERT:
      case UPDATE:
      case DELETE:
        read_from_server(message, sizeof(message), STRING);
        printf("%s\n", message);
        break;

      case CONNECT_DB:
        connect_db_handler();
        break;

      case SELECT:
        select_handler();
        break;

      case FINISHED:
        keep_connected = false;
        break;
      
      default:
        printf("Unknown code from server!\n");
        break;
    }
  }
}


/* Handler function */
void connect_db_handler() {
  int status;
  read_from_server(&status, sizeof(status), INTEGER);

  if (status == REJECTED) {
    printf("db connection denied!\n");
    return;
  }

  is_db_connected = true;

  char db_name[50];
  read_from_server(db_name, sizeof(db_name), STRING);

  strcpy(connected_db, db_name);
  printf("Connected to database \"%s\"\n", db_name);
}

void select_handler() {
  int status;
  bool keep_reading = false;
  char record[256];

  read_from_server(&status, sizeof(status), INTEGER);

  if (status == REJECTED) {
    printf("Cannot perform SELECT\n");
    return;
  }
  
  do {
    read_from_server(&keep_reading, sizeof(keep_reading), BOOLEAN);

    if (!keep_reading) break;

    read_from_server(record, sizeof(record), STRING);
    printf("%s\n", record);
  } while (keep_reading);
}

int translate_request(const char* request) {

  return -1;
}


/* Helper function */
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

bool contains(const char* big, const char* small) {
  return strstr(big, small) != NULL;
}

void split_string(char splitted[][100], char* origin, const char* delimiter) {
  int i = 0;
  char* token = strtok(origin, delimiter);

  while (token) {
    strcpy(splitted[i++], token);
    token = strtok(NULL, delimiter);
  }
}
