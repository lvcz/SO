#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <sys/un.h>
#include <unistd.h>
#include <pthread.h>    
#include <semaphore.h>
#include <sys/ioctl.h>
#include <openssl/conf.h>
#include <openssl/evp.h>
#include <openssl/err.h>
#include <openssl/engine.h>

#include "casanova.h"
#include "locking.h"
#include "hash.h"

#define CRYPTEDSIZE 16
#define NTHREADS	4

unsigned char *key = (unsigned char *)"01234567890123456789012345678901";
inline int encrypt(EVP_CIPHER_CTX *, unsigned char *, int , unsigned char *);
inline int decrypt(EVP_CIPHER_CTX *, unsigned char *, int , unsigned char *);

void handleErrors(void) {
	ERR_print_errors_fp(stderr);
}


int cont=0;

void get_entries();
void put_entries();
void store(char *);
void retrieve(char *, FILE *);
void put_entries_msg();
void get_entries_msg(FILE *);

Hash* hash;

sem_t sem_mutex_crypt;
sem_t sem_mutex_hash;
sem_t mutex_actual;
sem_t sem_getahead;

pthread_mutex_t lock_put, lock_get;

// Mantem o registro das entradas, para saber se uma consulta já foi inserida ou nao. 
char actual_put[ID_SIZE];
char actual_get[ID_SIZE];
int  getahead;

int server_sockfd_put, client_sockfd_put, server_sockfd_get, client_sockfd_get;

EVP_CIPHER_CTX *ctxp, *ctxge, *ctxgd;

int main(int argc, char *argv[]){

	hash = inicializaHash(HASH_MIN, hashFunc);

	/* inicialização */
	ERR_load_crypto_strings();
	OpenSSL_add_all_algorithms();
	OPENSSL_config(NULL);
	if(!(ctxp = EVP_CIPHER_CTX_new())) handleErrors();
	if(!(ctxge = EVP_CIPHER_CTX_new())) handleErrors();
	if(!(ctxgd = EVP_CIPHER_CTX_new())) handleErrors();

	/* inicializandos puts e gets recebidos, min/max */
	memset(actual_put, 0, ID_SIZE-1);
	memset(actual_get, 0, ID_SIZE-1);

	getahead = FALSE;

	sem_init(&sem_mutex_hash, 0, 1);
	sem_init(&sem_mutex_crypt, 0, 1);
	sem_init(&mutex_actual, 0, 1);
	sem_init(&sem_getahead, 0, 0);

	pthread_t thread1, thread2;  
	thread_setup();
	
	/* Cria um thread para a entrada e outra para a saída */
	pthread_create (&thread1, NULL, (void *) &put_entries, (void *) 0);
	pthread_create (&thread2, NULL, (void *) &get_entries, (void *) 0);

	/* espero a morte das threads */
	pthread_join(thread1, NULL);
	pthread_join(thread2, NULL);

	fprintf(stdout, "Cleaning up\n");
	/* limpeza geral, hash table, semáforos e biblioteca de criptografia */
	thread_cleanup();
	liberaHash(hash);
	sem_destroy(&sem_mutex_hash);
	sem_destroy(&sem_mutex_crypt);

	FIPS_mode_set(0);
	ENGINE_cleanup();         
	CONF_modules_unload(1);
	EVP_CIPHER_CTX_free(ctxp);
	EVP_CIPHER_CTX_free(ctxge);
	EVP_CIPHER_CTX_free(ctxgd);
	EVP_cleanup();
	CRYPTO_cleanup_all_ex_data();
	ERR_remove_state(0); 
	ERR_free_strings();

	return 0;
}


void store(char* buffer) {
	unsigned char telefone_crypt[CRYPTEDSIZE];
	unsigned char nome_crypt[CRYPTEDSIZE];
	GBytes *ghashnome, *ghashtelefone;

	encrypt( ctxp, (unsigned char*) buffer+ID_SIZE, NOME_SIZE, nome_crypt );
	encrypt( ctxp, (unsigned char*) (buffer+ID_SIZE+NOME_SIZE), FONE_SIZE, telefone_crypt );
	
	ghashnome     = g_bytes_new (nome_crypt, CRYPTEDSIZE);
	ghashtelefone = g_bytes_new (telefone_crypt, CRYPTEDSIZE);

	sem_wait(&sem_mutex_hash);
	insereHash(hash, (char *)ghashnome, (char *)ghashtelefone);
	//printf("TAM: %d\n", ht_count(hash));
	sem_post(&sem_mutex_hash); 

	/* protegendo os valores dos atuais */
	sem_wait(&mutex_actual);
	memcpy(actual_put, buffer, ID_SIZE-1); 
	
	if ( getahead == TRUE) {
		/* se o put for maior ou igual libera o GET */
		if(strncmp(actual_get, actual_put, ID_SIZE-1) <= 0) {
			getahead = FALSE;
			sem_post(&sem_getahead);
		}
	}

	sem_post(&mutex_actual);
}

