FLAGS := -Wall -pedantic-errors -O2 -pthread
EXE := ftp_serv
HEADER := myftp
CLIENT := client
SERVER := server

run: $(EXE) $(CLIENT)

%: %.cpp $(HEADER).cpp
	g++ $(FLAGS) $^ -o $(SERVER)

$(CLIENT): $(CLIENT).cpp
	g++ $(FLAGS) $< -o $@

clean:
	rm $(SERVER) $(CLIENT)

cli:
	./$(CLIENT)

ftp:
	./$(SERVER)

memleak:
	valgrind --leak-check=full ./$(SERVER)

commit:
	./.commit.sh

