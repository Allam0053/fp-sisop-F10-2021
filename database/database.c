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
char active_password[50];
char active_db[50];

typedef struct sockaddr_in SA_IN;
typedef struct sockaddr SA;
typedef struct where_t{
  bool on;
  char asked_column[100];
  int table_column_index;
  char value[100];
} where_t;

/* Basic connection */
int setup_server(short, int);
int accept_new_connection(int);
void check(int, char* );
void handle_connection(int);

/* Hanlder functions */
int translate_request(const char* );

// DDL
void create_user_handler(char* );
void connect_db_handler(char* );
void grant_permission_handler(char* );
void create_db_handler(char* );
void create_table_handler(char* );
void drop_db_handler(char* );
void drop_table_handler(char* );
void drop_column_handler(char* );

// DML
void insert_handler(char* );
void update_handler(char* );
void delete_handler(char* );

void select_handler(char* );

/* Helper functions */
void get_user_password(char*, const char* );
bool authenticate_user();
bool is_user_has_db_access(const char* );
void send_to_client(const void*, int);
void read_from_client(void*, int, int);
void record_log(int, const char* );
int split_string(char [][100], char [], const char []);
int join_string (char[], char[][100], const char[]);
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

  /* Authenticate for user */
  keep_handling = authenticate_user();
  
  /* Kalau bukan root & gagal login -> keep_handling = false */
  while (keep_handling) {
    printf("Waiting for request...\n");
    read_from_client(request, sizeof(request), STRING);
    printf("Recieved request: %s\n", request);

    int t = translate_request(request);
    printf("req code = %d\n", t);
    switch (t) {
      case CREATE_USER:
        create_user_handler(request);
        break;

      case CONNECT_DB:
        connect_db_handler(request);
        break;
        
      case GRANT_PERMISSION:
        grant_permission_handler(request);
        break;

      case CREATE_DB:
        create_db_handler(request);
        break;

      case CREATE_TABLE:
        create_table_handler(request);
        break;

      case DROP_DB:
        drop_db_handler(request);
        break;

      case DROP_TABLE:
        // drop_table_handler(request);
        break;

      case DROP_COLUMN:
        drop_column_handler(request);
        break;

      case INSERT:
        insert_handler(request);
        break;
        
      case SELECT:
        select_handler(request);
        break;

      default: 
        printf("Invalid request!\n"); 
        break;
    }

    printf("\n");
  }

  close(client_socket);
}

/* Hanlder functions */
int translate_request(const char* request) {
  char request_copy[256];
  strcpy(request_copy, request);
  to_lower(request_copy);

  // TODO: error handler ketika token2 itu NULL (artinya tidak ada lagi word)
  char* token1 = strtok(request_copy, " ");
  char* token2 = strtok(NULL, " ");

  if (strcmp(token1, "create") == 0 && strcmp(token2, "user") == 0) return CREATE_USER;
  if (strcmp(token1, "create") == 0 && strcmp(token2, "database") == 0) return CREATE_DB;
  if (strcmp(token1, "create") == 0 && strcmp(token2, "table") == 0) return CREATE_TABLE;

  if (strcmp(token1, "drop") == 0 && strcmp(token2, "database") == 0) return DROP_DB;
  if (strcmp(token1, "drop") == 0 && strcmp(token2, "table") == 0) return DROP_TABLE;
  if (strcmp(token1, "drop") == 0 && strcmp(token2, "column") == 0) return DROP_COLUMN;

  if (strcmp(token1, "use") == 0) return CONNECT_DB;
  if (strcmp(token1, "grant") == 0) return GRANT_PERMISSION;
  if (strcmp(token1, "insert") == 0) return INSERT;
  if (strcmp(token1, "update") == 0) return UPDATE;
  if (strcmp(token1, "delete") == 0) return DELETE;
  if (strcmp(token1, "select") == 0) return SELECT;

  return -1;
}

