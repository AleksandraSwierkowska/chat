CC = gcc

all: serwer klient

serwer: inf141325_s.c
	$(CC) inf141325_s.c -o serwer

klient: inf141325_k.c
	$(CC) inf141325_k.c -o klient


