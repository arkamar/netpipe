
#include "compat-header.h"
#include "aes.h"
#include "sha2.h"
#include "logging.h"
#include "auth.h"




static void _generateSalt(unsigned char Salt[16])
{
	for (size_t i = 0; i < 16; ++i)
		Salt[i] = rand();

	return;
}


static void _increment16(unsigned char Block[16])
{
	unsigned char *b = Block + 16;

	do {
		--b;
		++*b;
	} while (b != Block && *b == 0);

	return;
}


static void _print16(const char *Text, const unsigned char *x)
{
	LogInfo("[AUTH]: %s: %.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x", Text, 
		x[0], x[1], x[2], x[3], x[4], x[5], x[6], x[7],
		x[8], x[9], x[10], x[11], x[12], x[13], x[14], x[15]);

	return;
}


void AuthKeyGen(const char *Password, unsigned char *Salt, unsigned int SaltLen, uint32_t IterationCount, unsigned char Key[16])
{
	sha256_ctx ctx;
	unsigned char digest[256/8];

	sha256_init(&ctx);
	sha256_update(&ctx, Salt, SaltLen);
	sha256_update(&ctx, Password, (unsigned int)strlen(Password));
	for (size_t i = 0; i < IterationCount - 1; ++i) {
		sha256_final(&ctx, digest);
		sha256_init(&ctx);
		sha256_update(&ctx, Salt, SaltLen);
		sha256_update(&ctx, digest, sizeof(digest));
	}

	sha256_final(&ctx, digest);
	memcpy(Key, digest, 0x10);

	return;
}


int AuthSocket(SOCKET Socket, const char *Password)
{
	int ret = 0;
	unsigned char ourChallenge[16];
	unsigned char otherChallenge[16];
	unsigned char salt[16];
	unsigned char key[16];
	unsigned char tmp[16];
	struct timeval timeout;
	AES_Ctx aesCtx;

	timeout.tv_sec = 5;
	timeout.tv_usec = 0;
	ret = setsockopt(Socket, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout));
	if (ret != 0) {
		ret = errno;
		LogError("[AUTH]: Unable to set socket send timeout: %i", ret);
		goto Exit;
	}

	ret = setsockopt(Socket, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(timeout));
	if (ret != 0) {
		ret = errno;
		LogError("[AUTH]: Unable to set socket recv timeout: %i", ret);
		goto Exit;
	}

	_generateSalt(ourChallenge);
	ret = send(Socket, ourChallenge, sizeof(ourChallenge), 0);
	if (ret != sizeof(ourChallenge)) {
		ret = errno;
		LogError("[AUTH]: Failed to sned challenge: %i\n", ret);;
		goto Exit;
	}

	ret = recv(Socket, otherChallenge, sizeof(otherChallenge), MSG_WAITALL);
	if (ret != sizeof(otherChallenge)) {
		ret = errno;
		LogError("[AUTH]: Failed to receive challenge: %i\n", ret);;
		goto Exit;
	}

	for (size_t i = 0; i < sizeof(salt) / sizeof(salt[0]); ++i)
		salt[i] = ourChallenge[i] ^ otherChallenge[i];

	AuthKeyGen(Password, salt, sizeof(salt), 65536, key);
	_print16("our challenge", ourChallenge);
	_print16("other challenge", otherChallenge);
	_print16("salt", salt);
	_print16("key", key);

	_increment16(otherChallenge);
	_print16("other inc", otherChallenge);
	AES_SetupEncrypt(&aesCtx, key, sizeof(key) * 8);
	AES_Encrypt(&aesCtx, otherChallenge, sizeof(otherChallenge), tmp);
	_print16("other encrypted", tmp);
	ret = send(Socket, tmp, sizeof(tmp), 0);
	if (ret != sizeof(tmp)) {
		ret = errno;
		LogError("[AUTH]: Failed to send encrypted info: %i\n", ret);;
		goto Exit;
	}

	ret = recv(Socket, tmp, sizeof(tmp), MSG_WAITALL);
	if (ret != sizeof(tmp)) {
		ret = errno;
		LogError("[AUTH]: Failed to receive encrypted info: %i\n", ret);;
		goto Exit;
	}
	
	_print16("our encrypted", tmp);
	AES_SetupDecrypt(&aesCtx, key, sizeof(key) * 8);
	AES_Decrypt(&aesCtx, tmp, sizeof(tmp), otherChallenge);
	_increment16(ourChallenge);
	_print16("our decrypted", otherChallenge);
	_print16("our inc", ourChallenge);
	ret = memcmp(otherChallenge, ourChallenge, sizeof(otherChallenge));
	if (ret != 0) {
		ret = -1;
		LogError("[AUTH]: Authentication failed: %i\n", ret);;
	}

Exit:
	return ret;
}
