

#define _GNU_SOURCE 1
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <assert.h>
#include <poll.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <memory.h>
#include "Job.h"

#define PORT 3333
#define SERVERIP "127.0.0.1"
#define BACKLOG 10
#define MAXUSER 1024
#define POLLTIMEOUT 5

struct pollfd *fds = NULL;

int ReadNBytes(int fd, char *buf, int len)
{
    int toRead = 4;
    int readed = 0;
    int ret = 0;
    int err = 0;

    /* get msg len */
    while (toRead > 0) {
        ret = recv(fd, buf + readed, toRead, 0);
        if (ret == 0) {
            /* need close socket */
            err = EINVAL;
            break;
        } else if (ret < 0) {
            err = errno;
            if (err == EAGAIN || err == EINPROGRESS) {
                err = 0;
                continue;
            }
            break;
        } else {
            readed += ret;
            toRead -= ret;
        }
    }

    return err;
}

int InitListen(char *ip, short port, int *listenFd)
{
    int fd = -1;
    int reuse = 1;
    fd = socket(AF_INET, SOCK_STREAM, 0);
    assert(fd > 0);
    *listenFd = fd;

    struct sockaddr_in serverAddr;
    serverAddr.sin_addr.s_addr = inet_addr(ip);
    serverAddr.sin_port = htons(port);
    serverAddr.sin_family = AF_INET;

    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
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

void SetNonBlock(int fd)
{
    int val = fcntl(fd, F_GETFL);

    val |= O_NONBLOCK;
   
    assert(fcntl(fd, F_SETFL, val) == 0);
}

void SetEvent(struct pollfd * fds, int index, int fd, short events)
{
    fds[index].fd = fd;
    fds[index].events = events;
    fds[index].revents = 0;
}

int HandleRead(int fd)
{
    int err = 0;

    char len[4];
    err = ReadNBytes(fd, len, 4);
    if (err != 0) {
        return err;
    }

    int msglen = 0;
    memcpy(&msglen, len, 4);
    msglen = ntohl(msglen);

    char * data = (char *)malloc(msglen);
    err = ReadNBytes(fd, data, msglen);
    if (err != 0) {
        free(data);
    } else {
        DoJob(data);
    }

    return err;
}

int main(int argc, char *argv[])
{
    /* TODO parse args */
    int err = 0;
    int ret = 0;
    int listenFd = -1;
    int currentUser = 0;

    InitListen(SERVERIP, PORT, &listenFd);

    fds = (struct pollfd *)malloc(sizeof(struct pollfd) * (MAXUSER + 1));
    assert(fds != NULL);

    RandomInit(fds, MAXUSER + 1);
    SetEvent(fds, 0, listenFd, POLLIN);

    while(1) {
        ret = 0;
        ret = poll(fds, currentUser + 1, POLLTIMEOUT);
        if (ret < 0) {
            /* poll error */
            err = errno;
            perror("poll failed!");
            break;
        } else if (ret == 0) {
            /* timeout */
            printf("poll timeout!");
            continue;
        } else {
            /* fds ready */
            int i = 0;
            int max = ret;
            for (i; i < currentUser + 1; ++i) {
                if (fds[i].fd == listenFd && fds[i].revents & POLLIN) {
                    struct sockaddr_in addr;
                    int len = 0;
                    int client = accept(listenFd, (struct sockaddr*)&addr, &len);
                    if (client < 0) {
                        perror("accpet client falied!");
                        continue;
                    }

                    SetNonBlock(client);
                    fds[client].fd = client;
                    fds[client].events = POLLIN | POLLRDHUP;
                    fds[client].revents = 0;

                } else if (fds[i].revents & POLLRDHUP) {
                    close(fds[i].fd);
                    fds[i].fd = fds[currentUser].fd;
                    currentUser--;
                } else if (fds[i].revents & POLLIN) {
                    err = HandleRead(fds[i].revents);
                    if (err != 0) {
                        close(fds[i].fd);
                        fds[i].fd = fds[currentUser].fd;
                        currentUser--;
                    }
                }
            }
        }
    }

    /* TODO frees */
}


