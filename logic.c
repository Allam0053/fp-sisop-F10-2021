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

bool isRoot (char *user); 
bool isCreate (char *command); //cheack if command is "CREATE"
void toLower (char *str); //lower case a string
int getOptCreate (char *opt); //get command option "COMMAND OPTION ..." -> get the option 1:user, 2:database, 3:table
bool isIdentified (char *str);
bool isBy (char *str);

int main () {
    if (isRoot(currentUser)) {
        char input[] = "CREATE DATABASE db1";
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
                    if (isIdentified(tempId) && isBy(tempBy)) { //command valid
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

bool isIdentified (char *str) {
    toLower(str);
    if (strcmp(str, "identified") == 0) {
        return true;
    }
    return false;
}

bool isBy (char *str) {
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