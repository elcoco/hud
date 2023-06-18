
#include "sock.h"


void* listen_for_conn(void* args)
{
    int sockfd;
    struct sockaddr_un name;
    struct ThreadArgs *ta = args;

    unlink(UNIX_SOCK_PATH);

    memset(&name, 0, sizeof(name));
    name.sun_family = AF_UNIX;
    strncpy(name.sun_path, UNIX_SOCK_PATH, sizeof(name.sun_path) - 1);


    if ((sockfd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
        fprintf(stderr, "ERROR opening socket\n");
        return NULL;
    }

    if (bind(sockfd, (const struct sockaddr *) &name, sizeof(name)) < 0) {
        fprintf(stderr, "ERROR binding: %s\n", UNIX_SOCK_PATH);
        return NULL;
    }


    if (listen(sockfd, 1) < 0) {
        fprintf(stderr, "ERROR listen %s\n", UNIX_SOCK_PATH);
        return NULL;
    }

    printf("Listening on socket: %s\n", UNIX_SOCK_PATH);

    int data_sock;

    while (!ta->stop) {
        data_sock = accept(sockfd, NULL, NULL);
        printf("Accepted connection\n");
        ta->cb(ta->arg);
        close(data_sock);
    }
    printf("Stopping thread\n");
    return NULL;
}
