default:
	gcc -o client client.c -lpthread -lmcrypt
	gcc -o server server.c -lpthread -lmcrypt

clean:
	rm *.txt *~ server client

server:
	gcc -o server server.c -lpthread -lmcrypt

client:
	gcc -o client client.c -lpthread -lmcrypt

dist:
	tar -czvf lab1b-004331063.tar.gz server.c client.c Makefile README my.key