void put_entries() {
	unsigned int i;
	struct sockaddr_un server_address;
	struct sockaddr_un client_address;
	socklen_t addr_size;

	/* inicializa SOCK_STREAM */
	unlink(SOCK_PUT_PATH);
	server_sockfd_put = socket(AF_UNIX, SOCK_STREAM, 0);
	server_address.sun_family = AF_UNIX;
	strcpy(server_address.sun_path, SOCK_PUT_PATH);
	bind(server_sockfd_put, (struct sockaddr *)&server_address, sizeof(server_address));
				
	/* aguarda conexão */
	listen(server_sockfd_put, 5);

	fprintf(stderr, "PUT WAITTING\n");
	addr_size = sizeof(client_address);
	client_sockfd_put = accept(server_sockfd_put, (struct sockaddr *)&client_address, &addr_size);
	fprintf(stderr, "PUT CONNECTED\n");

	pthread_mutex_init(&lock_put, NULL);

	pthread_t put[NTHREADS];

	// Cria as threads do PUT
	for(i=0; i<NTHREADS; i++)
		pthread_create (&put[i], NULL, (void *) &put_entries_msg, (void *) 0);

	for(i=0; i<NTHREADS; i++)
		pthread_join(put[i], NULL);

	pthread_mutex_destroy(&lock_put);

	close(client_sockfd_put);
}

void put_entries_msg() {
	int n, count = 0, bytesrw = 0;
	char buffer[PUT_MESSAGE_SIZE*170];
	int read_ret, read_total, m_avail;

	buffer[PUT_MESSAGE_SIZE]='\0';
	
	do {
		/* numero de mensagens inteiras no buffer */
		ioctl(client_sockfd_put, FIONREAD, &bytesrw);
		m_avail = bytesrw / PUT_MESSAGE_SIZE; 
		if (m_avail == 0 ) m_avail = 1;
		if (m_avail > 170 ) m_avail = 170;

		/* le o número de mensagens escolhido */ 
		read_total = 0;
		do {
			read_ret = read(client_sockfd_put, buffer, PUT_MESSAGE_SIZE*m_avail-read_total); 
			read_total += read_ret;
		} while (read_total < PUT_MESSAGE_SIZE*m_avail && read_ret > 0 );
	

		if (read_ret <= 0)
			m_avail=0;
		/* armazena todas as entradas lidas */
		//pthread_mutex_lock(&lock_put);
		cont++;
		for (n=0; n<m_avail; n++)
			store(buffer+n*PUT_MESSAGE_SIZE);

		printf(" %d ", cont);
		cont--;
		//pthread_mutex_unlock(&lock_put);

			count += m_avail;
	} while (read_ret > 0);

	fprintf(stderr, "PUT EXITED, %d MESSAGES RECEIVED \n", count );
}

void retrieve(char* buffer, FILE *fp) {
	int telefoneint;
	unsigned char *telefonecrypt;
	unsigned char telefonedecrypt[FONE_SIZE+1];
	unsigned char nome_crypt[CRYPTEDSIZE+1];
	GBytes *ghashnome, *ghashtelefone;
	gsize tam = CRYPTEDSIZE;

	telefonedecrypt[FONE_SIZE] = '\0';
	nome_crypt[CRYPTEDSIZE] = '\0';

	/* protegendo os valores dos atuais */
	sem_wait(&mutex_actual);
	memcpy(actual_get, buffer, ID_SIZE-1); 
	buffer+=ID_SIZE; 

	/* se o get for maior, vai precisar esperar o put andar */
	if(strncmp(actual_get, actual_put, ID_SIZE-1) > 0){
		sem_post(&mutex_actual);
		getahead = TRUE;
		sem_wait(&sem_getahead);
	} else {
		sem_post(&mutex_actual);
	}

	/* criptografa nome para fazer a busca */
	encrypt( ctxge, (unsigned char*) buffer, NOME_SIZE, nome_crypt );
	ghashnome = g_bytes_new (nome_crypt, CRYPTEDSIZE);

	sem_wait(&sem_mutex_hash); 
	ghashtelefone = consultaHash(hash, (char *)ghashnome);

	if ( ghashtelefone == NULL) {
		sem_post(&sem_mutex_hash);
		fprintf(stderr, "NAO ENCONTRADO: %.15s\n", buffer); /*nao deve acontecer*/
		g_bytes_unref(ghashnome);
		return;
	}
	sem_post(&sem_mutex_hash);

	telefonecrypt= (unsigned char*) g_bytes_get_data (ghashtelefone,&tam);
	decrypt(ctxgd, telefonecrypt, CRYPTEDSIZE, telefonedecrypt);
	telefonedecrypt[FONE_SIZE] = '\0';
	/* se quiser ver as entradas descomente 
	fprintf(stdout, "%.15s %s\n", buffer, telefonedecrypt); */
	telefoneint = atoi((char*) telefonedecrypt);
	fwrite( (void*) &telefoneint, sizeof(int), 1, fp); 
	g_bytes_unref(ghashnome);
}

