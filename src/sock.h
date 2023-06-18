#ifndef SOCK_H
#define SOCK_H

#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/un.h>

#include <string.h>

#define UNIX_SOCK_PATH "/tmp/test.sock"

struct ThreadArgs {
    int(*cb)(void*);
    void* arg;
    int stop;
};

void* listen_for_conn(void*);

#endif
