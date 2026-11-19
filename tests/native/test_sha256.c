#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "nexus_sha256.h"

static void assert_digest(const void *data, size_t length, const char *expected)
{
    unsigned char digest[32];
    char actual[65];
    unsigned int i;

    assert(nexus_sha256(data, length, digest) == 0);
    for (i = 0; i < 32U; ++i)
        (void)snprintf(actual + (i * 2U), 3U, "%02x", (unsigned int)digest[i]);
    actual[64] = '\0';
    assert(strcmp(actual, expected) == 0);
}

int main(void)
{
    static const unsigned char zeros[131072] = {0};
    static unsigned char million_a[1000000];
    memset(million_a, 'a', sizeof(million_a));

    assert_digest(0, 0, "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
    assert_digest("abc", 3, "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
    assert_digest(zeros, 55, "02779466cdec163811d078815c633f21901413081449002f24aa3e80f0b88ef7");
    assert_digest(zeros, 56, "d4817aa5497628e7c77e6b606107042bbba3130888c5f47a375e6179be789fbb");
    assert_digest(zeros, 63, "c7723fa1e0127975e49e62e753db53924c1bd84b8ac1ac08df78d09270f3d971");
    assert_digest(zeros, 64, "f5a5fd42d16a20302798ef6ed309979b43003d2320d9f0e8ea9831a92759fb4b");
    assert_digest(zeros, 65, "98ce42deef51d40269d542f5314bef2c7468d401ad5d85168bfab4c0108f75f7");
    assert_digest(zeros, sizeof(zeros), "fa43239bcee7b97ca62f007cc68487560a39e19f74f3dde7486db3f98df8e471");
    assert_digest(million_a, sizeof(million_a), "cdc76e5c9914fb9281a1c7e284d73e67f1809a48a497200e046d39ccc7112cd0");

    assert(nexus_sha256(0, 1, (unsigned char[32]){0}) == -1);
    assert(nexus_sha256("abc", 3, 0) == -1);
    return 0;
}
