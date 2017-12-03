#include <glib.h>
#include "casanova.h"
#include "hash.h"


void imprimeAlgo(char* buffer, FILE *fp, Hash* lista){
    printf("Imprimiindoo;;;\n");
    char item[GET_MESSAGE_SIZE];
    unsigned char* telefone_busca;

    for(int i = 0; i < N_MESSAGE; ++i){
        strncpy(&item[0], &buffer[GET_MESSAGE_SIZE*i], sizeof(item));
        printf("Procurando por: %s\n", item);
    }

    telefone_busca = buscaHash((unsigned char*)item, lista);
    

    // printf("RETORNOU!!!!\n");

    
    if(telefone_busca != NULL){    
        printf("O telefone de %s eh %s\n", item, telefone_busca);
    }
    else{
        printf("nao encontrou\n");
    }


    //printf("tamanho do buffer %lu e imp ------- %s\n", strlen(buffer), buffer);

}


void put_entries(Hash * lista){

    int server_sockfd, client_sockfd;
    struct sockaddr_un server_address;
    struct sockaddr_un client_address;
	socklen_t addr_size;

	int n, count=0, bytesrw = 0;
	char buffer[PUT_MESSAGE_SIZE*170];
	int read_ret, read_total, m_avail;

	buffer[PUT_MESSAGE_SIZE]='\0';

    /* inicializa SOCK_STREAM */
    unlink(SOCK_PUT_PATH);
    server_sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
    server_address.sun_family = AF_UNIX;
    strcpy(server_address.sun_path, SOCK_PUT_PATH);
    bind(server_sockfd, (struct sockaddr *)&server_address, sizeof(server_address));
    
	/* aguarda conexão */
	listen(server_sockfd, 5);

	//fprintf(stderr, "PUT WAITTING\n");
	addr_size=sizeof(client_address);
    client_sockfd = accept(server_sockfd, (struct sockaddr *)&client_address, &addr_size);
	//fprintf(stderr, "PUT CONNECTED\n");


	do {
        
        /* numero de mensagens inteiras no buffer */
	    ioctl(client_sockfd, FIONREAD, &bytesrw);
        m_avail = bytesrw / PUT_MESSAGE_SIZE; 
        if(m_avail == 0){
        	m_avail = 1;
        }
        if(m_avail > 170){
        	m_avail = 170;
        }

        /* le o número de mensagens escolhido */ 
        read_total=0;
	    do {
			read_ret=read(client_sockfd, buffer, PUT_MESSAGE_SIZE*m_avail-read_total); 
            read_total+=read_ret;
        } while (read_total < PUT_MESSAGE_SIZE*m_avail && read_ret > 0 );
	

        if (read_ret <=0)
           m_avail=0;

        /* armazena todas as entradas lidas */
        for (n=0;n<m_avail;n++)
        	insereHash((unsigned char*) buffer+n*PUT_MESSAGE_SIZE, lista);

	    count+=m_avail;
	} while (read_ret > 0);
       
    close(client_sockfd);
	//fprintf(stderr, "PUT EXITED, %d MESSAGES RECEIVED \n", count);
}


void get_entries(Hash* lista){

    int server_sockfd, client_sockfd, n;
    struct sockaddr_un server_address;
    struct sockaddr_un client_address;
    socklen_t addr_size;

    FILE *fp;
    int count=0, read_ret, read_total, m_avail, bytesrw;

    char buffer[GET_MESSAGE_SIZE*273];

    /* inicializa SOCK_STREAM */
    unlink(SOCK_GET_PATH);
    server_sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
    server_address.sun_family = AF_UNIX;
    strcpy(server_address.sun_path, SOCK_GET_PATH);
    bind(server_sockfd, (struct sockaddr *)&server_address, sizeof(server_address));
    listen(server_sockfd, 5);

    fprintf(stderr, "GET WAITTING\n");
    addr_size=sizeof(client_address);
    client_sockfd = accept(server_sockfd, (struct sockaddr *)&client_address, &addr_size);
    fprintf(stderr, "GET CONNECTED\n");
    fp=fopen(OUTPUTFILE, "w+");
}


void main(int argc, int** argv){
    
    
    
}