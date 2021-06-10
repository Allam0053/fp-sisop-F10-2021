/*allam
gunakan code ini sebagai logic dari ddl
jika mau pake file lain dipersilakan
tapi untuk logic ddl sebisa mungkin 
code nya readable
*/

#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <stdio.h>

char currentUser[] = "root";
// CREATE USER [nama_user] IDENTIFIED BY [password_user];
// CREATE DATABASE [nama_database];
// CREATE TABLE [nama_tabel] ([nama_kolom] [tipe_data], ...);
// CREATE TABLE table2 (kolom1 string, kolom2 int, kolom3 string, kolom4 int);

bool isRoot (char *user); 
bool isCreate (char *command); //cheack if command is "CREATE"
void toLower (char *str); //lower case a string
int getOptCreate (char *opt); //get command option "COMMAND OPTION ..." -> get the option 1:user, 2:database, 3:table
bool steqIdentified (char *str); //steqIdentified mean is a given string eq to 'identified'
bool steqBy (char *str); //steqBy mean is a given string eq to 'by'

int main () {
    if (isRoot(currentUser)) {
        char input[] = "CREATE TABLE table (kolom1 string, kolom2 int, kolom3 string, kolom4 int);";
        char *command = strtok(input, " ");
        char *opt = strtok(NULL, " ");
        if (isCreate(command)) {
            switch (getOptCreate(opt)) {
                case 1:
                    printf("CREATE USER:\n");
                    char username[20];
                    char password[20];
                    char identified[15];
                    char by[5];
                    char *newUsername = strtok(NULL, " ");
                    char *tempId = strtok(NULL, " ");
                    char *tempBy = strtok(NULL, " ");
                    char *newPassword = strtok(NULL, "\0");
                    strcpy(username, newUsername);
                    strcpy(password, newPassword);
                    if (steqIdentified(tempId) && steqBy(tempBy)) { //command valid
                        printf("username: %s\npassword: %s\n", username, password);
                    } else {
                        printf("error\n");
                        break;
                    }
                    /*here goes how to create user. use variable username & password
                    *
                    */
                    break;

                case 2:
                    printf("CREATE DATABASE:\n");
                    char database[20];
                    char *newDatabase = strtok(NULL, "\0");
                    strcpy(database, newDatabase);
                    printf("database: %s\n", database);
                    /*here goes how to create database. use variable database to get the database's name
                    *
                    */
                    break;
                    
                case 3:
                    printf("CREATE TABLE\n");
                    char table[20];
                    char *newTable = strtok(NULL, " ");
                    char *columns = strtok(NULL, ";");
                    char *newColumns = strtok(columns+1, ")");
                    char *columnName[100];
                    char *columnType[100];
                    char *temp = strtok(newColumns, " ");
                    int i=0;
                    do {
                        columnName[i] = temp;
                        columnType[i++] = strtok(NULL, ",");
                        temp = strtok(NULL, " ");
                    } while(temp);
                    for (int j=0; j<i; j++) {
                        printf("kolom: %s\ntipe: %s\n\n", columnName[j], columnType[j]);
                    }
                    break;

                default:
                    printf("error\n");
            }
        }
        else {
            printf("error\n");
        }
    }
    printf("endofprogram\n");
    return 0;
}

bool isRoot (char *user) {
    if (strcmp(user, "root") == 0) {
        return true;
    }
    return false;
}

bool isCreate (char *command) {
    toLower(command);
    if (strcmp(command, "create") == 0) {
        return true;
    }
    return false;
}

bool steqIdentified (char *str) {
    toLower(str);
    if (strcmp(str, "identified") == 0) {
        return true;
    }
    return false;
}

bool steqBy (char *str) {
    toLower(str);
    if (strcmp(str, "by") == 0) {
        return true;
    }
    return false;
}

void toLower (char *str) {    
    for(int i = 0; str[i]; i++){
        str[i] = tolower(str[i]);
    }
    return;
}

int getOptCreate (char *opt) {
    toLower(opt);
    if (strcmp(opt, "user") == 0) {
        return 1;
    } else if (strcmp(opt, "database") == 0) {
        return 2;
    } else if (strcmp(opt, "table") == 0) {
        return 3;
    } else {
        return 0;
    }
}