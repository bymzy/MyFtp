

#include <memory.h>
#include <arpa/inet.h>
#include "Util.h"

char *readInt(char *buf, uint32_t  *val)
{
    memcpy(val, buf, sizeof(uint32_t));
    *val = ntohl(*val);
    return buf + sizeof(uint32_t);
}

char *readBytes(char *buf, char *out, uint32_t len)
{
    memcpy(out, buf, len);
    return buf + len;
}

char *writeInt(char *buf, uint32_t value)
{
    uint32_t temp = htonl(value);
    memcpy(buf, &temp, sizeof(uint32_t));
    return buf + sizeof(uint32_t);
}

char *writeBytes(char *buf, char *in, uint32_t len)
{
    memcpy(buf, in, len);
    return buf + len;
}

char *writeString(char *buf, char *in, uint32_t len)
{
    buf = writeInt(buf, len);
    buf = writeBytes(buf, in, len);
}




