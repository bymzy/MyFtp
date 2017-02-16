

#include <assert.h>
#include <string.h>
#include "Job.h"
#include "Util.h"

void DoJob(char *data)
{
    char *index = data;

    /* parse msg type*/
    int typelen;
    index = readInt(data, &typelen);

    assert(typelen < 20);
    char reqType[20];
    index = readBytes(reqType, typelen);

    if (strcmp(reqType, "List") == 0) {

    } else {
        assert(0);
    }
}


