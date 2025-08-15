#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>

#include "aes.h"

#define BUF_SIZE 32 + 1 /* includes null terminator */

/* the key is obfuscated and generated based on fixed input but for simplicity
   sake statically defined here */
#define KEY_STRING "S@gi0PneW^#*T1Zb"

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <uid>\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *uid = argv[1];

    size_t len = strlen(uid);

    if (len != 16) {
        fputs("Error: invalid uid length\n", stderr);
        return EXIT_FAILURE;
    }

    for (size_t i = 0; i < len; i++) {
        if (!isxdigit(uid[i])) {
            fputs("Error: uid contains non-hex char\n", stderr);
            return EXIT_FAILURE;
        }
    }

    uint64_t val = strtoull(uid, NULL, 16);

    /* regard ffffffffffffffff and 0000000000000000 entries as invalid
       if errno is ERANGE, ULLONG_MAX is returned 2 birds 1 stone */
    if (val == ULLONG_MAX || val == 0) {
        fputs("Error: uid is nil or too large\n", stderr);
        return EXIT_FAILURE;
    }

    /* take the 40 msb of the uid and shift right by 2 ie. >> 24 + 2
       
       eg.

       e6632c25a344b330 >> 18 (24 dec)
       e6632c25a3 >> 2
       3998cb0968 & ffffffff (taken care by the cast to uint32_t)
       98cb0968 */
    val >>= 26;

    char buf[BUF_SIZE] = {0};

    snprintf(buf, 9, "%08x", (uint32_t)val); /* lowercase */

    /* create a 32-char repeated string from the sliced and shifted uid
       hexadecimal representation

       eg.

       98cb0968
       98cb096898cb0968
       98cb096898cb096898cb096898cb0968 */
    for (int i = 1; i < 3; i++) {
        strncat(buf, buf, i << 3);
    }

    buf[0]  = '@';
    buf[3]  = '#';
    buf[7]  = 'x';
    buf[9]  = 'N';
    buf[11] = '?';
    buf[15] = 'a';
    buf[17] = '*';
    buf[20] = ')';
    buf[24] = 't';
    buf[26] = 'C';
    buf[29] = '?';
    buf[30] = 'P';

    const char key[] = KEY_STRING;

    struct AES_ctx ctx = {0};

    AES_init_ctx(&ctx, (const uint8_t *)key);

    for (size_t i = 0; i < (sizeof(buf) >> 4); i++) {
        /* swtg adds a coefficient prior to MixColumns and after the last
           round in the Cipher function. note: swtg firmware uses a flag */
        AES_ECB_encrypt(&ctx, (uint8_t *)&buf[i << 4]);
    }

    for (size_t i = 0; i < (sizeof(buf) & ~0xf); i++) {
        /* output whitening */
        printf("%02hhx", (uint8_t)buf[i] ^ (uint8_t)(i % 16));
    }

    puts("");

    return EXIT_SUCCESS;
}
