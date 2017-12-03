#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <pthread.h>    
#include <semaphore.h>
#include <sys/ioctl.h>
#include <glib.h>
#include "hash.h"


void handleErrors(void)
{
  ERR_print_errors_fp(stderr);
}

int encrypt(unsigned char *plaintext, int plaintext_len, unsigned char *key,
  unsigned char *iv, unsigned char *ciphertext)
{
  EVP_CIPHER_CTX *ctx;

  int len;

  int ciphertext_len;

  /* Create and initialise the context */
  if(!(ctx = EVP_CIPHER_CTX_new())) handleErrors();

  /* Initialise the encryption operation. IMPORTANT - ensure you use a key
   * and IV size appropriate for your cipher
   * In this example we are using 256 bit AES (i.e. a 256 bit key). The
   * IV size for *most* modes is the same as the block size. For AES this
   * is 128 bits */
  if(1 != EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv))
    handleErrors();

  /* Provide the message to be encrypted, and obtain the encrypted output.
   * EVP_EncryptUpdate can be called multiple times if necessary
   */
  if(1 != EVP_EncryptUpdate(ctx, ciphertext, &len, plaintext, plaintext_len))
    handleErrors();
  ciphertext_len = len;

  /* Finalise the encryption. Further ciphertext bytes may be written at
   * this stage.
   */
  if(1 != EVP_EncryptFinal_ex(ctx, ciphertext + len, &len)) handleErrors();
  ciphertext_len += len;

  /* Clean up */
  EVP_CIPHER_CTX_free(ctx);

  return ciphertext_len;
}


int decrypt(unsigned char *ciphertext, int ciphertext_len, unsigned char *key,
  unsigned char *iv, unsigned char *plaintext)
{
  EVP_CIPHER_CTX *ctx;

  int len;

  int plaintext_len;

  /* Create and initialise the context */
  if(!(ctx = EVP_CIPHER_CTX_new())) handleErrors();

  /* Initialise the decryption operation. IMPORTANT - ensure you use a key
   * and IV size appropriate for your cipher
   * In this example we are using 256 bit AES (i.e. a 256 bit key). The
   * IV size for *most* modes is the same as the block size. For AES this
   * is 128 bits */
  if(1 != EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv))
    handleErrors();

  /* Provide the message to be decrypted, and obtain the plaintext output.
   * EVP_DecryptUpdate can be called multiple times if necessary
   */
  if(1 != EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext, ciphertext_len))
    handleErrors();
  plaintext_len = len;

  /* Finalise the decryption. Further plaintext bytes may be written at
   * this stage.
   */
  if(1 != EVP_DecryptFinal_ex(ctx, plaintext + len, &len)) handleErrors();
  plaintext_len += len;

  /* Clean up */
  EVP_CIPHER_CTX_free(ctx);

  return plaintext_len;
}


// int encrypt(EVP_CIPHER_CTX * ctxe, unsigned char *plaintext, int plaintext_len, unsigned char *ciphertext){

//   int len;
//   int ciphertext_len;
 

//   if(1 != EVP_EncryptInit_ex(ctxe, EVP_aes_256_ecb(), NULL, key, NULL)) handleErrors();
// //  EVP_CIPHER_CTX_set_padding(ctxe, 1);
//   if(1 != EVP_EncryptUpdate(ctxe, ciphertext, &len, plaintext, plaintext_len))
// 	handleErrors();
//   ciphertext_len = len;
//   if(1 != EVP_EncryptFinal_ex(ctxe, ciphertext + len, &len)) 
// 	handleErrors();
//   ciphertext_len += len;

//   return ciphertext_len;

// }

// int decrypt(EVP_CIPHER_CTX * ctxgd, unsigned char *ciphertext, int ciphertext_len, unsigned char *plaintext){
	
//   int plaintext_len, len;

//   if(1 != EVP_DecryptInit_ex(ctxd, EVP_aes_256_ecb(), NULL, key, NULL)) 	handleErrors();
// //  EVP_CIPHER_CTX_set_padding(ctxd, 0);
//   if(1 != EVP_DecryptUpdate(ctxd, plaintext, &len, ciphertext, ciphertext_len)) 
//   {
// 	BIO_dump_fp (stdout, (const char *)ciphertext, ciphertext_len);
// 	handleErrors();
//   }
//   plaintext_len = len;
//   if(1 != EVP_DecryptFinal_ex(ctxd, plaintext + len, &len)) 
//   {
// 	BIO_dump_fp (stdout, (const char *)ciphertext, ciphertext_len);
// 	handleErrors();
//   }
//   plaintext_len += len;
//   return plaintext_len;
// }