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
#define BYTES 1024
#define NOT_FOUND 404
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

#define UPDATE_WHERE 400
#define DELETE_WHERE 401
#define SELECT_WHERE 402

#define LOGOUT 1
#define REGISTER 2
#define LOGIN 3
#define ADD 4
#define DOWNLOAD 5
#define DELETE 6
#define SEE 7
#define FIND 8

int client_socket; //ini setup server
bool keep_connected;
bool is_root;
bool is_db_connected;
char connected_db[50];

typedef struct sockaddr_in SA_IN;
typedef struct sockaddr SA;

int connect_to_server(int);
void handle_connection(int, const char*, const char* );

void connect_db_handler();
void select_handler();

int translate_request(const char* );
void authenticate_handler(int);
void logout_handler();
void register_handler();
void login_handler();
void add_handler();
void download_handler(char* );
void delete_handler(char* );
void search_handler(int, char* );

off_t fsize(const char * );
void send_to_server(const void*, int);
void read_from_server(void*, int, int);
bool is_autheticated();
void extract_request(char*, char*, char* );
void seperate(const char* , char* , char* );
char* get_file_name(const char* );
bool contains(const char* , const char* );
void split_string(char[][100], char*, const char* );
void check(int , char* );

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
      exit(EXIT_SUCCESS);
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
  int rows;
  char record[256];

  read_from_server(&status, sizeof(status), INTEGER);

  if (status == REJECTED) {
    printf("Cannot perform SELECT\n");
    return;
  }
  
  read_from_server(&rows, sizeof(rows), INTEGER);

  for (int i = 0; i < rows; i++) {
    read_from_server(record, sizeof(record), STRING);
    printf("%s\n", record);
  }
}

int translate_request(const char* request) {
  if (strcmp(request, "logout") == 0) return LOGOUT;
  if (strcmp(request, "register") == 0) return REGISTER;
  if (strcmp(request, "login") == 0) return LOGIN;
  if (strcmp(request, "add") == 0) return ADD;
  if (strcmp(request, "download") == 0) return DOWNLOAD;
  if (strcmp(request, "delete") == 0) return DELETE;
  if (strcmp(request, "see") == 0) return SEE;
  if (strcmp(request, "find") == 0) return FIND;

  return -1;
}


void register_handler() {
  char id[50];
  char password[50];
  char server_message[50];

  printf("Create account:\n");

  printf("Id >> ");
  scanf("%[^\n]%*c", id);
  send_to_server(id, STRING);

  printf("Password >> ");
  scanf("%[^\n]%*c", password);
  send_to_server(password, STRING);

  read_from_server(server_message, sizeof(server_message), STRING);
  printf("Message from server: %s\n", server_message);
}

void login_handler() {
  if (is_autheticated()) {
    printf("Already logged in\n");
    return;
  }

  char id[50];
  char password[50];
  char server_message[50];

  printf("Login:\n");

  printf("Id >> ");
  scanf("%[^\n]%*c", id);
  send_to_server(id, STRING);

  printf("Password >> ");
  scanf("%[^\n]%*c", password);
  send_to_server(password, STRING);

  read_from_server(server_message, sizeof(server_message), STRING);
  printf("Message from server: %s\n", server_message);
}

void add_handler() {
  if (!is_autheticated()) {
    printf("Login to add upload files.\n");
    return;
  }

  int CHUNK_SIZE = 1 * BYTES;
  int total_recieved_size = 0;
  unsigned char chunk[CHUNK_SIZE];

  char publisher[50];
  char tahun[10];
  char file_path[PATH_MAX];
  char file_path_copy[PATH_MAX];
  
  printf("Publisher: ");
  scanf("%[^\n]%*c", publisher);
  
  printf("Tahun publikasi: ");
  scanf("%[^\n]%*c", tahun);
  
  printf("Filepath: ");
  scanf("%[^\n]%*c", file_path);
  strcpy(file_path_copy, file_path);
  FILE* source_file = fopen(file_path, "rb");
  char* file_name = basename(file_path);

  int file_status = source_file == NULL ? NOT_FOUND : 1;
  send_to_server(&file_status, INTEGER);

  if (source_file == NULL) {
    printf("Failed opening file!\n");
    return;
  }

  send_to_server(publisher, STRING);
  send_to_server(tahun, STRING);
  send_to_server(file_name, STRING);

  int file_size = (int) fsize(file_path_copy);

  send_to_server(&file_size, INTEGER);

  double x;
  double y = (double) file_size;

  while (!feof(source_file)) {
    int bytes_read = fread(chunk, 1, CHUNK_SIZE, source_file);
    send(client_socket, &bytes_read, sizeof(int), 0);
    send(client_socket, chunk, bytes_read, 0);

    total_recieved_size += bytes_read;
    x = (double) total_recieved_size;

    printf("sent: %.1lf%%\r", 100 * x / y);
  }
  
  fclose(source_file);
  printf("sent: 100%%\n");
  printf("File (%.1lf MB) was sent to server.\n", x / 1000000);
}

