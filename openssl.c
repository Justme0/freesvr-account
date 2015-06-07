#include "openssl.h"
#include "global.h"
#include <iconv.h>

int code_convert(char *from_charset, char *to_charset, char *inbuf, size_t inlen, char *outbuf, size_t outlen)
{
	iconv_t cd;
	char **pin = &inbuf;
	char **pout = &outbuf;

	cd = iconv_open(to_charset, from_charset);

	if (cd == 0)
		return -1;

	if (iconv(cd, pin, (size_t *) & inlen, pout, (size_t *) & outlen) == (size_t)(-1)) {
		perror("iconv");
		return -1;
	}

	iconv_close(cd);

	return 0;
}

char * g2u(char *inbuf)
{
	size_t outlen = BUFSIZE, inlen = strlen(inbuf);
	static char outbuf[BUFSIZE];

	memset(outbuf, 0, sizeof(outbuf));
	if (code_convert("GB2312", "UTF-8", inbuf, inlen, outbuf, outlen) == 0)
		return outbuf;
	else
		return NULL;
}

void print_hex(unsigned char *buf, int len)
{
	// avoid warning unused
	(void)buf;
	(void)len;
}

int initial_key_ivec(DES_cblock key, DES_cblock ivec)
{
	SHA_CTX sha1;
	/* Output of SHA1 algorithm, must be 20 bytes */
	unsigned char sha1_hash[20];

	/* Initial SHA1 */
	SHA1_Init(&sha1);
	/* SHA1 encrypt */
	SHA1_Update(&sha1, KEY, strlen(KEY));
	/* Clean up */
	SHA1_Final(sha1_hash, &sha1);

	/* Print the cipher text */
	print_hex(sha1_hash, 20);

	memcpy(key, &sha1_hash[0], 8);
	memcpy(ivec, &sha1_hash[12], 8);

	return 0;
}

int decrypt(const char *cipher, char *plain, int cipher_len, int *plain_len)
{

	DES_key_schedule schedule;
	DES_cblock key, ivec;
	int plen;
	unsigned char input[BUFSIZE], output[BUFSIZE];

	memset(input, 0x00, BUFSIZE);
	memset(output, 0x00, BUFSIZE);
	memset(&schedule, 0x00, sizeof(DES_key_schedule));
	memset(key, 0x00, sizeof(DES_cblock));
	memset(ivec, 0x00, sizeof(DES_cblock));

	initial_key_ivec(key, ivec);
	DES_set_key_unchecked(&key, &schedule);

	memcpy((char *) input, cipher, cipher_len);
	print_hex(input, cipher_len);
	DES_ncbc_encrypt(input, output, cipher_len, &schedule, &ivec, 0);
	plen = strlen((const char *) output);
	print_hex(output, plen);

	if (plain != NULL)
	{

		memcpy(plain, (const char *) output, plen);
	}

	if (plain_len != NULL)
	{

		*plain_len = plen;
	}

	return 0;
}

int encrypt(const char *plain, char *cipher, int plain_len, int *cipher_len)
{

	DES_key_schedule schedule;
	DES_cblock key, ivec;
	int clen = 0;
	unsigned char input[BUFSIZE], output[BUFSIZE];

	memset(input, 0x00, BUFSIZE);
	memset(output, 0x00, BUFSIZE);
	memset(&schedule, 0x00, sizeof(DES_key_schedule));
	memset(key, 0x00, sizeof(DES_cblock));
	memset(ivec, 0x00, sizeof(DES_cblock));

	initial_key_ivec(key, ivec);
	DES_set_key_unchecked(&key, &schedule);

	memcpy((char *) input, plain, plain_len);
	print_hex(input, plain_len);
	DES_ncbc_encrypt(input, output, plain_len, &schedule, &ivec, 1);
	clen = (plain_len + 7) / 8 * 8;
	print_hex(output, clen);

	if (cipher != NULL)
	{

		memcpy(cipher, (const char *) output, clen);
	}

	if (cipher_len != NULL)
	{

		*cipher_len = clen;
	}

	return 0;
}

//int main()
//{
//
//	return 0;
//}
