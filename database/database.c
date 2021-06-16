#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <limits.h>
#include <libgen.h>
#include <ctype.h>
#include <dirent.h>
#include <errno.h>

#define SERVER_PORT 8080
#define BACKLOG 7

#define BOOLEAN 1
#define INTEGER 2
#define STRING 3

#define REJECTED -1
#define FINISHED 0
#define ACCEPTED 1

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
bool is_root;
bool keep_handling;
char active_user[50];
char active_db[50];

typedef struct sockaddr_in SA_IN;
typedef struct sockaddr SA;

/* Basic connection */
int setup_server(short, int);
int accept_new_connection(int);
void check(int, char* );
void handle_connection(int);

/* Hanlder functions */
int translate_request(const char* );
void connect_db_handler(const char* request);
void create_db_handler(const char* request);
void grant_permission_handler(const char* request);
void insert_handler(const char* request);
void update_handler();
void delete_handler();
void create_handler();
void drop_db_handler(const char* request);
void drop_table_handler(const char* request);
void drop_column_handler(const char* request);
void select_handler();

/* Helper functions */
void get_user_password(char*, const char* );
bool authenticate_user(const char*, const char* );
bool is_user_has_db_access(const char* );
void send_to_client(const void*, int);
void read_from_client(void*, int, int);
void record_log(int, const char* );
int split_string(char [][100], char [], const char []);
void to_lower (char *str);

int main(int argc, char const *argv[]) {
  int server_socket = setup_server(SERVER_PORT, BACKLOG);

  while (true) {
    printf("Waiting for connections...\n");
    client_socket = accept_new_connection(server_socket);
    printf("Connected!!\n\n");

    // Do whatever we want
    handle_connection(client_socket);
  }

  close(server_socket);
  return EXIT_SUCCESS;
}

int setup_server(short port, int backlog) {
  int opt = 1;
  int server_socket;
  SA_IN server_address;

  check(
    (server_socket = socket(AF_INET, SOCK_STREAM, 0)), 
    "Failed creating socket!"
  );

  check(
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)), 
    "Failed setting socket option!"
  );

  // Initialize the address struct
  server_address.sin_family = AF_INET;
  server_address.sin_addr.s_addr = INADDR_ANY;
  server_address.sin_port = htons(port);

  check(
    bind(server_socket, (SA* ) &server_address, sizeof(server_address)),
    "Bind Failed!"
  );

  check(
    listen(server_socket, backlog),
    "Listend Failed!"
  );

  return server_socket;
}

int accept_new_connection(int server_socket) {
  int address_size = sizeof(SA_IN);
  int client_socket;
  SA_IN client_address;

  check(
    (client_socket = 
      accept(
        server_socket, 
        (SA* ) &client_address, 
        (socklen_t* ) &address_size
      )),
    "Accept Failed!"
  );

  return client_socket;
}

void check(int result, char* error_message) {
  const int ERROR = -1;
  if (result != ERROR) return;

  perror(error_message);
  exit(EXIT_FAILURE);
}

void handle_connection(int client_socket) {
  char request[256];
  char user[50];
  char password[50];

  read_from_client(&is_root, sizeof(is_root), BOOLEAN);

  is_root ? strcpy(active_user, "root") : NULL;
  keep_handling = is_root;

  /* Authenticate for user */
  if (!is_root) {
    read_from_client(user, sizeof(user), STRING);
    read_from_client(password, sizeof(password), STRING);

    keep_handling = authenticate_user(user, password);
    keep_handling ? strcpy(active_user, user) : NULL;

    send_to_client(&keep_handling, BOOLEAN);
  }
  
  // Kalau bukan root & gagal login -> keep_handling = false
  while (keep_handling) {
    printf("Waiting for request...\n");
    read_from_client(request, sizeof(request), STRING);
    printf("Recieved request: %s\n", request);

    // TODO: perbaiki translate
    switch (translate_request(request)) {
      case GRANT_PERMISSION:
        grant_permission_handler(request);
        break;

      default: printf("Invalid request!\n"); break;
    }

    printf("\n");
  }

  close(client_socket);
}

