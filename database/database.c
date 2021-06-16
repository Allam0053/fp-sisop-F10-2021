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

#define SERVER_PORT 8080
#define BYTES 1024
#define NOT_FOUND 404
#define BACKLOG 7
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

int client_socket;
bool is_root;
bool keep_handling;
char active_user[50];
char active_db[50];

// TODO: hapus logged_in, user_id, user_password
bool logged_in;
char user_id[50];
char user_password[50];

typedef struct sockaddr_in SA_IN;
typedef struct sockaddr SA;

int setup_server(short, int);
int accept_new_connection(int);
void handle_connection(int);

int translate_request(const char* );
void register_handler();
void logout_handler();
void login_handler();
void add_handler();
void download_handler();
void delete_handler();
void search_handler();
void to_lower (char *str); //lower case a string

off_t fsize(const char* );
void save_session(char*, char* );
void clear_session();
void record_log(int, const char* );
void send_to_client(const void*, int);
void read_from_client(void*, int, int);
int split_string(char [][100], char [], const char []);
void check(int, char* );

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

void handle_connection(int client_socket) {
  read_from_client(&is_root, sizeof(is_root), BOOLEAN);

  keep_handling = is_root;

  /* Authenticate for user */
  if (!is_root) {
    // * Kalo bukan root harus diautentikasi dulu
    char record_user[100];
    char user[50];
    char password[50];

    read_from_client(user, sizeof(user), STRING);
    read_from_client(password, sizeof(password), STRING);
    
    // Harus buka dari dari /database (jangan dari /fp)
    FILE* users_table = fopen("./database/databases/db_user/users.csv", "r");

    while (fgets(record_user, 100, users_table)) {
      char record_user_column[3][100];

      // remove '\n' character from fgets
      record_user[strcspn(record_user, "\n")] = 0;  
      split_string(record_user_column, record_user, ",");

      printf("user: %s\tpassword: %s\n", record_user_column[0], record_user_column[1]);

      // User exists
      if (strcmp(record_user_column[0], user) == 0 && strcmp(record_user_column[1], password) == 0) {
        keep_handling = true;
        fclose(users_table);
        break;
      }
    }

    // Kirim status apakah authentication berhasil / tidak
    send_to_client(&keep_handling, BOOLEAN);
  }

  // TODO: dihapus
  logged_in = false;

  char request[256];

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

void connect_db_handler(const char* request) {
  char request_copy[256];
  char splitted[2][100];
  char success_message[50];
  char error_message[50] = "invalid connect to database syntax";

  strcpy(request_copy, request);
  to_lower(request_copy);
  split_string(splitted, request_copy, " ");
  
  int response = CONNECT_DB;
  send_to_client(&response, INTEGER);
  
    
}

void create_db_handler(const char* request) {
  char request_copy[256];
  char splitted[3][100];
  char success_message[50];
  char error_message[50] = "invalid create database syntax";
  
  strcpy(request_copy, request);
  to_lower(request_copy);
  split_string(split_string, request_copy, " ");
  
  int response = CREATE_DB;
  send_to_client(&response, INTEGER);

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
  // 'request harus di ekstrak-ekstrak
  // GRANT PERMISSION database1 INTO user1;
  char request_copy[256];
  char splitted[5][100];
  char success_message[50];
  char error_message[50] = "invalid grant permission syntax";

  strcpy(request_copy, request);
  to_lower(request_copy);
  split_string(split_string, request_copy, " ");

  int response = GRANT_PERMISSION;
  send_to_client(&response, INTEGER);
  
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
  char record_data[256];
  char pswd[50];
  bool is_user_exists = false;
  
  FILE* users_table = fopen("./database/databases/db_user/users.csv", "r");

  while (fgets(record_data, 256, users_table)) {
    char splitted_record[3][100];
    
    record_data[strcspn(record_data, "\n")] = 0;
    split_string(splitted_record, record_data, ",");
    
    if (strcmp(splitted[4], splitted_record[0]) == 0) {
      is_user_exists = true;
      strcpy(pswd, splitted_record[1]);
      
      if (strcmp(splitted[2], splitted_record[2]) == 0) {
        char to_client_message[50] = "user has already access!";
        
        send_to_client(to_client_message, STRING);
        fclose(users_table);

        return;
      }
    }
  }

  fclose(users_table);

  if (!is_user_exists) {
    char to_client_message[50] = "user has already access!";
    send_to_client(to_client_message, STRING);
    return;
  }

  /* Add to users db */
  char to_client_message[50] = "user has access now";
  char new_user_access[256];

  users_table = fopen("./database/databases/db_user/users.csv", "a");
  sprintf(new_user_access, "%s,%s,%s", splitted[4], pswd, splitted[2]);
  fputs(new_user_access, users_table);

  fclose(users_table);

  send_to_client(to_client_message, STRING);
}

void insert_handler(const char* request) {
  // INSERT INTO table1 (‘value1’, 2, ‘value3’, 4);
  char request_copy[256];
  char splitted[50][100];
  char success_message[50];
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
  
  int i = 3;
  while (1) {
    if (strcmp(splitted[i], "") != 0) {
      //masukkan
    }
    i++;
  }
}

void update_handler() {
  
}

void delete_handler() {

}

void create_handler() {

}

void drop_handler() {

}

void select_handler() {

}


void logout_handler() {
  keep_handling = false;
  clear_session();
}

void register_handler() {
  char success_message[25] = "Account created";
  char id[50];
  char password[50];
  char account_data[100];

  printf("Reading id & password...\n");

  read_from_client(id, sizeof(id), STRING);
  read_from_client(password, sizeof(password), STRING);

  strcpy(account_data, id);
  strcat(account_data, ":");
  strcat(account_data, password);
  strcat(account_data, "\n");

  FILE* fp = fopen("./akun.txt", "a");
  fputs(account_data, fp);
  fclose(fp);

  send_to_client(success_message, STRING);
}

void login_handler() {
  send_to_client(&logged_in, BOOLEAN);
  if (logged_in) return;

  char success_message[50] = "Now you're logged in!";
  char failed_message[50] = "Incorrect id or password!";
  char id[50];
  char password[50];

  read_from_client(id, sizeof(id), STRING);
  read_from_client(password, sizeof(password), STRING);

  FILE* account_file = fopen("./akun.txt", "r");
  char data[256];
  char account[2][100];

  if (account_file == NULL) {
    FILE* create_account_file = fopen("./akun.txt", "a");
    fclose(create_account_file);

    account_file = fopen("./akun.txt", "r");
  }

  /* Iterating akun.txt */
  while (fgets(data, 256, account_file)) {
    data[strcspn(data, "\n")] = 0;  /* remove '\n' character from fgets */
    split_string(account, data, ":");

    const int ID = 0;
    const int PASS = 1;

    if (strcmp(id, account[ID]) == 0 && strcmp(password, account[PASS]) == 0) {
      logged_in = true;
      save_session(id, password);
      break;
    }
  }

  fclose(account_file);

  logged_in 
    ? send_to_client(success_message, STRING)
    : send_to_client(failed_message, STRING);
}

void add_handler() {
  send_to_client(&logged_in, BOOLEAN);

  if (!logged_in) return;

  int client_file_status;
  read_from_client(&client_file_status, sizeof(client_file_status), INTEGER);

  if (client_file_status == NOT_FOUND) return;

  int CHUNK_SIZE = 1 * BYTES;
  int total_recieved_size = 0;
  int bytes_read;
  char chunk[CHUNK_SIZE];
  char record_data[1024];
  int file_size;

  char publisher[50];
  char tahun[10];
  char file_name[50];
  char file_path[PATH_MAX];

  read_from_client(publisher, sizeof(publisher), STRING);
  read_from_client(tahun, sizeof(tahun), STRING);
  read_from_client(file_name, sizeof(file_name), STRING);

  sprintf(file_path, "./FILES/%s", file_name);

  FILE* database_file = fopen("./files.tsv", "a");
  FILE* destination_file = fopen(file_path, "wb");

  if (database_file == NULL || destination_file == NULL) {
    printf("Error open file!\n");
    return;
  }

  sprintf(record_data, "%s\t%s\t%s\n", file_path, tahun, publisher);
  fputs(record_data, database_file);

  read_from_client(&file_size, sizeof(file_size), INTEGER);

  double x;
  double y = (double) file_size;

  /* Proses utama menerima file dari client */
  do {
    bzero(chunk, CHUNK_SIZE);
    read(client_socket, &bytes_read, sizeof(int));
    read(client_socket, chunk, bytes_read);
    fwrite(chunk, 1, bytes_read, destination_file);

    total_recieved_size += bytes_read;
    x = (double) total_recieved_size;

    printf("recieved: %.1lf%%\r", 100 * x / y);
  } while (bytes_read >= CHUNK_SIZE);

  fclose(database_file);
  fclose(destination_file);

  printf("downloaded: 100%%\n");
  printf("File (%.1lf MB) was recieved from client.\n", x / 1000000);

  record_log(ADD, file_name);
}

void download_handler() {
  send_to_client(&logged_in, BOOLEAN);

  if (!logged_in) return;

  bool file_available = false;
  FILE* database_file = fopen("./files.tsv", "r");
  char file_path[100];
  char information[256];
  char requested_file[100];
  char records[3][100];

  read_from_client(requested_file, sizeof(requested_file), STRING);
  sprintf(file_path, "./FILES/%s", requested_file);

  /* Iterating files.tsv */
  while (fgets(information, 256, database_file) != NULL) {
    information[strcspn(information, "\n")] = 0; /* remove '\n' character from fgets */
    split_string(records, information, "\t");

    if (strcmp(file_path, records[0]) == 0) {
      printf("File available: (%s) (%s)\n", file_path, records[0]);
      file_available = true;
      break;
    }
  }

  send_to_client(&file_available, BOOLEAN);
  if (!file_available) return;

  FILE* source_file = fopen(file_path, "rb");

  if (source_file == NULL) {
    printf("Failed opening file!\n");
    return;
  }

  int CHUNK_SIZE = 1 * BYTES;
  int total_read_size = 0;
  unsigned char chunk[CHUNK_SIZE];
  int file_size = (int) fsize(file_path);

  send_to_client(&file_size, INTEGER);

  double x;
  double y = (double) file_size;

  while (!feof(source_file)) {
    int bytes_read = fread(chunk, 1, CHUNK_SIZE, source_file);
    send(client_socket, &bytes_read, sizeof(int), 0);
    send(client_socket, chunk, bytes_read, 0);

    total_read_size += bytes_read;
    x = (double) total_read_size;

    printf("sent: %.1lf%%\r", 100 * x / y);
  }
  
  fclose(source_file);
  printf("sent: 100%%\n");
  printf("File (%d bytes) was successfully sent!\n", total_read_size);
}

void delete_handler() {
  send_to_client(&logged_in, BOOLEAN);

  if (!logged_in) return;

  FILE* old_database_file = fopen("./files.tsv", "r");
  FILE* new_database_file = fopen("./new-files.tsv", "w");

  bool file_available = false;
  int requested_file_length = 0;
  char file_path[100];
  char new_file_path[100];
  char information[256];
  char copy_information[256];
  char requested_file[100];
  char filename[100];
  char records[3][100];

  read_from_client(requested_file, sizeof(requested_file), STRING);
  strcpy(filename, requested_file);

  // printf("deleted filename: %s\n", filename);

  sprintf(file_path, "./FILES/%s", requested_file);
  sprintf(new_file_path, "./FILES/old-%s", requested_file);

  // printf("deleted filename: %s\n", filename);

  /* Iterating files.tsv */
  while (fgets(information, 256, old_database_file) != NULL) {
    strcpy(copy_information, information);

    /* remove '\n' character from fgets */
    information[strcspn(information, "\n")] = 0;

    split_string(records, information, "\t");

    if (strcmp(file_path, records[0]) == 0) {
      printf("File available: (%s) (%s)\n", file_path, records[0]);
      file_available = true;
      continue;
    }

    fputs(copy_information, new_database_file);
  }

  // printf("deleted filename: %s\n", filename);

  send_to_client(&file_available, BOOLEAN);

  fclose(old_database_file);
  fclose(new_database_file);

  if (!file_available) {
    remove("./new-files.tsv");
    return;
  }

  remove("./files.tsv");
  rename("./new-files.tsv", "./files.tsv");
  rename(file_path, new_file_path);

  strcpy(filename, basename(file_path));
  record_log(DELETE, filename);
}

void search_handler() {
  send_to_client(&logged_in, BOOLEAN);
  
  if (!logged_in) return;

  char information[256];
  bool keep_read = false;
  FILE* database_file = fopen("./files.tsv", "r");

  do {
    keep_read = fgets(information, 256, database_file);
    information[strcspn(information, "\n")] = 0;

    send_to_client(&keep_read, BOOLEAN);
    
    if (!keep_read) break;
    
    send_to_client(information, STRING);
  } while (keep_read);

  fclose(database_file);
}

off_t fsize(const char *filename) {
  struct stat st; 

  if (stat(filename, &st) == 0) return st.st_size;
  return -1; 
}

void save_session(char* id, char* password) {
  strcpy(user_id, id);
  strcpy(user_password, password);
}

void clear_session() {
  bzero(user_id, 50);
  bzero(user_password, 50);
}

void record_log(int mode, const char* filename) {
  FILE* log = fopen("./running.log", "a");
  char log_format[256];
  bzero(log_format, 256);

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

  fputs(log_format, log);
  fclose(log);
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

int split_string(char splitted[][100], char origin[], const char delimiter[]) {
  int i = 0;
  char* token = strtok(origin, delimiter);
  
  while (token != NULL) {
    strcpy(splitted[i++], token);
    token = strtok(NULL, delimiter);
  }

  return i;
}

void check(int result, char* error_message) {
  const int ERROR = -1;
  if (result != ERROR) return;

  perror(error_message);
  exit(EXIT_FAILURE);
}

void to_lower (char *str) {    
    for (int i = 0; str[i]; i++) {
        str[i] = tolower(str[i]);
    }
    return;
}
