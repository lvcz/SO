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
#include <openssl/conf.h>
#include <openssl/evp.h>
#include <openssl/err.h>
#include <openssl/engine.h>


// Hash
#define TAM_HASH 52

typedef struct nodo{
	char nome[16];
	char telefone[16];
	struct nodo *esq;
	struct nodo *dir;
}Nodo;

typedef struct hash{
	Nodo* tabela[TAM_HASH];
}Hash;






// Crypt
#define CRYPTEDSIZE 16

int encrypt(unsigned char *plaintext, int plaintext_len, unsigned char *key, unsigned char *iv, unsigned char *ciphertext);
int decrypt(unsigned char *ciphertext, int ciphertext_len, unsigned char *key, unsigned char *iv, unsigned char *plaintext);
int funcaoEspalhamento(unsigned char* nome);
void inicializaHash(Hash *topzera);
Nodo* insereBinario(unsigned char* nome, char* telefone, Nodo *raiz);
Hash* insereHash(unsigned char* registro, Hash* lista);
unsigned char* buscaBinaria(unsigned char *nome, Nodo *raiz);
unsigned char* buscaHash(unsigned char* nome, Hash *lista);
void deletaBinaria(Nodo* raiz);
void deletaHash(char* nome, Hash *lista);
void imprimeBinaria(Nodo* raiz);
void imprimeHash(Hash* lista);