int translate_request(const char* request) {
  char request_copy[256];
  strcpy(request_copy, request);
  to_lower(request_copy);

  // TODO: error handler ketika token2 itu NULL (artinya tidak ada lagi word)
  char* token1 = strtok(request_copy, " ");
  char* token2 = strtok(NULL, " ");

  if (strcmp(token1, "use") == 0) return CONNECT_DB;
  if (strcmp(token1, "grant") == 0) return GRANT_PERMISSION;
  if (strcmp(token1, "insert") == 0) return INSERT;
  if (strcmp(token1, "update") == 0) return UPDATE;
  if (strcmp(token1, "delete") == 0) return DELETE;
  if (strcmp(token1, "select") == 0) return SELECT;

  if (strcmp(token1, "create") == 0 && strcmp(token2, "user")) return CREATE_USER;
  if (strcmp(token1, "create") == 0 && strcmp(token2, "database")) return CREATE_DB;
  if (strcmp(token1, "create") == 0 && strcmp(token2, "table")) return CREATE_TABLE;

  if (strcmp(token1, "drop") == 0 && strcmp(token2, "database")) return DROP_DB;
  if (strcmp(token1, "drop") == 0 && strcmp(token2, "table")) return DROP_TABLE;
  if (strcmp(token1, "drop") == 0 && strcmp(token2, "column")) return DROP_COLUMN;

  return -1;
}

void create_user_handler(const char* request) {
  char request_copy[256];
  char new_user[256];
  char splitted_request[6][100];

  int response_code = CREATE_USER;
  send_to_client(&response_code, INTEGER);

  if (!is_root) {
    char denied_message[50] = "Login as root to create user!";
    send_to_client(denied_message, STRING);
    return;
  }

  strcpy(request_copy, request);
  split_string(splitted_request, request_copy, " ");
  sprintf(new_user, "%s,%s,\n", splitted_request[2], splitted_request[5]);
  
  FILE* users_table = fopen("./database/databases/db_user/users.csv", "a");
  char message[50] = "User created!";
  
  send_to_client(message, STRING);
  fputs(new_user, users_table);
  fclose(users_table);
}

void connect_db_handler(const char* request) {
  char request_copy[256];
  char request_word[2][100];
  char success_message[50];
  char record[256];
  char error_message[50] = "invalid connect to database syntax";

  strcpy(request_copy, request);
  to_lower(request_copy);
  split_string(request_word, request_copy, " ");
  
  int response = CONNECT_DB;
  send_to_client(&response, INTEGER);
  
  bool has_access = is_user_has_db_access(request_word[1]);
  int status = has_access ? ACCEPTED : REJECTED;

  send_to_client(&status, INTEGER);

  if (!has_access) return;

  strcpy(active_db, request_word[1]);
  send_to_client(active_db, STRING);
}

void create_db_handler(const char* request) {
  char request_copy[256];
  char splitted[3][100];
  char success_message[50];
  char error_message[50] = "invalid create database syntax";
  
  int response = CREATE_DB;
  send_to_client(&response, INTEGER);

  strcpy(request_copy, request);
  to_lower(request_copy);
  split_string(split_string, request_copy, " ");

  /* Error tests */
  if (strcmp(splitted[0], "create") != 0) {
    send_to_client(error_message, STRING);
    return;
  }

  if (strcmp(splitted[1], "database") != 0) {
    send_to_client(error_message, STRING);
    return;
  }
  
  // Hapus semicolon
  splitted[2][strcspn(splitted[2], ";")] = 0; 
  
  // create db
  char db_name[100];
  sprintf(db_name, "%s%s", "./database/databases/", splitted[2]);
  mkdir(db_name, 0777);

  sprintf(success_message, "Success create db '%s'", splitted[2]);
  send_to_client(success_message, STRING);
}