void get_entries() {
	FILE *fp;
	unsigned int i;
	struct sockaddr_un server_address;
	struct sockaddr_un client_address;
	socklen_t addr_size;

	/* inicializa SOCK_STREAM */
	unlink(SOCK_GET_PATH);
	server_sockfd_get = socket(AF_UNIX, SOCK_STREAM, 0);
	server_address.sun_family = AF_UNIX;
	strcpy(server_address.sun_path, SOCK_GET_PATH);
	bind(server_sockfd_get, (struct sockaddr *)&server_address, sizeof(server_address));
	listen(server_sockfd_get, 5);

	fprintf(stderr, "GET WAITTING\n");
	addr_size = sizeof(client_address);
	client_sockfd_get = accept(server_sockfd_get, (struct sockaddr *)&client_address, &addr_size);
	fprintf(stderr, "GET CONNECTED\n");

	fp = fopen(OUTPUTFILE, "w+"); 
	if(fp == NULL) {
		perror("OPEN:");
		exit(1);
	}

	pthread_mutex_init(&lock_get, NULL);

	pthread_t get[NTHREADS];

	// Cria as threads do GET
	for(i=0; i<NTHREADS; i++)
		pthread_create (&get[i], NULL, (void *) &get_entries_msg, (void *)fp);

	for(i=0; i<NTHREADS; i++)
		pthread_join(get[i], NULL);

	pthread_mutex_destroy(&lock_get);

	fclose(fp);

	close(client_sockfd_get);
}

void get_entries_msg(FILE *fp) {
	int count = 0, read_ret, read_total, m_avail, bytesrw;
	int n;

	char buffer[GET_MESSAGE_SIZE*273];

	do {
		/* vejo quantidade de bytes no buffer */
		ioctl(client_sockfd_get, FIONREAD, &bytesrw);
		/* numero de mensagens inteiras no buffer */
		m_avail = bytesrw / GET_MESSAGE_SIZE;
		if (m_avail == 0 ) m_avail = 1;
		if (m_avail > 273 ) m_avail = 273;

		read_total = 0;

		do {
			read_ret = read(client_sockfd_get, buffer, GET_MESSAGE_SIZE*m_avail-read_total);
			read_total += read_ret;
		} while(read_total < GET_MESSAGE_SIZE*m_avail && read_ret > 0);

		if(read_ret <= 0)
			m_avail = 0;

		pthread_mutex_lock(&lock_get);
		for (n=0; n<m_avail; n++)
			retrieve(buffer+n*GET_MESSAGE_SIZE,fp);
		pthread_mutex_unlock(&lock_get);

		count += m_avail;
	} while(read_ret > 0);

	fprintf(stderr, "GET EXITED, %d MESSAGES RECEIVED\n", count);
}


int encrypt(EVP_CIPHER_CTX * ctxe, unsigned char *plaintext, int plaintext_len, unsigned char *ciphertext) {
	
	int len;
	int ciphertext_len;
 
 	if(1 != EVP_EncryptInit_ex(ctxe, EVP_aes_256_ecb(), NULL, key, NULL)) 
		handleErrors();

	if(1 != EVP_EncryptUpdate(ctxe, ciphertext, &len, plaintext, plaintext_len))
		handleErrors();

	ciphertext_len = len;

	if(1 != EVP_EncryptFinal_ex(ctxe, ciphertext + len, &len)) 
		handleErrors();

	ciphertext_len += len;

	return ciphertext_len;

}

int decrypt(EVP_CIPHER_CTX * ctxd, unsigned char *ciphertext, int ciphertext_len, unsigned char *plaintext) {
	
	int plaintext_len, len;
 
	if(1 != EVP_DecryptInit_ex(ctxd, EVP_aes_256_ecb(), NULL, key, NULL))
		handleErrors();
	EVP_CIPHER_CTX_set_padding(ctxd, 0);

	if(1 != EVP_DecryptUpdate(ctxd, plaintext, &len, ciphertext, ciphertext_len)) {
		BIO_dump_fp (stdout, (const char *)ciphertext, ciphertext_len);
		handleErrors();
	}

	plaintext_len = len;

	if(1 != EVP_DecryptFinal_ex(ctxd, plaintext + len, &len)) {
		BIO_dump_fp (stdout, (const char *)ciphertext, ciphertext_len);
		handleErrors();
	}

	plaintext_len += len;

	return plaintext_len;
}