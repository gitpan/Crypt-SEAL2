/* SEAL2 */

#include <stdio.h>

#define ALG_OK 0
#define WORDS_PER_SEAL2_CALL 1024
         
typedef struct seal_ctx {
    unsigned long t[520]; /* 512 rounded up to a multiple of 5 + 5 */
    unsigned long s[265]; /* 256 rounded up to a multiple of 5 + 5 */
    unsigned long r[20]; /* 16 rounded up to multiple of 5 */
    unsigned long counter; /* 32-bit synch value. */
    unsigned long ks_buf[WORDS_PER_SEAL2_CALL];
    int ks_pos;
} seal_ctx;
         
#define ROT2(x) (((x) >>2) | ((x) << 30))
#define ROT8(x) (((x) >>8) | ((x) << 24))
#define ROT9(x) (((x) >>9) | ((x) << 23))
#define ROT16(x) (((x) >> 16) | ((x) << 16))
#define ROT24(x) (((x) >> 24) | ((x) << 8))
#define ROT27(x) (((x) >> 27) | ((x) << 5))
         
#define WORD(cp) ((cp[0]<<24)|(cp[1]<<16)|(cp[2]<<8)|(cp[3]))
         
#define F1(x, y, z) (((x) & (y)) | ((~(x)) & (z)))
#define F2(x, y, z) ((x) ^ (y) ^ (z)) 
#define F3(x, y, z) (((x) & (y)) | ((x) & (z)) | ((y) & (z)))
#define F4(x, y, z) ((x) ^ (y) ^ (z))
         
int g(unsigned char *in, int i, unsigned long *h)
{
    unsigned long h0, h1, h2, h3, h4, a, b, c, d, e, temp, w[80];
    unsigned char *kp;

    kp = in;
    h0 = WORD(kp); kp += 4;
    h1 = WORD(kp); kp += 4;
    h2 = WORD(kp); kp += 4;
    h3 = WORD(kp); kp += 4;
    h4 = WORD(kp); kp += 4;
         
    w[0] = i;
    for (i=1;i<16;i++) w[i] = 0;

    for (i=16;i<80;i++) w[i] = w[i-3]^w[i-8]^w[i-14]^w[i-16];

    a = h0; b = h1; c = h2; d = h3; e = h4;

    for (i = 0; i < 20; i++) {
        temp = ROT27(a) + F1(b, c, d) + e + w[i] + 0x5a827999;
        e = d; d = c; c = ROT2(b); b = a; a = temp;
    }

    for (i = 20; i < 40; i++) {
        temp = ROT27(a) + F2(b, c, d) + e + w[i] + 0x6ed9eba1;
        e = d; d = c; c = ROT2(b); b = a; a = temp;
    }

    for (i = 40; i < 60; i++) {
        temp = ROT27(a) + F3(b, c, d) + e + w[i] + 0x8f1bbcdc;
        e = d; d = c; c = ROT2(b); b = a; a = temp;
    }

    for (i = 60; i < 80; i++) {
        temp = ROT27(a) + F4(b, c, d) + e + w[i] + 0xca62c1d6;
        e = d; d = c; c = ROT2(b); b = a; a = temp;
    } 

    h[0] = h0+a; h[1] = h1+b; h[2] = h2+c; h[3] = h3+d; h[4] = h4+e;

    return (ALG_OK);
}         

int seal_init( seal_ctx *result, unsigned char *key )
{         
    int i;
    unsigned long h[5];
         
    for (i = 0; i < 510; i += 5)
        g(key, i/5, &(result->t[i]));

    /* horrible special case for the end */
    g(key, 510/5, h);

    for (i = 510; i < 512; i++)
        result->t[i] = h[i-510];

    /* 0x1000 mod 5 is +1, so have horrible special case for the start */
    g(key, (-1+0x1000)/5, h);

    for (i = 0; i < 4; i++)
        result->s[i] = h[i+1];

    for (i = 4; i < 254; i += 5)
        g(key, (i+0x1000)/5, &(result->s[i]));

   /* horrible special case for the end */
    g(key, (254+0x1000)/5, h);
   
    for (i = 254; i < 256; i++)
        result->s[i] = h[i-254];
   
    /* 0x2000 mod 5 is +2, so have horrible special case at the start */
    g(key, (-2+0x2000)/5, h);
   
    for (i = 0; i < 3; i++)
        result->r[i] = h[i+2];
   
    for (i = 3; i < 13; i += 5)
        g(key, (i+0x2000)/5, &(result->r[i]));
   
    /* horrible special case for the end */
    g(key, (13+0x2000)/5, h);
   
    for (i = 13; i < 16; i++)
        result->r[i] = h[i-13];
    
    return (ALG_OK);
}

