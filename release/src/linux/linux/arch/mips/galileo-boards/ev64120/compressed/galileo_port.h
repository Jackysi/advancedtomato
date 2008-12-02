#define GFP_KERNEL 0
#define GFP_ATOMIC 1
#define KERN_ERR ""

void *kmalloc(unsigned int, int);
void *memset(void *, char, unsigned int);
int memcmp(char *, char *, unsigned int);
void *memcpy(void *to, const void *from, unsigned int);
