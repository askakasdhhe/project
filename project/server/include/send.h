#ifndef SEND_H
#define SEND_H

#include <sys/socket.h>
#include <string.h>
#include <stdlib.h>
#include "cJSON.h"

void send_json_response(int fd, const char *status, const char *msg);
void send_json_device_data(int fd, const char *device_id, const char *time,
                           double temp, double humi);
void send_json_end(int fd, int page, int page_size);

#endif
