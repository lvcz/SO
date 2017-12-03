C = gcc
CFLAGS = -w -D_REENTRANT  -std=c99

all: casanova put_client get_client

casanova: casanova.c casanova.h
	$(CC) $(CFLAGS) -o casanova encrypt.c hash.c casanova.c -I /usr/include/glib-2.0 $(shell pkg-config --cflags --libs glib-2.0) -lpthread  -lcrypto -std=c99

put_client: put_client.c casanova.h
	$(CC) $(CFLAGS) -o put_client put_client.c -I /usr/include/glib-2.0 $(shell pkg-config --cflags --libs glib-2.0) -lpthread  -lcrypto -w

get_client: get_client.c casanova.h
	$(CC) $(CFLAGS) -o get_client get_client.c -I /usr/include/glib-2.0 $(shell pkg-config --cflags --libs glib-2.0) -lpthread  -lcrypto

clean: 
	rm -f *.o *.*~ *~ put_client get_client casanova 
	rm -f *.o *.*~ *~ telefones 