// DDL
void create_user_handler(char* request) {
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

void connect_db_handler(char* request) {
  char request_copy[256];
  char request_word[2][100];
  char success_message[50];
  char record[256];
  char error_message[50] = "invalid connect to database syntax";

  strcpy(request_copy, request);
  to_lower(request_copy);
  split_string(request_word, request_copy, " ;");
  
  int response = CONNECT_DB;
  send_to_client(&response, INTEGER);
  
  bool has_access = is_user_has_db_access(request_word[1]);
  int status = has_access ? ACCEPTED : REJECTED;

  send_to_client(&status, INTEGER);

  if (!has_access) return;

  strcpy(active_db, request_word[1]);
  send_to_client(active_db, STRING);
}

void grant_permission_handler(char* request) {
  // GRANT PERMISSION database1 INTO user1;
  char request_copy[256];
  char splitted[5][100];
  char success_message[50];
  char error_message[50] = "invalid grant permission syntax";

  int res_code = GRANT_PERMISSION;
  send_to_client(&res_code, INTEGER);

  strcpy(request_copy, request);
  to_lower(request_copy);
  split_string(splitted, request_copy, " ;");

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

// TODO: update di users.csv
void create_db_handler(char* request) {
  char request_copy[256];
  char splitted[3][100];
  char success_message[50];
  char error_message[50] = "invalid create database syntax";
  
  int response = CREATE_DB;
  send_to_client(&response, INTEGER);

  strcpy(request_copy, request);
  to_lower(request_copy);
  split_string(splitted, request_copy, " ;");

  /* Error tests */
  if (strcmp(splitted[0], "create") != 0) {
    send_to_client(error_message, STRING);
    return;
  }

  if (strcmp(splitted[1], "database") != 0) {
    send_to_client(error_message, STRING);
    return;
  }
  
  // create db
  char db_name[100];
  sprintf(db_name, "%s%s", "./database/databases/", splitted[2]);
  mkdir(db_name, 0777);

  sprintf(success_message, "Success create db '%s'", splitted[2]);
  send_to_client(success_message, STRING);
}

// TODO: kalo active_db null -> cegah
void create_table_handler(char* request) {
  // CREATE TABLE mhs (nama string, umur int);
  // CREATE TABLE table1 (kolom1 string, kolom2 int, kolom3 string, kolom4 int);
  char request_copy[256];
  char splitted[50][100];
  char success_message[50] = "Success create value";
  char error_message[50] = "invalid create syntax";

  strcpy(request_copy, request);
  to_lower(request_copy);
  int len = split_string(splitted, request_copy, " ,=();");

  int response = CREATE_TABLE;
  send_to_client(&response, INTEGER);


  if (strcmp(active_db, "") == 0) {
    strcpy(error_message, "Connect to a database first.");
    send_to_client(error_message, STRING);
    return;
  }
  if (strcmp(splitted[0], "create") != 0) {
    send_to_client(error_message, STRING);
    return;
  }
  if (strcmp(splitted[1], "table") != 0) {
    send_to_client(error_message, STRING);
    return;
  }

  char path[256];
  char new_path[256];
  sprintf(path, "./database/databases/%s/%s.csv", active_db, splitted[2]);
  if (access(path, F_OK) == 0) {
    strcpy(error_message, "Table already exist!");
    send_to_client(error_message, STRING);
    return;
  }
  sprintf(new_path, "./database/databases/%s/%s.csv", active_db, splitted[2]);

  FILE* table_write = fopen(new_path, "w");


  char columns[256];
  char type_data[256];

  bzero(columns, 256);
  bzero(type_data, 256);

  for (int it = 3; it < len; it += 2) {
    if (strcmp(splitted[it], "") == 0 && (strcmp(splitted[it+1], "int") != 0 || strcmp(splitted[it+1], "string") != 0)) {
      send_to_client(error_message, STRING);
      return;
    }
    strcat(columns, splitted[it]);
    strcat(type_data, splitted[it+1]);
    if (it != len-2) strcat(columns, ",");
    else strcat(columns, "\n");
    if (it != len-2) strcat(type_data, ",");
    else strcat(type_data, "\n");
  }

  printf("columns: %s\n", columns);
  printf("type_data: %s\n", type_data);

  fputs(columns, table_write);
  fputs(type_data, table_write);
  /*
  FILE* users_table_append = fopen("./database/databases/db_user/users.csv", "a");
  char new_table[256];
  sprintf(new_table, "%s,%s,%s\n", active_user, active_password, splitted[2]);
  fputs(new_table, users_table_append);
  fclose(users_table_append);
  */
  fclose(table_write);

  send_to_client(success_message, STRING);
  return;
}

void drop_db_handler(char* request) {
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

      // TODO: masih salah fputs
      // fputs(column, users_table_write);
    }
    
    rmdir(db_path);
    
    closedir(dir);
  } else if (ENOENT == errno) {
    /* Directory does not exist. */
  } else {
    /* opendir() failed for some other reason. */
  }
}

