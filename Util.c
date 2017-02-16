

#include <memory.h>
#include <arpa/inet.h>
#include "Util.h"

char *readInt(char *buf, int *val)
{
    memcpy(val, buf, sizeof(int));
    *val = ntohl(*val);
    return buf + sizeof(int);
}

char *readBytes(char *buf, char *out, int len)
{
    memcpy(out, buf, len);
    return buf + len;
}
