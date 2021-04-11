//--------------------------------------------
//  Name: Stuart Hamilton
//  ID: 1619864
//  CMPUT 275, Winter 2021
//
//   Assignment 1 Part 2
// --------------------------------------------

#include <iostream>
#include <cassert>
#include <fstream>
#include <sstream>
#include <string>
#include <string.h>
#include <list>
#include <unistd.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include "wdigraph.h"
#include "dijkstra.h"
#include <cstring>      // strlen, strcmp
#include <sys/types.h>    // include for portability
#include <netdb.h>      // getaddrinfo, freeaddrinfo, INADDR_ANY (superset of netinet/in.h)

using namespace std;

#define LISTEN_BACKLOG 1 // Only serves one client, but can be modified for more
#define BUFFER_SIZE 1024 // buffer size for socket to client

struct Point {
    long long lat, lon;
};

// returns the manhattan distance between two points
long long manhattan(const Point& pt1, const Point& pt2) {
  long long dLat = pt1.lat - pt2.lat, dLon = pt1.lon - pt2.lon;
  return abs(dLat) + abs(dLon);
}

// finds the point that is closest to a given point, pt
int findClosest(const Point& pt, const unordered_map<int, Point>& points) {
  pair<int, Point> best = *points.begin();

  for (const auto& check : points) {
    if (manhattan(pt, check.second) < manhattan(pt, best.second)) {
      best = check;
    }
  }
  return best.first;
}

// reads graph description from the input file and builts a graph instance
void readGraph(const string& filename, WDigraph& g, unordered_map<int, Point>& points) {
  ifstream fin(filename);
  string line;

  while (getline(fin, line)) {
    // split the string around the commas, there will be 4 substrings either way
    string p[4];
    int at = 0;
    for (auto c : line) {
      if (c == ',') {
        // starting a new string
        ++at;
      }
      else {
        // appending a character to the string we are building
        p[at] += c;
      }
    }

    if (at != 3) {
      // empty line
      break;
    }

    if (p[0] == "V") {
      // adding a new vertex
      int id = stoi(p[1]);
      assert(id == stoll(p[1])); // sanity check: asserts if some id is not 32-bit
      points[id].lat = static_cast<long long>(stod(p[2])*100000);
      points[id].lon = static_cast<long long>(stod(p[3])*100000);
      g.addVertex(id);
    }
    else {
      // adding a new directed edge
      int u = stoi(p[1]), v = stoi(p[2]);
      g.addEdge(u, v, manhattan(points[u], points[v]));
    }
  }
}


