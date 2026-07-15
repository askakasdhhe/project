#ifndef DB_H
#define DB_H

#include <sqlite3.h>
#include <stdio.h>
#include <string.h>

int init_db(void);
int register_device(const char *id, const char *pwd);
int auth_device(const char *id, const char *pwd);
void save_check(const char *id, float temp, float humi);
void history(int fd, int page, int page_size);

extern sqlite3 *db;

#endif