void drop_column_handler(char* request) {
  int response_code = DROP_COLUMN;
  bool column_not_exist = true;
  
  FILE* table;
  FILE* new_table;
  int number_of_columns;
  int deleted_column_number;
  char req_token[5][100];
  char table_path[PATH_MAX];
  char new_table_path[PATH_MAX];
  char column_name[50];
  char table_name[50];
  char table_columns[256];
  char table_column_token[20][100];
  char message[50];

  send_to_client(&response_code, INTEGER);
  
  split_string(req_token, request, " ");
  strcpy(column_name, req_token[2]);
  strcpy(table_name, req_token[4]);
  sprintf(table_path, "./database/databases/%s/%s.csv", active_db, table_name);
  sprintf(new_table_path, "./database/databases/%s/new-%s.csv", active_db, table_name);

  // 1. apakah table ada?
  // 2. apakah kolom ada?
  // 3. lakukan copy file tanpa kolom lalu hapus

  // 1. apakah table ada?
  if (access(table_path, F_OK) != 0) {
    strcpy(message, "Table does not exits!");
    send_to_client(message, STRING);
    return;
  }

  // 2. apakah kolom ada?
  table = fopen(table_path, "r");
  fgets(table_columns, 256, table);
  fclose(table);
  
  table_columns[strcspn(table_columns, "\n")] = 0;
  number_of_columns = split_string(table_column_token, table_columns, ",");
  
  for (int i = 0; i < number_of_columns; i++) {
    if (strcmp(table_column_token[i], column_name) == 0) {
      column_not_exist = false;
      deleted_column_number = i;
      break;
    }
  }

  if (column_not_exist) {
    strcpy(message, "Column does not exist!");
    send_to_client(message, STRING);
    return;
  }

  // 3. lakukan copy file tanpa kolom lalu hapus
  table = fopen(table_path, "r");
  new_table = fopen(new_table_path, "w");

  while (fgets(table_columns, 256, table)) {
    table_columns[strcspn(table_columns, "\n")] = 0;
    split_string(table_column_token, table_columns, ",");

    for (int i = 0; i < number_of_columns; i++) {
      i - 1 == number_of_columns 
        ? strcat(table_column_token[i], "\n")
        : strcat(table_column_token[i], ",");

      fputs(table_column_token[i], new_table);
    }
  }

  fclose(table);
  fclose(new_table);
  
  remove(table_path);
  rename(new_table_path, table_path);

  strcpy(message, "Column deleted!");
  send_to_client(message, STRING);
}

// DML
void insert_handler(char* request) {
  // INSERT INTO table1 (‘value1’, 2, ‘value3’, 4);
  char request_copy[256];
  char splitted[50][100];
  char success_message[50] = "Success insert value";
  char error_message[50] = "invalid insert syntax";

  strcpy(request_copy, request);
  to_lower(request_copy);
  split_string(splitted, request_copy, ", ();");

  int response = INSERT;
  send_to_client(&response, INTEGER);

  if (strcmp(splitted[0], "insert") != 0) {
    send_to_client(error_message, STRING);
    return;
  }
  
  if (strcmp(splitted[1], "into") != 0) {
    send_to_client(error_message, STRING);
    return;
  }
  
  char path[256];
  char type_data[50][100];
  char columns[256];
  char input[500];
  bzero(input, 500);
  sprintf(path, "./database/databases/%s/%s.csv", active_db, splitted[2]);
  FILE* table_read = fopen(path, "r");
  FILE* table = fopen(path, "a");

  fgets(columns, 256, table_read); strcpy(columns, ""); //mau ambil type data tapi malah dapet nama kolom
  fgets(columns, 256, table_read); //ambil type_data
  int len = split_string(type_data, columns, ", \n");

  int i = 3;
  int j = 0;
  bool flag = true;

  while (j < len) {
    strcat(input, splitted[i]);
    if( j != len-1)strcat(input, ",");
    else strcat(input, "\n");
    i++;
    j++;
  }
  /* 
  USE mahasiswa;
  INSERT INTO table1 (‘value1’, 2, ‘value3’, 4);
  */
  if (flag) {
    input[strlen(input) - 1] = '\n';
    fputs(input, table);
    send_to_client(success_message, STRING);
  } else {
    printf("Salah di flag nya\n");
    send_to_client(error_message, STRING);
  }
  fclose(table);
  fclose(table_read);

  return;
}