int main(int argc, char* argv[]) {
  // check argument count
    if (argc != 2) { 
        cout << "This program takes one command line argument" << endl;
        return 0;
    }
  // extract the server's port number from argument vector
  int port_num = atoi(argv[1]);       // the server port number


  WDigraph graph;
  unordered_map<int, Point> points;

  // build the graph
  readGraph("edmonton-roads-2.0.1.txt", graph, points);


  // Create server listen and connect sockets then bind and establish connection
  // with the client socket:

  // declare structure variables that store local and peer socket addresses
  // sockaddr_in is the address sturcture used for IPv4 
  // sockaddr is the protocol independent address structure
  struct sockaddr_in my_addr, peer_addr;

  // zero out the structor variable because it has an unused part
  memset(&my_addr, '\0', sizeof my_addr);

  // declare variables for socket descriptors 
  int lstn_socket_desc, conn_socket_desc;

  // initalize buffer for input from client socket
  char inbuffer[BUFFER_SIZE] = {};

  // intialize cstrings to send
  char E_cstr[] = "E\n"; // send if finished sending waypoints
  char send_cstr0[] = "N 0"; // send if no waypoints

  /*
    socket() input arguments are:
    socket domain (AF_INET):  IPv4 Internet protocols
    socket type (SOCK_STREAM):  sequenced, reliable, two-way, connection-based
                  byte streams
    socket protocol (0):    OS selects a protocol that supports the requested
                  socket type (in this case: IPPROTO_TCP)
    socket() returns a socket descriptor
  */
  lstn_socket_desc = socket(AF_INET, SOCK_STREAM, 0);
  if (lstn_socket_desc == -1) {
    std::cerr << "Listening socket creation failed!\n";
    return 1;
  }

  // prepare sockaddr_in structure variable
  my_addr.sin_family = AF_INET;       // address family (2 bytes)
  my_addr.sin_port = htons(port_num);       // port in network byte order (2 bytes)
                        // htons takes care of host-order to short network-order conversion.
  my_addr.sin_addr.s_addr = htonl(INADDR_ANY);// internet address (4 bytes) INADDR_ANY is all local interfaces
                        // htons takes care of host-order to long network-order conversion.

  // note bind takes in a protocol independent address structure
  // hence we need to cast sockaddr_in* to sockaddr*
  if (bind(lstn_socket_desc, (struct sockaddr *) &my_addr, sizeof my_addr) == -1) {
      std::cerr << "Binding failed!\n";
      close(lstn_socket_desc);
      return 1;
  }
  std::cout << "Binding was successful\n";


  if (listen(lstn_socket_desc, LISTEN_BACKLOG) == -1) {
    std::cerr << "Cannot listen to the specified socket!\n";
    close(lstn_socket_desc);
    return 1;
  }

  socklen_t peer_addr_size = sizeof my_addr;

  // add outer while loop to allow the server to accept more than one connection request


  // extract the first connection request from the queue of pending connection requests
  // return a new connection socket descriptor which is not in the listening state
  conn_socket_desc = accept(lstn_socket_desc, (struct sockaddr *) &peer_addr, &peer_addr_size);
  if (conn_socket_desc == -1){
    std::cerr << "Connection socket creation failed!\n";
    // continue;
    return 1;
  }
  std::cout << "Connection request accepted from " << inet_ntoa(peer_addr.sin_addr) << ":" << ntohs(peer_addr.sin_port) << "\n";

  // declare structure variable that represents an elapsed time 
  // it stores the number of whole seconds and the number of microseconds
  // struct timeval timer = {.tv_sec = 1, .tv_usec = 10000};

  // Uncomment and implement for timeouts
  /*   setsockopt sets a socket option
  it takes a socket descriptor, an integer that represents the level at which the option resides,
  an integer that can be mapped to the option name, a buffer pointed to by a const void * that 
  contains the option value, and an integer for the size of that buffer

  to manipulate socket API options, use SOL_SOCKET for the level
  SO_RCVTIMEO and SO_SNDTIMEO are option names for receiving and sending timeouts respectively
  send and recv functions return -1 if timeout occurs
  */
  // if (setsockopt(conn_socket_desc, SOL_SOCKET, SO_RCVTIMEO, (void *) &timer, sizeof(timer)) == -1) {
  //  std::cerr << "Cannot set socket options!\n";
  //  close(conn_socket_desc);
  //  return 1;
  // }

  while (true) {
    // blocking call - blocks until a message arrives 
    // (unless O_NONBLOCK is set on the socket's file descriptor)
    // Receives "R" followed by start and end coords in 100000th degrees
    int rec_size = recv(conn_socket_desc, inbuffer, BUFFER_SIZE, 0);

    // if 'Q\n' is recieved then break, close socket and terminate
    std::cout << "Message received\n";
    if (strcmp("Q\n", inbuffer) == 0) {
      std::cout << "Connection will be closed\n";
      break;
    }
    // delimit cstring by space and take in start and end coords in 100000th degrees
    // and convert to long long
    Point sPoint, ePoint;
    char *c_ptr = strtok(inbuffer, " ");
    char c = c_ptr[0]; // read R character
    c_ptr = strtok(NULL, " ");
    sPoint.lat = atoll(c_ptr); // read start latitude
    c_ptr = strtok(NULL, " ");
    sPoint.lon = atoll(c_ptr); // read start longitude
    c_ptr = strtok(NULL, " ");
    ePoint.lat = atoll(c_ptr); // read end latitude
    c_ptr = strtok(NULL, " ");
    ePoint.lon = atoll(c_ptr); // read end longitude
    
    // Find closest points to given start and end points based on shortest
    // manhattan distance
    int start = findClosest(sPoint, points), end = findClosest(ePoint, points);

    // run dijkstra's, this is the unoptimized version that does not stop
    // when the end is reached but it is still fast enough
    unordered_map<int, PIL> tree;
    dijkstra(graph, start, tree);

    // if no path, send 'N 0' and wait for new coordinates
    if (tree.find(end) == tree.end()) {
      send(conn_socket_desc, send_cstr0, strlen(send_cstr0) + 1, 0);
      continue;
    }

    // read off the path by stepping back through the search tree
    list<int> path;
    while (end != start) {
      path.push_front(end);
      end = tree[end].first;
    }
    path.push_front(start);
    // Treat 'N 1' case like 'N 0' case but wait for acknowledgement
    if (path.size() == 1){
      char send_cstr1[] = "N 1";
      send(conn_socket_desc, send_cstr1, strlen(send_cstr1) + 1, 0);

    // blocking call - blocks until a message arrives 
    // (unless O_NONBLOCK is set on the socket's file descriptor)
    // blocks until acknowledgement from client is received
    rec_size = recv(conn_socket_desc, inbuffer, BUFFER_SIZE, 0);
    std::cout << "Acknowledgement received\n";
      continue;
    }

    // send the number of waypoints
    std::string send_str = "N " + to_string(path.size());
    send(conn_socket_desc, send_str.c_str(), send_str.length() + 1, 0);

    // blocking call - blocks until a message arrives 
    // (unless O_NONBLOCK is set on the socket's file descriptor)
    // blocks until acknowledgement from client is received
    rec_size = recv(conn_socket_desc, inbuffer, BUFFER_SIZE, 0);
    std::cout << "Acknowledgement received\n";
    
    // send each waypoint to server then wait for acknowledgement to client
    for (int v : path) {
      send_str = "W "+ to_string(points[v].lat) +" "+ to_string(points[v].lon);
      send(conn_socket_desc, send_str.c_str(), send_str.length() + 1, 0);

      // blocking call - blocks until a message arrives 
      // (unless O_NONBLOCK is set on the socket's file descriptor)
      // blocks until acknowledgement from client is received
      rec_size = recv(conn_socket_desc, inbuffer, BUFFER_SIZE, 0);
      std::cout << "Acknowledgement received\n";
    }

    // send 'E\n' when finished sending all waypoints to tell plotter there are no more
    send(conn_socket_desc, E_cstr, strlen(E_cstr) + 1, 0);


  }

  // close socket descriptors
  close(lstn_socket_desc);
  close(conn_socket_desc);




  return 0;
}
