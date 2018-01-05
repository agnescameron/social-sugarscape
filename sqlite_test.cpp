#include <stdio.h>
#include <string>
#include <sqlite3.h>
#include <stdio.h>
#include <iostream> 


using namespace std; 

static int callback(void *data, int argc, char **argv, char **azColName){
   int i;
   fprintf(stderr, "%s: ", (const char*)data);
   
   for(i = 0; i<argc; i++){
      printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
   }
   
   printf("\n");
   return 0;
}

int get(int row) {
   sqlite3 *db;
   char *zErrMsg = 0;
   int rc;
   char *sql;
   const char* data = "Callback function called";

   rc = sqlite3_open("../ishaan/feels.sqlite", &db);

   if( rc ) {
      fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
      return(0);
   } else {
      fprintf(stderr, "Opened database successfully\n");
   }

   sprintf(sql, "SELECT * from tweets LIMIT 1 OFFSET %d", row);

   cout << "prints to char" << endl;
   /* Execute SQL statement */
   rc = sqlite3_exec(db, sql, callback, (void*)data, &zErrMsg);

   cout << "got here" << endl;
   
   if( rc != SQLITE_OK ) {
      fprintf(stderr, "SQL error: %s\n", zErrMsg);
      sqlite3_free(zErrMsg);
   } else {
      fprintf(stdout, "Operation done successfully\n");
   }

   sqlite3_close(db);
}


int main(int argc, char* argv[]){
   get(50);

cout << "in main " << endl;
   return 0;
}