void update_handler(char* request) {
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
  sprintf(new_path, "./database/databases/%s/new-%s.csv", active_db, splitted[1]);

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
    while (strcmp(splitted_columns[index_where++], splitted[6]) != 0); 

    index_where--;

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

  fclose(table_read);
  fclose(table_write);
  remove(path);
  rename(new_path, path);
}

void delete_handler(char* request) {
  // DELETE FROM table1;
  // DELETE FROM table1 WHERE kolom1=’value1’;

  char request_copy[256];
  char splitted[50][100];
  char success_message[] = "Success delete value";
  char error_message[] = "invalid delete syntax";

  strcpy(request_copy, request);
  to_lower(request_copy);
  split_string(splitted, request_copy, " ,=();");

  int response = UPDATE;
  send_to_client(&response, INTEGER);

  if (strcmp(splitted[0], "delete") != 0) {
    send_to_client(error_message, STRING);
    return;
  }
  if (strcmp(splitted[2], "from") != 0) {
    send_to_client(error_message, STRING);
    return;
  }

  char path[256];
  char new_path[256];
  sprintf(path, "./database/databases/%s/%s.csv", active_db, splitted[2]);
  sprintf(path, "./database/databases/%s/new-%s.csv", active_db, splitted[2]);

  FILE* table_read = fopen(path, "r");
  FILE* table_write = fopen(new_path, "w");
  char columns[256], cpy_column[256];
  char record_column[50][100];
  fgets(columns, 256, table_read);
  strcpy(cpy_column, columns);
  columns[strcspn(columns, "\n")] = 0;
  int total_columns = split_string(record_column, columns, ",");

  where_t condition;
  condition.on = false;
  if (strcmp(splitted[3], "where") == 0) {
    condition.on = true;
    strcpy(condition.asked_column, splitted[4]);
    strcpy(condition.value, splitted[5]);
    for (int it1 = 0; it1 <= total_columns; it1++) { //cari kolom
      if (strcmp(condition.asked_column, record_column[it1]) == 0) {
        condition.table_column_index = it1;
        break;
      }
    }
  }
  
  fputs(cpy_column, table_write);
  if (condition.on) {
    while (fgets(columns, 256, table_read)) {
      char splitted_column[50][100];
      char selected[50][100];
      
      strcpy(cpy_column, columns);
      columns[strcspn(columns, "\n")] = 0;  
      int total_columns = split_string(splitted_column, columns, ",");
      if (strcmp(splitted_column[condition.table_column_index], condition.value) != 0) { //menghapus yang tidak sama
        fputs(cpy_column, table_write);
      }
    }
  }

  //rename new-db
  fclose(table_read);
  fclose(table_write);
  remove(path);
  rename(new_path, path);
  send_to_client(success_message, STRING);
  return;
}

