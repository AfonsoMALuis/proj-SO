# Makefile, versao 1
# Sistemas Operativos, DEI/IST/ULisboa 2020-21

CC   = gcc
LD   = gcc
CFLAGS =-pthread -Wall -std=gnu99 -I../
LDFLAGS=-lm

# A phony target is one that is not really the name of a file
# https://www.gnu.org/software/make/manual/html_node/Phony-Targets.html
.PHONY: all clean run

all: tecnicofs tecnicofs-client

tecnicofs: Server/fs/state.o Server/fs/operations.o Server/main.o
	$(LD) $(CFLAGS) $(LDFLAGS) -o Server/tecnicofs Server/fs/state.o Server/fs/operations.o Server/main.o

Server/fs/state.o: Server/fs/state.c Server/fs/state.h tecnicofs-api-constants.h
	$(CC) $(CFLAGS) -o Server/fs/state.o -c Server/fs/state.c

Server/fs/operations.o: Server/fs/operations.c Server/fs/operations.h Server/fs/state.h tecnicofs-api-constants.h
	$(CC) $(CFLAGS) -o Server/fs/operations.o -c Server/fs/operations.c

Server/main.o: Server/main.c Server/fs/operations.h Server/fs/state.h tecnicofs-api-constants.h
	$(CC) $(CFLAGS) -o Server/main.o -c Server/main.c


tecnicofs-client: Client/tecnicofs-client-api.o Client/tecnicofs-client.o
	$(LD) $(CFLAGS) $(LDFLAGS) -o Client/tecnicofs-client Client/tecnicofs-client-api.o Client/tecnicofs-client.o

Client/tecnicofs-client.o: Client/tecnicofs-client.c tecnicofs-api-constants.h Client/tecnicofs-client-api.h
	$(CC) $(CFLAGS) -o Client/tecnicofs-client.o -c Client/tecnicofs-client.c

Client/tecnicofs-client-api.o: Client/tecnicofs-client-api.c tecnicofs-api-constants.h Client/tecnicofs-client-api.h
	$(CC) $(CFLAGS) -o Client/tecnicofs-client-api.o -c Client/tecnicofs-client-api.c




clean:
	@echo Cleaning...
	rm -f Server/fs/*.o Server/*.o Client/*.o Server/tecnicofs Client/tecnicofs-client