void download_handler(char* file_name) {
  if (!is_autheticated()) {
    printf("Login to download files.\n");
    return;
  }

  bool file_available = false;

  send_to_server(file_name, STRING);
  read_from_server(&file_available, sizeof(file_available), BOOLEAN);

  if (!file_available) {
    printf("File is not available\n");
    return;
  }

  FILE* destination_file = fopen(file_name, "wb");

  if (destination_file == NULL) {
    printf("Failed opening file!\n");
    return;
  }

  int CHUNK_SIZE = 1 * BYTES;
  int total_recieved_size = 0;
  int bytes_read;
  int file_size;
  unsigned char chunk[CHUNK_SIZE];

  read_from_server(&file_size, sizeof(file_size), INTEGER);

  double x;
  double y = (double) file_size;

  do {
    bzero(chunk, CHUNK_SIZE);
    read(client_socket, &bytes_read, sizeof(int));
    read(client_socket, chunk, bytes_read);
    fwrite(chunk, 1, bytes_read, destination_file);

    total_recieved_size += bytes_read;
    x = (double) total_recieved_size;

    printf("downloaded: %.1lf%%\r", 100 * x / y);
  } while (bytes_read >= CHUNK_SIZE);

  fclose(destination_file);
  printf("downloaded: 100%%\n");
  printf("File (%.1lf MB) was downloaded from server.\n", x / 1000000);
}

void delete_handler(char* file_name) {
  bool file_available = false;

  if (!is_autheticated()) {
    printf("You must login first!\n");
    return;
  }

  send_to_server(file_name, STRING);
  read_from_server(&file_available, sizeof(file_available), BOOLEAN);

  if (!file_available) {
    printf("File is not available\n");
    return;
  }

  printf("%s was deleted!!\n", file_name);
}

void search_handler(int mode, char* keyword) {
  bool file_available = false;

  if (!is_autheticated()) {
    printf("You must login first!\n");
    return;
  }

  const int FILE_PATH = 0;
  const int TAHUN = 1;
  const int PUBLISHER = 2;
  bool do_filter = mode == FIND;
  bool keep_read = false;
  char plain_filename[50];
  char file_extension[50];
  char information[256];
  char records[3][100];

  do {
    read_from_server(&keep_read, sizeof(keep_read), BOOLEAN);

    if (!keep_read) break;

    read_from_server(information, sizeof(information), STRING);

    split_string(records, information, "\t");
    seperate(records[FILE_PATH], plain_filename, file_extension);
    
    if (do_filter && !contains(get_file_name(records[FILE_PATH]), keyword)) continue;

    printf("Nama: %s\n", plain_filename);
    printf("Publisher: %s\n", records[PUBLISHER]);
    printf("Tahun publishing: %s\n", records[TAHUN]);
    printf("Ekstensi: %s\n", file_extension);
    printf("Filepath: %s\n\n", records[FILE_PATH]);

  } while (keep_read);

}

off_t fsize(const char *filename) {
  struct stat st; 

  if (stat(filename, &st) == 0) return st.st_size;
  return -1; 
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

bool is_autheticated() {
  bool logged_in = false;
  read_from_server(&logged_in, sizeof(logged_in), BOOLEAN);
  return logged_in;
}

void extract_request(char* command, char* main_command, char* second_command) {
  int i = 1;
  char* token = strtok(command, " ");
  while (token) {
    switch (i++) {
      case 1: strcpy(main_command, token); break;
      case 2: strcpy(second_command, token); break;
      default: 
        strcat(second_command, " ");
        strcat(second_command, token);
        break;
    }
    token = strtok(NULL, " ");
  }
}

void seperate(const char* path, char* plain, char* ext) {
  char piece[2][100];
  char copy_path[100];
  strcpy(copy_path, path);

  char* filename = basename(copy_path);
  char copy_filename[100];

  strcpy(copy_filename, filename);

  split_string(piece, copy_filename, ".");
  strcpy(plain, piece[0]);
  strcpy(ext, piece[1]);
}

char* get_file_name(const char* path) {
  char copy_path[100];
  strcpy(copy_path, path);

  return basename(copy_path);
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

void check(int result, char* error_message) {
  const int ERROR = -1;
  if (result != ERROR) return;

  perror(error_message);
  exit(EXIT_FAILURE);
}

