CC = gcc
CFLAGS= -Wall

all: serwer klient

serwer: inf141325_s.c
	$(CC) $(CFLAGS) inf141325_s.c -o serwer

klient: inf141325_k.c
	$(CC) $(CFLAGS) inf141325_k.c -o klient