void grant_permission_handler(const char* request) {
  // GRANT PERMISSION database1 INTO user1;
  char request_copy[256];
  char splitted[5][100];
  char success_message[50];
  char error_message[50] = "invalid grant permission syntax";

  int res_code = GRANT_PERMISSION;
  send_to_client(&res_code, INTEGER);

  strcpy(request_copy, request);
  to_lower(request_copy);
  split_string(split_string, request_copy, " ");

  if (!is_root) {
    char denied_message[50] = "Only root can grant permission!";
    send_to_client(denied_message, STRING);
    return;
  }
  
  if (strcmp(splitted[0], "grant") != 0) {
    send_to_client(error_message, STRING);
    return;
  }

  if (strcmp(splitted[1], "permission") != 0){
    send_to_client(error_message, STRING);
    return;
  }

  if (strcmp(splitted[3], "into") != 0){
    send_to_client(error_message, STRING);
    return;
  }

  /* Get password */
  char password[50];
  get_user_password(password, splitted[4]);

  /* Add to users db */
  char message[50] = "user has access now";
  char new_user_access[256];

  FILE* users_table = fopen("./database/databases/db_user/users.csv", "a");
  sprintf(new_user_access, "%s,%s,%s\n", splitted[4], password, splitted[2]);
  fputs(new_user_access, users_table);

  fclose(users_table);

  send_to_client(message, STRING);
}

void insert_handler(const char* request) {
  // INSERT INTO table1 (‘value1’, 2, ‘value3’, 4);
  char request_copy[256];
  char splitted[50][100];
  char success_message[50] = "Success inserting value";
  char error_message[50] = "invalid insert permission syntax";

  strcpy(request_copy, request);
  to_lower(request_copy);
  split_string(splitted, request_copy, ", ();");

  int response = INSERT;
  send_to_client(&response, INTEGER);

  if (strcmp(splitted[0], "insert") != 0) {
    send_to_client(error_message, STRING);
    return;
  }
  if (strcmp(splitted[0], "into") != 0) {
    send_to_client(error_message, STRING);
    return;
  }
  
  char path[256];
  char type_data[50][100];
  char columns[256];
  char input[500];
  sprintf(path, "./database/databases/%s/%s.csv", active_db, splitted[2]);
  FILE* table = fopen(path, "a");

  fgets(columns, 256, table); strcpy(columns, ""); //mau ambil type data tapi malah dapet nama kolom
  fgets(columns, 256, table); //ambil type_data
  split_string(type_data, columns, ", \n");

  int i = 3;
  int j = 0;
  bool flag = true;
  while (1) {
    if (strcmp(splitted[i], "") != 0 && strcmp(type_data[j], "") != 0) {
      if (strcmp(splitted[i][0], "'") == 0) {
        if (strcmp(type_data[j], "string") == 0) {
          flag = true;
        } else { 
          flag = false;
          break;
        }
      } else {
        if (strcmp(type_data[j], "int") == 0) {
          flag = true;
        } else { 
          flag = false;
          break;
        }
      }
    } else {
      flag = false;
      break;
    }
    strcat(input, splitted[i]);
    strcat(input, ",");
    i++;
    j++;
  }
  
  if (flag) {
    input[strlen(input) - 1] = '\n';
    fputs(input, table);
    send_to_client(success_message, STRING);
  } else {
    send_to_client(error_message, STRING);
  }

  return;
}

