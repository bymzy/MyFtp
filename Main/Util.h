

char *readInt(char *buf, uint32_t *val);
char *read64Int(char *buf, uint64_t *val);
char *readBytes(char *buf, char *out, uint32_t len);
char *readString(char *buf, char **out);

char *writeInt(char *buf, uint32_t value);
char *write64Int(char *buf, uint64_t value);
char *writeBytes(char *buf, char *in, uint32_t len);
char *writeString(char *buf, char *in, uint32_t len);

uint64_t ntohll(const uint64_t input);
uint64_t htonll(const uint64_t input);


