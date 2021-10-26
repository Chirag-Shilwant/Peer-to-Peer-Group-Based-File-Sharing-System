CC = g++
CFLAGS = -Wall
DEPS = client_header.h
OBJ = client.o calcSHA.o commands.o peerToPeer.o uploadAndDownload.o utilities.o
%.o: %.cpp $(DEPS)
	$(CC) $(CFLAGS) -c -o $@ $<

client: $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ -lssl -lcrypto -pthread 
 
clean:
	rm -rf *o client