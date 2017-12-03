#include "hash.h"

size_t hashFunc(const char *key) {
    unsigned long hash = 5381;
    int c;

    while ((c = *key++))
        hash = ((hash << 5) + hash) + c;

	return hash % HASH_MIN;
}

Hash* inicializaHash(size_t size, size_t (*func)(const char *key)) {
	
	Hash *ht = malloc(sizeof *ht);
	if(ht == NULL)
		return NULL;

	ht->size = size;
	ht->func = func;
	ht->buckets = calloc(size, sizeof *ht->buckets);
	if(ht->buckets == NULL)
		return NULL;
	ht->count = 0;

	pthread_mutex_t new_mutex;
	pthread_mutex_init(&new_mutex, NULL);
	ht->ht_mutex = new_mutex;

	return ht;
}

Nodo *insereHash(Hash *ht, const char *key, void *value){

	size_t index = ht->func(key)&(ht->size - 1);
	Nodo *node;

	// Se não houver colisao...
	if(ht->buckets[index] == NULL) {
		ht->buckets[index] = malloc(sizeof *ht->buckets[index]);
		if(ht->buckets[index] == NULL)
			return NULL;
		ht->buckets[index]->key = key;
		ht->buckets[index]->value = value;
		ht->buckets[index]->left = NULL;
		ht->buckets[index]->right = NULL;

		pthread_mutex_t new_mutex;
		pthread_mutex_init(&new_mutex, NULL);
		ht->buckets[index]->n_mutex = new_mutex;

		node = ht->buckets[index];

	// Se houver colisao... **TREE**
	}
	else{

		pthread_mutex_lock(&ht->buckets[index]->n_mutex);

		Nodo *prev = NULL, *left = NULL, *right = NULL;
		Nodo *curr = ht->buckets[index];

		while(curr) {
			if(strcmp(curr->key, key) == 0) {
				pthread_mutex_unlock(&ht->buckets[index]->n_mutex);
				return NULL;
			}

			prev = curr;
			if(strcmp(curr->key, key) == 1) {
				curr = left;
			} else {
				curr = right;
			}
		}

		Nodo *new = malloc(sizeof *new);

		new->key = key;
		new->value = value;
		new->left = NULL;
		new->right = NULL;

		pthread_mutex_t new_mutex;
		pthread_mutex_init(&new_mutex, NULL);
		new->n_mutex = new_mutex;

		if(strcmp(prev->key, key) == 1)
			prev->left = new;
		else
			prev->right = new;

		node = new;

		pthread_mutex_unlock(&ht->buckets[index]->n_mutex);
	}

	pthread_mutex_lock(&ht->ht_mutex);
	ht->count++;
	pthread_mutex_unlock(&ht->ht_mutex);

	return node;
}

void *consultaHash(Hash *ht, const char *key) {
	size_t index = ht->func(key) & (ht->size - 1);

	pthread_mutex_lock(&ht->buckets[index]->n_mutex);

	Nodo *curr = ht->buckets[index];

	while(curr) {
		if(strcmp(curr->key, key) == 0) {
			pthread_mutex_unlock(&ht->buckets[index]->n_mutex);
			return curr->value;
		}
		
		if(strcmp(curr->key, key) == 1)
			curr = curr->left;
		else
			curr= curr->right;
	}

	pthread_mutex_unlock(&ht->buckets[index]->n_mutex);

	return NULL;
}

void liberaHash(Hash *ht) {
	pthread_mutex_lock(&ht->ht_mutex);
	Nodo *curr;

	size_t i;
	for(i=0; i<ht->size; i++) {
		curr = ht->buckets[i];
		free(curr);
	}

	free(ht->buckets);
	pthread_mutex_unlock(&ht->ht_mutex);
	free(ht);

	return;
}

size_t ht_count(const Hash *ht) {
	return ht->count;
}
