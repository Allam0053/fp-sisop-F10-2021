#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <stdio.h>

char currentUser[] = "root";
// CREATE USER [nama_user] IDENTIFIED BY [password_user];
// CREATE DATABASE [nama_database];
// CREATE TABLE [nama_tabel] ([nama_kolom] [tipe_data], ...);
// CREATE TABLE table2 (kolom1 string, kolom2 int, kolom3 string, kolom4 int);
// INSERT INTO table1 (‘abc’, 1, ‘bcd’, 2);
// UPDATE table1 SET kolom1=’new_value1’;
// [Command UPDATE, SELECT, DELETE] WHERE [nama_kolom]=[value];

bool isRoot (char *user); 

void toLower (char *str); //lower case a string
bool isCreate (char *command); //cheack if command is "CREATE"
int getOptCreate (char *opt); //get command option "COMMAND OPTION ..." -> get the option 1:user, 2:database, 3:table
bool steqIdentified (char *str); //steqIdentified mean is a given string eq to 'identified'
bool steqBy (char *str); //steqBy mean is a given string eq to 'by'

bool isInsert (char *command);
bool steqInto (char *into);

bool isUpdate (char *command);
bool steqSet (char *set);

bool isDelete (char *command);
bool steqFrom (char *from);

bool isSelect (char *command);