void update_handler(const char* request) {
  // UPDATE table1 SET kolom1='new_value1';
  // UPDATE table1 SET kolom1='new_value1' WHERE kolomx = 'certain_value';
  
  char request_copy[256];
  char splitted[50][100];
  char success_message[] = "Success update value";
  char error_message[] = "invalid update syntax";

  strcpy(request_copy, request);
  to_lower(request_copy);
  split_string(splitted, request_copy, " ,=();");

  int response = UPDATE;
  send_to_client(&response, INTEGER);

  if (strcmp(splitted[0], "update") != 0) {
    send_to_client(error_message, STRING);
    return;
  }
  if (strcmp(splitted[2], "set") != 0) {
    send_to_client(error_message, STRING);
    return;
  }

  char path[256];
  char new_path[256];
  char type_data[50][100];
  char columns[256];
  char copy_columns[256];
  char splitted_columns[50][100];
  char input[500];
  sprintf(path, "./database/databases/%s/%s.csv", active_db, splitted[1]);
  sprintf(path, "./database/databases/%s/new-%s.csv", active_db, splitted[1]);

  FILE* table_read = fopen(path, "r");
  FILE* table_write = fopen(new_path, "w");

  fgets(columns, 256, table_read);
  if (!strstr(columns, splitted[3])) {
    send_to_client(error_message, STRING);
    return;
  }
  fputs(columns, table_write);
  strcpy(copy_columns, columns);
  split_string(splitted_columns, copy_columns, ", \n");
  int i = 0;
  while (strcmp(splitted_columns[i++], splitted[3]) != 0);
  i--;

  fgets(columns, 256, table_read);
  fputs(columns, table_write);
  
  //jika ketambahan where
  if (strcmp(splitted[5], "where") == 0) {
    int index_where = 0;
    while (strcmp(splitted_columns[index_where++], splitted[6]) != 0); index_where--;
    while(fgets(columns, 256, table_read)) {
      char record_column[20][100];
      columns[strcspn(columns, "\n")] = 0;  
      int total_columns = split_string(record_column, columns, ",");

      if (strcmp(record_column[index_where], splitted[7])) {
        strcpy(record_column[i], splitted[4]);
      }
      int j = 0;
      while (j < total_columns) {
        fputs(record_column[j++], table_write);
        j < total_columns - 1 ? fputs(",", table_write) : fputs("\n", table_write);
      }
    }
  } else {
    while(fgets(columns, 256, table_read)) {
      char record_column[20][100];
      
      columns[strcspn(columns, "\n")] = 0;  
      int total_columns = split_string(record_column, columns, ",");

      strcpy(record_column[i], splitted[4]);
      int j = 0;
      while (j < total_columns) {
        fputs(record_column[j++], table_write);
        j < total_columns - 1 ? fputs(",", table_write) : fputs("\n", table_write);
      }
    }
  }
}

void delete_handler() {

}

void create_handler() {

}

void drop_db_handler(const char* request) {
  char cpy_req[256];
  char req_split[3][100];
  char db_path[PATH_MAX];

  int res_code = DROP_DB;
  send_to_client(&res_code, INTEGER);

  strcpy(cpy_req, request);
  split_string(req_split, cpy_req, " ");

  bool has_access = is_user_has_db_access(req_split[2]);
  
  if (!has_access) {
    char denied_message[50] = "Database access denied!";
    send_to_client(denied_message, STRING);
    return;
  }

  strcpy(db_path, "./database/databases/");
  strcat(db_path, req_split[2]);

  DIR* dir = opendir("mydir");
  
  if (dir) {
    /* Directory exists. */
    FILE* users_table_read = fopen("./database/databases/db_user/users.csv", "r");
    FILE* users_table_write = fopen("./database/databases/db_user/new-users.csv", "w");

    char record[256];
    while (fgets(record, 256, users_table_read)) {
      char column[5][100];
      char copy_record[256];

      strcpy(copy_record, record);
      
      record[strcspn(record, "\n")] = 0;

      split_string(column, copy_record, ",");

      if (strcmp(column[2], req_split[2]) == 0) continue;

      fputs(column, users_table_write);
    }
    
    rmdir(db_path);
    
    closedir(dir);
  } else if (ENOENT == errno) {
    /* Directory does not exist. */
  } else {
    /* opendir() failed for some other reason. */
  }
}

