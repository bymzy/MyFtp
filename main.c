

#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <assert.h>
#include <poll.h>
#include <stdlib.h>

#define PORT 3333
#define SERVERIP "127.0.0.1"
#define BACKLOG 10
#define MAXUSER 1024
#define POLLTIMEOUT 5

struct pollfd *fds = NULL;

int InitListen(char *ip, short port, int *listenFd)
{
    int fd = -1;
    fd = socket(AF_INET, SOCK_STREAM, 0);
    assert(fd > 0);
    *listenFd = fd;

    struct sockaddr_in serverAddr;
    serverAddr.sin_addr.s_addr = inet_addr(ip);
    serverAddr.sin_port = htons(port);
    serverAddr.sin_family = AF_INET;

    assert(bind(fd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == 0);
    assert(listen(fd, 10) == 0);

    return 0;
}

void RandomInit(struct pollfd *fds, int total)
{
    int i = 0;
    for (; i < total; ++i) {
        fds[i].fd = -1;
        fds[i].events = 0;
        fds[i].revents = 0;
    }
}

void SetEvent(struct pollfd * fds, int index, int fd, short events)
{
    fds[index].fd = fd;
    fds[index].events = events;
    fds[index].revents = 0;
}

int main(int argc, char *argv[])
{
    /* TODO parse args */
    int err = 0;
    int listenFd = -1;
    int currentUser = 0;

    InitListen(SERVERIP, PORT, &listenFd);

    fds = (struct pollfd *)malloc(sizeof(struct pollfd) * (MAXUSER + 1));
    assert(fds != NULL);

    RandomInit(fds, MAXUSER + 1);
    SetEvent(fds, 0, listenFd, POLLIN);

    while(1) {
        err = poll(fds, currentUser + 1, POLLTIMEOUT);
    }
}