void select_handler(char* request) {
  // SELECT * FROM table1;
  // SELECT kolom1, kolom2 FROM table1;
  // SELECT kolom1, kolom2 FROM table1 WHERE kolomx = 'certain_value';
  // USE mahasiswa
  // SELECT * FROM table1;

  char request_copy[256];
  char splitted[50][100];
  char success_message[] = "Success select value";
  char error_message[] = "invalid select syntax";

  strcpy(request_copy, request);
  to_lower(request_copy);
  int size_command = split_string(splitted, request_copy, " ,=();"); 
  printf("test-case-1\n");

  int response = SELECT;
  send_to_client(&response, INTEGER);

  if (strcmp(splitted[0], "select") != 0) {
    send_to_client(error_message, STRING);
    return;
  }

  int i = 1;
  int where_column = -1;
  while (strcmp(splitted[++i], "from") != 0); 
  
  char path[256];
  sprintf(path, "./database/databases/%s/%s.csv", active_db, splitted[i + 1]);

  FILE* table_read = fopen(path, "r");
  int indexes[50];
  char columns[256];
  char record_column[50][100];
  fgets(columns, 256, table_read);
  columns[strcspn(columns, "\n")] = 0;
  int total_columns = split_string(record_column, columns, ",");
  int it3 = 0;

  /* ambil index kolom yang dibutuhkan, disimpan di int indexes[50] */
  if (strcmp(splitted[1], "*") == 0) {
    for (int it2 = 0; it2 < total_columns; it2++) {
      indexes[it3++] = it2;
    }
  } else {
    for (int it1 = 1; it1 < i; it1++) {
      for (int it2 = 0; it2 < total_columns; it2++) {
        if (strcmp(splitted[it1], record_column[it2]) == 0) {
          indexes[it3++] = it2;
        }
      }
    }
  }

  /*
  where_t condition;
  condition.on = false;
  if (strcmp(splitted[i + 3], "where") == 0) {
    condition.on = true;
    strcpy(condition.asked_column, splitted[i + 4]);
    strcpy(condition.value, splitted[i + 5]);
    for (int it1 = 0; it1 <= total_columns; it1++) {
      if (strcmp(condition.asked_column, record_column[it1]) == 0) {
        condition.table_column_index = it1;
        break;
      }
    }
  }
  */
  
  //send nama kolom
  bool keep_writing = false; //status to for client to keep read from server
  int send_column = 0;
  do {
    keep_writing = fgets(columns, 256, table_read);

    char data_to_send[500];
    char cpy_columns[500];
    char splitted_column[50][100];
    char selected[50][100];
    
    columns[strcspn(columns, "\n")] = 0;
    strcpy(cpy_columns, columns);
    int total_columns = split_string(splitted_column, columns, ",");
    
    send_to_client(&keep_writing, BOOLEAN);
    if (!keep_writing) break;

    // if (condition.on && send_column != 0) {
    //   handle_select_where(condition, );
    // }
    /* ambil yang diminta, taruh di char selected[50][100]
    if (condition.on || send_column != 0) {
      if (strcmp(splitted_column[condition.table_column_index], condition.value) == 0) { //if value match
        for (int it1 = 0; it1 < it3; it1++) {
          strcpy(selected[it1], splitted_column[indexes[it1]]);
          printf("test-case-100\n");
        }
        printf("test-case-10\n");
        join_string(data_to_send, selected, "\t");
        printf("test-case-11\n");

        send_to_client(columns, STRING); //send value
      }
    } else {*/
    for (int it1 = 0; it1 < it3; it1++) {
      strcpy(selected[it1], splitted_column[indexes[it1]]);
    }
    join_string(data_to_send, selected, "\t|\t");
    send_to_client(data_to_send, STRING); //send value
    // }
    
    send_column++;
  } while (keep_writing);
  printf("test-case-7\n");
  return;
}

void handle_select_where() {

}

/* Helper functions */
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

bool authenticate_user() {
  bool user_auth = true;
  char user[50];
  char password[50];
  char record[256];
  
  bzero(active_user, 50);
  bzero(active_db, 50);

  read_from_client(&is_root, sizeof(is_root), BOOLEAN);

  if (is_root) {
    strcpy(active_user, "root");
    strcpy(active_password, "");
    return true;
  }

  read_from_client(user, sizeof(user), STRING);
  read_from_client(password, sizeof(password), STRING);

  // Harus buka dari dari /database (jangan dari /fp)
  FILE* users_table = fopen("./database/databases/db_user/users.csv", "r");

  while (fgets(record, 256, users_table)) {
    char record_column[3][100];

    // remove '\n' character from fgets
    record[strcspn(record, "\n")] = 0;  
    split_string(record_column, record, ",");

    // Case user exists
    if (
      strcmp(record_column[0], user) == 0  
      && strcmp(record_column[1], password) == 0
    ) {
      user_auth = true;
      strcpy(active_user, user);
      strcpy(active_password, password);
      break;
    }
  }

  send_to_client(&user_auth, BOOLEAN);

  fclose(users_table);
  return user_auth;
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

// return size
int split_string(char splitted[][100], char origin[], const char delimiter[]) {
  int i = 0;
  char* token = strtok(origin, delimiter);
  
  while (token != NULL) {
    strcpy(splitted[i++], token);
    token = strtok(NULL, delimiter);
  }

  return i;
}

// return size of splitted[][100].
int join_string (char dest[], char splitted[][100], const char delimiter[]) {
  int i = 0;
  
  strcat(dest, splitted[i++]);
  while (strcmp(splitted[i], "") != 0) {
    strcat(dest, delimiter);
    strcat(dest, splitted[i++]);
  }
  return i;
}

void to_lower (char *str) {    
  for (int i = 0; str[i]; i++) {
    str[i] = tolower(str[i]);
  }
  return;
}
