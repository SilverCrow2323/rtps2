#include <assert.h>
#include <stddef.h>
#include <string.h>
#include "nexus_sha256.h"

static void expect_hex(const unsigned char digest[32], const char *hex)
{
    static const char digits[]="0123456789abcdef";
    char actual[65];
    unsigned int i;
    for (i=0; i<32U; ++i) {
        actual[i*2U]=digits[digest[i]>>4];
        actual[i*2U+1U]=digits[digest[i]&15U];
    }
    actual[64]='\\0';
    assert(strcmp(actual,hex)==0);
}

int main(void)
{
    unsigned char digest[32];
    static const char empty_hash[]="e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855";
    static const char abc_hash[]="ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad";
    static const char long_hash[]="248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1";

    assert(nexus_sha256(0,0,digest)==0);
    expect_hex(digest,empty_hash);
    assert(nexus_sha256("abc",3,digest)==0);
    expect_hex(digest,abc_hash);
    assert(nexus_sha256("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq",56,digest)==0);
    expect_hex(digest,long_hash);
    assert(nexus_sha256(0,1,digest)==-1);
    assert(nexus_sha256("abc",3,0)==-1);
    return 0;
}
