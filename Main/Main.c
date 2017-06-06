

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
#include <unistd.h>
#include <getopt.h>
#include <sys/stat.h>

#include "Job.h"
#include "ThreadGroup.h"
#include "../FileManager/FileManager.h"

static short PORT = 3333;
static char *SERVERIP = "127.0.0.1";
static int BACKLOG = 10;
static uint32_t MAXUSER = 1024;
static int POLLTIMEOUT = 100;
static char *LOGFILE = "/var/log/myftp";

struct pollfd *fds = NULL;

int ReadNBytes(int fd, char *buf, int len)
{
    int toRead = len;
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
            perror("recv failed!");
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
    assert(listen(fd, BACKLOG) == 0);

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

    char * data = (char *)malloc(msglen + 1);
    bzero(data, msglen + 1);
    err = ReadNBytes(fd, data, msglen);
    if (err != 0) {
        free(data);
    } else {
        ParseJob(fd, data);
    }

    return err;
}

void Usage()
{
    char *s = " Usage: MyFtp [options] \n\
               --work_dir   -w  server working dir \n\
               --ip         -i  server listen ip \n\
               --port       -p  server listen port \n\
               --log_file   -l  server log file \n\
               \n\
               DESC: \n\
               Simple ftp program made by mzy !\n\
               ";

    printf("%s\n", s);
}

int main(int argc, char *argv[])
{
    /* TODO parse args */
    int err = 0;
    int ret = 0;
    int listenFd = -1;
    int currentUser = 0;
    int c = -1;
    int fd = -1;

    /* parse cmd line */
    struct option long_options[] = {
        {"work_dir", required_argument, 0, 0},
        {"ip", required_argument, 0, 0},
        {"port", optional_argument, 0, 0},
        {"log_file", optional_argument, 0, 0},
        {0,0,0,0}
    };

    while (1) {
        c = getopt_long(argc, argv, "w:i:p:l:", long_options, NULL);
        if (-1 == c) {
            break;
        }

        switch (c) {
            case 'w':
                g_repo = optarg;
                break;
            case 'i':
                SERVERIP = optarg;
                break;
            case 'p':
                PORT = atoi(optarg);
                break;
            case 'l':
                LOGFILE = optarg;
                break;
            default:
                err = EINVAL;
                break;
        };
        
        if (err != 0) {
            break;
        }
    }

    if (err != 0) {
        Usage();
        return err;
    }

    /* check dir exists */
    struct stat st;
    err = stat(g_repo, &st);
    if (err != 0) {
        printf("invalid repo dir %s \n", g_repo);
        err = EINVAL;
        return err;
    }
    
    if (PORT <= 0 || PORT >= 65536) {
        printf("invalid port %d \n", PORT);
        err = EINVAL;
        return err;
    }

    struct in_addr addrt;
    err = inet_aton(SERVERIP, &addrt);
    if (err == 0) {
        printf("invalid ip %s \n", SERVERIP);
        return err;
    }

    /* try open log file */
    fd = open(LOGFILE, O_RDWR | O_CREAT | O_APPEND, 0644);
    if (fd < 0) {
        err = errno;
        printf("open log file %s failed, errstr: %s \n", LOGFILE, strerror(err));
        return err;
    }

    /*
    err = daemon(0, 0);
    if (err != 0) {
        err = errno;
        printf("make daemon failed, errstr: %s \n", strerror(err));
        return err;
    }
    */

    close(1);
    close(2);
    dup(fd);
    dup(fd);

    InitFileManager(g_repo);
    InitThreads();
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
            continue;
        } else {
            /* fds ready */
            int i = 0;
            for (; i < currentUser + 1; ++i) {
                if (fds[i].fd == listenFd && fds[i].revents & POLLIN) {
                    struct sockaddr_in addr;
                    socklen_t len = 0;
                    int client = accept(listenFd, (struct sockaddr*)&addr, &len);
                    if (client < 0) {
                        perror("accpet client falied!");
                        continue;
                    }

                    currentUser++;
                    SetNonBlock(client);
                    SetEvent(fds, currentUser, client, POLLIN | POLLRDHUP);
                    printf("accept client %d, current user %d\n", client, currentUser);
                } else if (fds[i].revents & POLLRDHUP) {
                    close(fds[i].fd);
                    fds[i].fd = fds[currentUser].fd;
                    currentUser--;
                } else if (fds[i].revents & POLLIN) {
                    err = HandleRead(fds[i].fd);
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
    free(fds);

    return err;
}


