WALL = -Wall
PTHREAD = -lpthread
SRC = .
OBJ = ./obj
BIN = ./bin
SERVER_P = $(BIN)/mul_process_s
SERVER_T = $(BIN)/mul_thread_s
OBJECT_P = $(OBJ)/mul_process_s.o  $(OBJ)/server_func.o
OBJECT_T = $(OBJ)/mul_thread_s.o $(OBJ)/thread_func.o $(OBJ)/server_func.o
SERVER_HEADER_P = $(SRC)/mul_process_s.h $(SRC)/server.h $(SRC)/server_func.h
SERVER_HEADER_T = $(SRC)/mul_thread_s.h $(SRC)/server.h $(SRC)/server_func.h

.PHONY: all
all: mul_process_s mul_thread_s

mul_process_s: $(OBJECT_P)
	gcc $^ -o $(SERVER_P) $(WALL)

mul_thread_s: $(OBJECT_T)
	gcc $^ -o $(SERVER_T) $(WALL) $(PTHREAD)

$(OBJECT_P): $(SERVER_HEADER_P)
$(OBJECT_T): $(SERVER_HEADER_T)

$(OBJ)/%.o: $(SRC)/%.c
	gcc -c $< -o $@ $(WALL)

.PHONY: clean
clean:
	rm -f $(SERVER_P) $(SERVER_T)
	rm -f $(OBJ)/*.o


