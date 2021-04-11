# --------------------------------------------
#   Name: Stuart Hamilton
#   ID: 1619864
#   CMPUT 275, Winter 2021
#
#   Assignment 1 Part 2
# --------------------------------------------

all: client server

client: client.o
	g++ -o client/client client/client.o

client.o: client/client.cpp
	g++ -c client/client.cpp -o client/client.o

server: server.o digraph.o dijkstra.o
	g++ -o server/server server/server.o server/digraph.o server/dijkstra.o server/digraph.h server/dijkstra.h server/wdigraph.h server/heap.h

server.o: server/server.cpp
	g++ -c server/server.cpp -o server/server.o

dijkstra.o: server/dijkstra.cpp
	g++ -c server/dijkstra.cpp -o server/dijkstra.o

digraph.o: server/digraph.cpp
	g++ -c server/digraph.cpp -o server/digraph.o

clean:
	rm -f server/dijkstra.o server/server.o server/digraph.o server/server client/client client/client.o inpipe outpipe
