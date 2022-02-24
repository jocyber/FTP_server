FLAGS := -std=c++17 -Wall -pedantic-errors -O2 -pthread
EXE := ftp_serv
HEADER := myftp
CLIENT := client
SERVER := myftpserver
nPORT := 7273
tPORT := 7276

run: $(EXE) $(HEADER)

%: server_source/%.cpp server_source/*.cpp
	g++ $(FLAGS) $^ -o server_source/$(SERVER)

$(HEADER): ./*.cpp
	g++ $(FLAGS) $^ -o $@

clean:
	rm $(HEADER) server_source/$(SERVER)

memleak:
	valgrind --leak-check=full ./$(SERVER)

serv:
	./$(SERVER) $(nPORT) $(tPORT)

cli:
	./$(HEADER) localhost $(nPORT) $(tPORT)

move:
	scp server_source/myftpserver jeh82014@odin.cs.uga.edu:~

commit:
	./.commit.sh

