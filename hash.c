#include <glib.h>
#include "hash.h"


unsigned char *key = (unsigned char *)"01234567890123456789012345678901";
unsigned char *iv = (unsigned char *)"0123456789012345";



int funcaoEspalhamento(unsigned char* nome){
	// Imprime a Hash para consultar as operacoes
	int ascii = nome[0], indice;

	if((ascii > 64) && (ascii < 91))
		indice = ascii - 65;
	if(ascii > 96 && ascii < 122)
		indice = ascii - 97 + 26;

	return indice;
}

void inicializaHash(Hash *topzera){

	for (int i = 0; i < TAM_HASH; ++i){
		topzera->tabela[i] = NULL;
	}
}

Nodo* insereBinario(unsigned char* nome, char* telefone, Nodo *raiz){

	if(raiz == NULL){
		raiz = malloc(sizeof(Nodo));
		strcpy(raiz->nome, (char*)nome);
		strcpy(raiz->telefone, telefone);
		raiz->esq = NULL;
		raiz->dir = NULL;
		return raiz;
	}

	if(strcmp(raiz->nome, (char*)nome) < 0){
		if(raiz->esq){
			(insereBinario(nome, telefone, raiz->esq));
			return raiz;
		}
		else{
			raiz->esq = malloc(sizeof (Nodo));
			strcpy(raiz->esq->nome, (char*)nome);
			strcpy(raiz->esq->telefone, telefone);
			raiz->esq->esq = NULL;
			raiz->esq->dir = NULL;
			return raiz;
		}
	}
	if(strcmp(raiz->nome, (char*)nome) > 0){
		if(raiz->dir){
			(insereBinario(nome, telefone, raiz->dir));
			return raiz;
		}
		else{
			raiz->dir = malloc(sizeof (Nodo));
			strcpy(raiz->dir->nome, (char*)nome);
			strcpy(raiz->dir->telefone, telefone);
			raiz->dir->esq = NULL;
			raiz->dir->dir = NULL;
			return raiz;
		}
	}
	
	fprintf( stderr, "nome ja existente\n");
	return raiz;

}

Hash* insereHash(unsigned char* registro, Hash* lista){
	
	char* hexa = malloc(sizeof(char)*10);
	unsigned long int posicao;
	unsigned char nome_plaintext[15];
	unsigned char telefone_plaintext[9];
	unsigned char nome_criptografado[CRYPTEDSIZE];
	unsigned char telefone_criptografado[CRYPTEDSIZE];

	hexa[0] = '0';
	hexa[1] = 'x';

	//printf("\n\nDados:\n");


	memcpy(&hexa[2], &registro[0], 8);
	posicao = (int) strtol(hexa, NULL, 0);
	memcpy(&nome_plaintext[0], &registro[9], sizeof(nome_plaintext));
	memcpy(&telefone_plaintext[0], &registro[24], sizeof(telefone_plaintext));



	//printf("%ld -- %s -- %s\n", posicao, nome_plaintext, telefone_plaintext);


	// Chama a funcao de espalhamento
	int indice = funcaoEspalhamento(nome_plaintext);

	// Encripta

	//encrypt(nome_plaintext, strlen ((char *)nome_plaintext), key, iv, nome_criptografado);
	//encrypt(telefone_plaintext, strlen ((char *)telefone_plaintext), key, iv, telefone_criptografado);

	//lista->tabela[indice] = insereBinario( (unsigned char*) nome_criptografado, (char*) telefone_criptografado, lista->tabela[indice]);
	lista->tabela[indice] = insereBinario( (unsigned char*) nome_plaintext, (char*) telefone_plaintext, lista->tabela[indice]);
	
		
	return(lista);
}

unsigned char* buscaBinaria(unsigned char *nome, Nodo *raiz){

	if(raiz == NULL) return NULL;

	if(strcmp(raiz->nome, (char*)nome) == 0)
		return raiz->telefone;
	if(strcmp(raiz->nome, (char*)nome) < 0)
		return buscaBinaria(nome, raiz->esq);
	if(strcmp(raiz->nome, (char*)nome) > 0)
		return buscaBinaria(nome, raiz->dir);
}

unsigned char* buscaHash(unsigned char* nome, Hash *lista){
	int indice = funcaoEspalhamento(nome);

	return buscaBinaria(nome, lista->tabela[indice]);
}

void deletaBinaria(Nodo* raiz){
	
	if(raiz->esq)
		deletaBinaria(raiz->esq);
	if(raiz->dir)
		deletaBinaria(raiz->dir);
	free(raiz);
}

void deletaHash(char* nome, Hash *lista){
	int i;
	for(i=0;i<TAM_HASH;i++)
		deletaBinaria(lista->tabela[i]);
	
	free(lista);
}

void imprimeBinaria(Nodo* raiz){
	
	unsigned char nome_descriptografado[15];
	unsigned char telefone_descriptografado[9];
	//int i, j;
	if(raiz == NULL)
		return;
	
	//i = decrypt( (unsigned char) raiz->nome, CRYPTEDSIZE, key, iv, nome_descriptografado);
	//j = decrypt( (unsigned char) raiz->telefone, CRYPTEDSIZE, key, iv, telefone_descriptografado);
		
	printf("Nome: %s--- Telefone: %.9s-----\n", raiz->nome, raiz->telefone);
	
	imprimeBinaria(raiz->esq);
	imprimeBinaria(raiz->dir);
}

void imprimeHash(Hash* lista){
	int i;

	for(i=0;i<TAM_HASH;i++)
		imprimeBinaria(lista->tabela[i]);

}
