

#include <memory.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include "Util.h"

char *readInt(char *buf, uint32_t  *val)
{
    memcpy(val, buf, sizeof(uint32_t));
    *val = ntohl(*val);
    return buf + sizeof(uint32_t);
}

char *read64Int(char *buf, uint64_t *val)
{
    memcpy(val, buf, sizeof(uint64_t));
    *val = ntohll(*val);
    return buf + sizeof(uint64_t);
}

char *readBytes(char *buf, char *out, uint32_t len)
{
    memcpy(out, buf, len);
    return buf + len;
}

char *readString(char *buf, char **out)
{
    uint32_t len = 0;
    buf = readInt(buf, &len);

    char *temp = malloc(len + 1);
    bzero(temp, len + 1);
    buf = readBytes(buf, temp, len);
    *out = temp;

    return buf;
}

char *writeInt(char *buf, uint32_t value)
{
    uint32_t temp = htonl(value);
    memcpy(buf, &temp, sizeof(uint32_t));
    return buf + sizeof(uint32_t);
}

char *write64Int(char *buf, uint64_t value)
{
    uint64_t temp = htonll(value);
    memcpy(buf, &temp, sizeof(uint64_t));
    return buf + sizeof(uint64_t);
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
    return buf;
}

uint64_t ntohll(const uint64_t input)
{
    uint64_t rval;
    uint8_t *data = (uint8_t *)&rval;

    data[0] = input>>56;
    data[1] = input>>48;
    data[2] = input>>40;
    data[3] = input>>32;
    data[4] = input>>24;
    data[5] = input>>16;
    data[6] = input>>8;
    data[7] = input>>0;

    return rval;
}

uint64_t htonll(const uint64_t input)
{
    return (ntohll(input));
}