void select_handler(const char* request) {
  // SELECT * FROM table1;
  // SELECT kolom1, kolom2 FROM table1;
  // SELECT kolom1, kolom2 FROM table1 WHERE kolomx = 'certain_value';

  char request_copy[256];
  char splitted[50][100];
  char success_message[] = "Success select value";
  char error_message[] = "invalid select syntax";

  strcpy(request_copy, request);
  to_lower(request_copy);
  int size_command = split_string(splitted, request_copy, " ,=();");

  int response = SELECT;
  send_to_client(&response, INTEGER);

  if (strcmp(splitted[0], "select") != 0) {
    send_to_client(error_message, STRING);
    return;
  }

  int i = 1;
  bool where = false;
  while (strcmp(splitted[++i], "from") != 0); i--;

  if (strcmp(splitted[i+3], "where") == 0) {
    where = true;
  }

  char path[256];
  sprintf(path, "./database/databases/%s/%s.csv", active_db, splitted[1]);

  FILE* table_read = fopen(path, "r");
  char columns[256];
  char record_column[20][100];
  fgets(columns, 256, table_read);
  columns[strcspn(columns, "\n")] = 0;
  int total_columns = split_string(record_column, columns, ",");
  for (int k = i; k > 0; k--) {
    while (1); //cek cari kolom yang diselect
  }
  
  
  while(fgets(columns, 256, table_read)) {
    char record_column[20][100];
    
    columns[strcspn(columns, "\n")] = 0;  
    int total_columns = split_string(record_column, columns, ",");

    strcpy(record_column[i], splitted[4]);
    int j = 0;
    while (j < total_columns) {
      fputs(record_column[j++], table_write);
      j < total_columns - 1 ? fputs(",", table_write) : fputs("\n", table_write);
    }
  }
}

void get_user_password(char* password, const char* username) {
  char record[256];
  FILE* users_table = fopen("./database/databases/db_user/users.csv", "r");

  while (fgets(record, 256, users_table)) {
    char record_column[3][100];

    // remove '\n' character from fgets
    record[strcspn(record, "\n")] = 0;  
    split_string(record_column, record, ",");
    
    if (strcmp(record_column[0], username) == 0) {
      strcpy(password, record_column[1]);
      fclose(users_table);
      return;
    }
  }

  strcpy(password, "\0");
  fclose(users_table);
}

bool authenticate_user(const char* username, const char* password) {
  char record[256];
  
  // Harus buka dari dari /database (jangan dari /fp)
  FILE* users_table = fopen("./database/databases/db_user/users.csv", "r");

  while (fgets(record, 256, users_table)) {
    char record_column[3][100];

    // remove '\n' character from fgets
    record[strcspn(record, "\n")] = 0;  
    split_string(record_column, record, ",");

    // Case user exists
    if (
      strcmp(record_column[0], username) == 0  
      && strcmp(record_column[1], password) == 0
    ) {
      fclose(users_table);
      return true;
    }
  }

  fclose(users_table);
  return false;
}

bool is_user_has_db_access(const char* db_name) {
  char record[256];
  
  FILE* users_table = fopen("./database/databases/db_user/users.csv", "r");

  while (fgets(record, 256, users_table)) {
    char record_column[3][100];

    // remove '\n' character from fgets
    record[strcspn(record, "\n")] = 0;  
    split_string(record_column, record, ",");

    // Case user has access
    if (
      strcmp(record_column[0], active_user) == 0 
      && strcmp(record_column[2], db_name) == 0 
    ) {
      fclose(users_table);
      return true;
    }
  }

  fclose(users_table);
  return false;
}

void send_to_client(const void* data, int mode) {
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

void read_from_client(void* data, int data_size, int mode) {
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

void record_log(int mode, const char* filename) {
  FILE* log = fopen("./running.log", "a");
  char log_format[256];
  bzero(log_format, 256);
  /* 
  switch (mode) {
    case ADD:
      sprintf(log_format, "Tambah : %s (%s:%s)\n", filename, user_id, user_password);
      break;

    case DELETE:
      sprintf(log_format, "Hapus : %s (%s:%s)\n", filename, user_id, user_password);
      break;

    default:
      printf("Unknown log mode!\n");
      break;
  }
  */
  fputs(log_format, log);
  fclose(log);
}

int split_string(char splitted[][100], char origin[], const char delimiter[]) {
  int i = 0;
  char* token = strtok(origin, delimiter);
  
  while (token != NULL) {
    strcpy(splitted[i++], token);
    token = strtok(NULL, delimiter);
  }

  return i;
}

void to_lower (char *str) {    
  for (int i = 0; str[i]; i++) {
    str[i] = tolower(str[i]);
  }
  return;
}
