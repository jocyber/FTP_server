FLAGS := -std=c++17 -Wall -pedantic-errors -O2 -pthread
EXE := ftp_serv
HEADER := myftp
CLIENT := client
SERVER := myftpserver
PORT := 2200

run: $(EXE) $(HEADER)

%: %.cpp $(HEADER).cpp
	g++ $(FLAGS) $^ -o $(SERVER)

$(HEADER): $(CLIENT).cpp
	g++ $(FLAGS) $< -o $@

clean:
	rm $(SERVER) $(HEADER)

memleak:
	valgrind --leak-check=full ./$(SERVER)

serv:
	./$(SERVER) $(PORT)

cli:
	./$(HEADER) odin.cs.uga.edu $(PORT)

move:
	scp myftpserver jeh82014@odin.cs.uga.edu:~

commit:
	./.commit.sh

