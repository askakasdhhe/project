#ifndef HANDLE_MESSAGE_H
#define HANDLE_MESSAGE_H

#include <string.h>
#include <stdio.h>
#include "cJSON.h"
#include "epoll_set.h"

void handle_message(ClientSession *session);

#endif
