LIBS = -L/usr/lib -lssl -lcrypto -lpthread
OBJS = tls_header.o

all: tls_client

tls_client: tls_client.c tls_header.o
	gcc -w -o tls_client tls_client.c $(OBJS) $(LIBS)

tls_header.o: tls_header.c
	gcc -c -o tls_header.o tls_header.c $(LIBS)

clean:
	rm -f tls_client tls_header.o

dist:
	tar -czvf lab4-004331063.tar.gz *.jpg *.c *.h Makefile README *.txt config_file