int main () {
    if (isRoot(currentUser)) {
        char input[] = "SELECT kolom1, kolom2 FROM table1;";
        char *command = strtok(input, " ");
        if (isCreate(command)) {
        	char *opt = strtok(NULL, " ");
            switch (getOptCreate(opt)) {
						case 1:;
							printf("CREATE USER:\n");
							char username[20];
							char password[20];
							char identified[15];
							char by[5];
							char *newUsername = strtok(NULL, " ");
							char *tempId = strtok(NULL, " ");
							char *tempBy = strtok(NULL, " ");
							char *newPassword = strtok(NULL, ";");
							strcpy(username, newUsername);
							strcpy(password, newPassword);
							if (steqIdentified(tempId) && steqBy(tempBy)) { //command valid
								printf("username: %s\npassword: %s\n", username, password);
							} else {
								printf("error\n");
								break;
							}
							/*here goes how to create user. use variable char username[20]; char password[20];
							*
							*/
							break;

						case 2:;
							char database[20];
							char *newDatabase = strtok(NULL, "\0");
							strcpy(database, newDatabase);
							printf("CREATE DATABASE:\n");
							printf("database: %s\n", database);
							/*here goes how to create database. use variable char database[20]; to get the database's name
							*
							*/
							break;
							
						case 3:;
							char table[20];
							char *newTable = strtok(NULL, " ");
							char *columns = strtok(NULL, ";");
							char *newColumns = strtok(columns+1, ")");
							char *columnName[100];
							char *columnType[100];
							char *temp = strtok(newColumns, " ");
							strcpy(table, newTable);
							printf("CREATE TABLE");
							printf(": %s\n", table);
							int i=0;
							do {
								columnName[i] = temp;
								columnType[i] = strtok(NULL, ", ");
								temp = strtok(NULL, " ");
								if (columnName[i]==NULL || columnType[i]==NULL) {
									printf("error\n");
									break;
								}
								i++;
							} while(temp);
							for (int j=0; j<i; j++) {
								printf("kolom: %s\ntipe: %s\n\n", columnName[j], columnType[j]);
							}
							/*here goes how to create table. var: char table[20]; char *columnName[100]; char *columnType[100];
							*
							*/
							break;

						default:
							printf("CREATING error\n");
            }
        } else if (isInsert(command)) { //insert values
			char *into = strtok(NULL, " ");
			if (steqInto(into)) {
				char table[20];
				char *value[100];
				char *getTable = strtok(NULL, " ");
				char *rawValues = strtok(NULL, ";");
				char *values = strtok(rawValues+1, ")");
				char *temp = strtok(values, ",");
				strcpy(table, getTable);
				int i = 0;
				do {
					value[i++] = temp;
					temp = strtok(NULL, ", ");
				} while (temp);
				printf("INSERT to '%s' VALUES\n", table);
				for (int j=0; j<i; j++) {
					printf("%s\n", value[j]);
				}
				/*here goes how to insert to table. var: char table[20], char *value[100];
				*
				*/
			} else printf("INSERTING error\n");
		} else if (isUpdate(command)) { //update kolom
			char table[20];
			char column[100];
			char newColumn[100];
			char newValue[100];
			char *getTable = strtok(NULL, " ");
			char *opt = strtok(NULL, " ");
			char *getColumn = strtok(NULL, "=");
			char *getNewColumn = strtok(NULL, ";");
			char *getNewValue;
			strcpy(table, getTable);
			strcpy(column, getColumn);
			if (steqSet(opt)) { //update kolom. opt = set
				if (getNewColumn[0] == '\'') {
					getNewColumn = strtok(getNewColumn+1, "\'");
					strcpy(newColumn, getNewColumn);
				} else {
					strcpy(newColumn, getNewColumn);
				}
				/*here goes how to update column. var: char table[20], char column[100]; char newColumn[100];
				*
				*/
			} else if (steqWhere(opt)) { //update value. opt = where
				if (getNewColumn[0] == '\'') {
					getNewValue = strtok(getNewColumn+1, "\'");
					strcpy(newValue, getNewValue);
				} else {
					strcpy(newValue, getNewValue);
				}
				/*here goes how to update value. var: char table[20], char column[100]; char newValue[100];
				*
				*/
			} else printf("UPDATING error\n");
			printf("UPDATE %s.%s to %s\n", table, column, newColumn);
			
		} else if (isDelete(command)) {
			char *from = strtok(NULL, " ");
			char *getTable = strtok(NULL, "\0");
			char table[20];
			strcpy(table, getTable);
			if (steqFrom(from)) {
				printf("clear data from %s\n", table);
			} else printf("DELETING error\n");
			/*here goes how to delete data table. var: char table[20];
			*
			*/
		} else if (isSelect(command)) {
			char *temp = strtok(NULL, ", ");
			char *columns[100];
			int i = 0;
			do {
				columns[i++] = temp;
				if (strcmp(temp, "*") == 0) break;
				temp = strtok(NULL, ", ");
			} while(!steqFrom(temp));
			char *getTable = strtok(NULL, ";");
			char table[20];
			strcpy(table, getTable);
			printf("select: %s\n", table);
			for (int j=0; j<i; j++) {
				printf("%s ", columns[j]);
			}printf("\n");
			/*here goes how to select data from table. var: char table[20]; char *columns[100];
			*
			*/
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

void toLower (char *str) {    
    for(int i = 0; str[i]; i++){
        str[i] = tolower(str[i]);
    }
    return;
}

bool steqWhere (char *where) {
	toLower(where);
	if (strcmp(where, "where") == 0) {
		return true;
	}
	return false;
}

// -- SELECT utilities --

bool isSelect (char *command) {
	toLower(command);
	if (strcmp(command, "select") == 0) {
		return true;
	}
	return false;
}

// -- DELETE utilities --

bool isDelete (char *command) {
	toLower(command);
	if (strcmp(command, "delete") == 0) {
		return true;
	}
	return false;
}

bool steqFrom (char *from) {
	toLower(from);
	if (strcmp(from, "from") == 0) {
		return true;
	}
	return false;
}

// -- UPDATE utilities --

bool isUpdate (char *command) {
	toLower(command);
	if (strcmp(command, "update") == 0) {
		return true;
	}
	return false;
}

bool steqSet (char *set) {
	toLower(set);
	if (strcmp(set, "set") == 0) {
		return true;
	}
	return false;
}

// -- INSERT utilities --

bool isInsert (char *command) {
	toLower(command);
	if (strcmp(command, "insert") == 0) {
		return true;
	}
	return false;
}

bool steqInto (char *into) {
	toLower(into);
	if (strcmp(into, "into") == 0) {
		return true;
	}
	return false;
}

// -- CREATE utilities --


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