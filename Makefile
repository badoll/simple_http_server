WALL = -Wall
SRC = .
OBJ = ./obj
BIN = ./bin
SERVER = $(BIN)/server
OBJECT = $(OBJ)/server.o $(OBJ)/server_func.o
SERVER_HEADER = $(SRC)/server.h $(SRC)/server_func.h

.PHONY: all
all: server

server: $(OBJECT)
	gcc $^ -o $(SERVER) $(WALL)

$(OBJECT): $(SERVER_HEADER)

$(OBJ)/%.o: $(SRC)/%.c
	gcc -c $< -o $@ $(WALL)

.PHONY: clean
clean:
	rm -f $(SERVER)
	rm -f $(OBJ)/*.o


