#ifndef HASHTABLE_H
#define HASHTABLE_H

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <pthread.h>

#define HASH_MIN 1000000
//#define HASH_MAX 20*HASH_MIN

typedef struct nodo {
	const char *key; // Nome
	void *value;	 // Telefone
	struct nodo *left, *right;
	pthread_mutex_t n_mutex;
} Nodo;

typedef struct hash {
	size_t size;
	size_t (*func)(const char *key);
	size_t count;
	Nodo **buckets;
	pthread_mutex_t ht_mutex;
} Hash;


size_t hashFunc(const char *key);
Hash *inicializaHash(size_t size, size_t (*func)(const char *key));
Nodo *insereHash(Hash *ht, const char *key, void *value);
void *consultaHash(Hash *ht, const char *key);
void liberaHash(Hash *ht);
size_t ht_count(const Hash *ht);

#endif