int seal(seal_ctx *key, unsigned long in, unsigned long *out)
{
    int i, j, l;         
    unsigned long a, b, c, d, n1, n2, n3, n4, *wp;
    unsigned short p, q;

    wp = out;
    for (l = 0; l < 4; l++) {
        a = in ^ key->r[4*l];
        b = ROT8(in) ^ key->r[4*l+1];
        c = ROT16(in) ^ key->r[4*l+2];
        d = ROT24(in) ^ key->r[4*l+3];
    }

    for (j = 0; j < 2; j++) {
        p = a & 0x7fc;
        b += key->t[p/4];
        a = ROT9(a);
         
        p = b & 0x7fc;
        c += key->t[p/4];
        b = ROT9(b);
         
        p = c & 0x7fc;
        d += key->t[p/4];
        c = ROT9(c);
         
        p = d & 0x7fc;
        a += key->t[p/4];
        d = ROT9(d);
         
        n1 = d; n2 = b; n3 = a; n4 = c;
         
        p = a & 0x7fc;
        b += key->t[p/4];
        a = ROT9(a);
         
        p = b & 0x7fc;
        c += key->t[p/4];
        b = ROT9(b);
         
        p = c & 0x7fc;
        d += key->t[p/4];
        c = ROT9(c);
         
        p = d & 0x7fc;
        a += key->t[p/4];
        d = ROT9(d);
         
        /* This generates 64 32-bit words, or 256 bytes of keystream. */
        for (i = 0; i < 64; i++) {
            p = a & 0x7fc;
	    b += key->t[p/4];
            a = ROT9(a);
            b ^= a;
         
            q = b & 0x7fc;
            c ^= key->t[q/4];
            b = ROT9(b);
            c += b;
         
            p = (p+c) & 0x7fc;
            d += key->t[p/4];
            c = ROT9(c);
            d ^= c;
         
            q = (q+d) & 0x7fc;
            a ^= key->t[q/4];
            d = ROT9(d);
            a += d;
         
            p = (p+a) & 0x7fc;
            b ^= key->t[p/4];
            a = ROT9(a);
         
            q = (q+b) & 0x7fc;
            c += key->t[q/4];
            b = ROT9(b);
         
            p = (p+c) & 0x7fc;
            d ^= key->t[p/4];
            c = ROT9(c);
         
            q = (q+d) & 0x7fc;
            a += key->t[q/4];
            d = ROT9(d);

            *wp = b + key->s[4*i]; wp++;
            *wp = c ^ key->s[4*i+1]; wp++;
            *wp = d + key->s[4*i+2]; wp++;
            *wp = a ^ key->s[4*i+3]; wp++;

            if (i & 1) {
                a += n3;
                c += n4;
            } else {
                a += n1;
                c += n2;
            }
        }
    }    

    return (ALG_OK);
}
         

/* Added call to refill ks_buf and reset counter and ks_pos. */
void seal_refill_buffer(seal_ctx *c)
{
    seal(c,c->counter,c->ks_buf);
    c->counter++;
    c->ks_pos = 0;
}

void seal_key(seal_ctx *c, unsigned char *key)
{
    seal_init(c, key);
    c->counter = 0; /* By default, init to zero. */
    c->ks_pos = WORDS_PER_SEAL2_CALL;
    /* Refill keystream buffer on next call. */
}
         
/* This encrypts the next w words with SEAL2. */
void seal_encrypt(seal_ctx *c, unsigned char *plaintext, int w,
    unsigned char *ciphertext)
{
    int i;

    for (i = 0; i < w; i++) {
        if(c->ks_pos >= WORDS_PER_SEAL2_CALL) seal_refill_buffer(c);
        ciphertext[i] = plaintext[i] ^ c->ks_buf[c->ks_pos];
        c->ks_pos++;
    }
}
         
void seal_decrypt(seal_ctx *c, unsigned char *data_ptr, int w,
    unsigned char *data2_ptr)
{
    seal_encrypt(c, data_ptr, w, data2_ptr);
}

void seal_resynch(seal_ctx *c, unsigned long synch_word)
{
    c->counter = synch_word;
    c->ks_pos = WORDS_PER_SEAL2_CALL;
}

/* added by jcd */
void seal_repos(seal_ctx *c, unsigned long pos)
{
    c->ks_pos = pos;
}

int main(void)
{
    seal_ctx sc;
    unsigned int COUNT = 4096;
    unsigned char plain1[COUNT], plain2[COUNT], ciphertext[COUNT];
    int i, flag;
    int j;
    unsigned long temp, temp1;

    unsigned char key[20] = {
        0x67, 0x45, 0x23, 0x01, 0xef, 0xcd, 0xab, 0x89, 0x98, 0xba,
        0xdc, 0xfe, 0x10, 0x32, 0x54, 0x76, 0xc3, 0xd2, 0xe1, 0xf0
    };

    printf("1 Key Setup\n");
    seal_key(&sc, key);
         
    printf("2 Data Setup\n");
    for (i = 0; i < COUNT; i++) plain1[i] = 0;
    for (i = 0; i < COUNT; i++) plain2[i] = 8;
    for (i = 0; i < COUNT; i++) ciphertext[i] = 2*i;

    printf("3 Encryption\n");
    seal_encrypt(&sc, plain1, COUNT, ciphertext);

    temp1 = temp = 0;
    for (i = 0; i < COUNT; i++) {
        for (j = 0; j < 4; j++) {
            temp = temp << 8;
            temp |= ciphertext[i];
        }

        temp1 ^= temp;
        temp = 0;
    }


    printf("4 Decryption\n");
    seal_key(&sc, key);
    seal_decrypt(&sc, ciphertext, COUNT, plain2);

    flag = 0;
    for (i = 0; i < COUNT; i++)
        if (plain2[i] != plain1[i]) {
            flag = 1;
            break;
        }

    if (flag) printf("Decryption failed\n");
        else printf("Decryption succeeded\n");

    return 0;
}

