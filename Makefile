FLAGS := -std=c++17 -Wall -pedantic-errors -O2 -pthread
EXE := ftp_serv
HEADER := myftp
CLIENT := myftp
SERVER := myftpserver

run: $(EXE) $(CLIENT)

%: %.cpp $(HEADER).cpp
	g++ $(FLAGS) $^ -o $(SERVER)

$(CLIENT): $(CLIENT).cpp
	g++ $(FLAGS) $< -o $@

clean:
	rm $(SERVER) $(CLIENT)

memleak:
	valgrind --leak-check=full ./$(SERVER)

serv:
	./$(SERVER)

cli:
	./$(CLIENT)

commit:
	./.commit.sh

