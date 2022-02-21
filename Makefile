FLAGS := -std=c++17 -Wall -pedantic-errors -O2 -pthread
EXE := ftp_serv
HEADER := myftp
CLIENT := client
SERVER := myftpserver
nPORT := 2273
tPORT := 2276

run: $(EXE) $(HEADER)

%: server_source/%.cpp server_source/*.cpp
	g++ $(FLAGS) $^ -o $(SERVER)

$(HEADER): $(CLIENT).cpp
	g++ $(FLAGS) $< -o server_source/$@

clean:
	rm $(SERVER) server_source/$(HEADER)

memleak:
	valgrind --leak-check=full ./$(SERVER)

serv:
	./$(SERVER) $(nPORT) $(tPORT)

cli:
	./$(HEADER) localhost $(nPORT) $(tPORT)

move:
	scp myftpserver jeh82014@odin.cs.uga.edu:~

commit:
	./.commit.sh

