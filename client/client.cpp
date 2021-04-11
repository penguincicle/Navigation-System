//--------------------------------------------
//  Name: Stuart Hamilton
//  ID: 1619864
//  CMPUT 275, Winter 2021
//
//   Assignment 1 Part 2
// --------------------------------------------

#include <iostream>
#include <unistd.h>
#include <sys/types.h>		// include for portability
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/socket.h>		// socket, connect
#include <arpa/inet.h>		// inet_aton, htonl, htons
#include <unistd.h>			// close
#include <cstring>			// strlen, strcmp
#include <cstdlib>          // atoi, atof
#include <string>
// Add more libraries, macros, functions, and global variables if needed
using namespace std;

#define MAX_SIZE 22 // bytes to read from plotter
#define BUFFER_SIZE 1024 // buffer size for reading from server via socket

int create_and_open_fifo(const char * pname, int mode) {
    // creating a fifo special file in the current working directory
    // with read-write permissions for communication with the plotter
    // both proecsses must open the fifo before they can perform
    // read and write operations on it
    if (mkfifo(pname, 0666) == -1) {
        cout << "Unable to make a fifo. Ensure that this pipe does not exist already!" << endl;
        exit(-1);
    }

    // opening the fifo for read-only or write-only access
    // a file descriptor that refers to the open file description is
    // returned
    int fd = open(pname, mode);

    if (fd == -1) {
        cout << "Error: failed on opening named pipe." << endl;
        exit(-1);
    }

    return fd;
}

int main(int argc, char const *argv[]) {
	// check argument count
    if (argc != 3) { 
        cout << "This program takes two command line arguments" << endl;
        return 0;
    }

    // extract the server's IPv4 address and port number from argument vector
    const char * server_ip = argv[2];   // the server ip address in numbers-and-dots notation
    int port_num = atoi(argv[1]);       // the server port number


    const char *inpipe = "inpipe";
    const char *outpipe = "outpipe";

    // Create and open input and output pipes
    int in = create_and_open_fifo(inpipe, O_RDONLY);
    cout << "inpipe opened..." << endl;
    int out = create_and_open_fifo(outpipe, O_WRONLY);
    cout << "outpipe opened..." << endl;

    // initalize buffers for the pipes
    char writerbuf[MAX_SIZE] = {0}; // outpipe buffer
    char readerbuf[MAX_SIZE] = {0}; // inpipe buffer



    // Create client socket and establish a connection with the server socket:

    // Declare structure variables that store local and peer socket addresses
	// sockaddr_in is the address sturcture used for IPv4 
	// sockaddr is the protocol independent address structure
	struct sockaddr_in my_addr, peer_addr;

	// zero out the structor variable because it has an unused part
	memset(&my_addr, '\0', sizeof my_addr);

	// Declare socket descriptor
	int socket_desc;

	// Initialize buffers for socket to server
	char outbound[BUFFER_SIZE] = {}; // socket write buffer
	char inbound[BUFFER_SIZE] = {}; // socket read buffer

	// Initialize c strings to send
    char ack[] = "A"; // Confirmation string
    char E_cstr[] = "E\n"; // string to send to plotter when finished
    char Q_cstr[] = "Q\n"; // string to send to server to terminate

    // Initialize conversion for division to double
    double convert = 100000; // double to convert from 100000th degrees to degrees


	/*
		socket() input arguments are:
		socket domain (AF_INET):	IPv4 Internet protocols
		socket type (SOCK_STREAM):	sequenced, reliable, two-way, connection-based
									byte streams
		socket protocol (0): 		OS selects a protocol that supports the requested
							 		socket type (in this case: IPPROTO_TCP)
		socket() returns a socket descriptor
	*/
	socket_desc = socket(AF_INET, SOCK_STREAM, 0);
	if (socket_desc == -1) {
		std::cerr << "Listening socket creation failed!\n";
		return 1;
	}

	// Prepare sockaddr_in structure variable
	peer_addr.sin_family = AF_INET;							// address family (2 bytes)
	peer_addr.sin_port = htons(port_num);				// port in network byte order (2 bytes)
															// htons takes care of host-order to short network-order conversion.
    inet_aton(server_ip, &(peer_addr.sin_addr));        // server IP address in network byte order

															// inet_aton converts the Internet host address from the IPv4 numbers-and-dots notation 
															// to binary form (network byte order)
															// htonl takes care of host-order to long network-order conversion.

	// connecting to the server socket
	if (connect(socket_desc, (struct sockaddr *) &peer_addr, sizeof peer_addr) == -1) {
		std::cerr << "Cannot connect to the host!\n";
		close(socket_desc);
		return 1;
	}
	std::cout << "Connection established with " << inet_ntoa(peer_addr.sin_addr) << ":" << ntohs(peer_addr.sin_port) << "\n";

    while (true) {
    	// Read coordinates of start point from inpipe (blocks until they are selected)
    	// If 'Q\n' is read instead of the coordinates then send Q and break.
        int bytesread = read(in, readerbuf, MAX_SIZE);
        if (bytesread == -1)
            std::cerr << "Error: read operation failed!" << endl;
        if (strstr(readerbuf, "Q\n") != NULL){
            send(socket_desc, Q_cstr, strlen(Q_cstr) + 1, 0);
            break;
        }
        // read start latitude and longitude and convert from degrees to 100000th degrees
        // and change to string
        char *start_ptr = strtok(readerbuf, " "); // read start latitude
        double startlat = atof(start_ptr);
        start_ptr = strtok(NULL, " "); // read start longitude
        double startlon = atof(start_ptr);
        long long ll_startlat = static_cast<long long>(startlat*100000);
        long long ll_startlon = static_cast<long long>(startlon*100000);
        std::string str_startlat = std::to_string(ll_startlat);
        std::string str_startlon = std::to_string(ll_startlon);

        // read end point coordinates from inpipe
        bytesread = read(in, readerbuf, MAX_SIZE);
        if (bytesread == -1)
            std::cerr << "Error: read operation failed!" << endl;

        // convert end point coords frm degrees to 100000th degrees and change to string
        char *end_ptr = strtok(readerbuf, " "); // read end latitude
        double endlat = atof(end_ptr);
        end_ptr = strtok(NULL, " "); // read end longitude
        double endlon = atof(end_ptr);
        long long ll_endlat = static_cast<long long>(endlat*100000);
        long long ll_endlon = static_cast<long long>(endlon*100000);
        std::string str_endlat = std::to_string(ll_endlat);
        std::string str_endlon = std::to_string(ll_endlon);

        // concatenate coordinate strings to send to server
        std::string out_string = "R "+ str_startlat +" "+ str_startlon +" "+ str_endlat +" "+ str_endlon;
        // send string as bytes to server
        send(socket_desc, out_string.c_str(), out_string.length() + 1, 0);

        // blocking call to recieve "N" followed by number of wapypoints
        int rec_size = recv(socket_desc, inbound, BUFFER_SIZE, 0);
        std::cout << "Received: " << inbound << std::endl;

        // Receives "N" followed by number of wapypoints
        char *n_ptr = strtok(inbound, " ");
        char com = n_ptr[0]; //  reads N character
        n_ptr = strtok(NULL, " "); // reads number of waypoints
        int points = atoi(n_ptr);

        // if no path, tell plotter to ask for new points
        if (points == 0){
            // write E to plotter
            if (write(out, E_cstr, strlen(E_cstr)) == -1){
                std::cerr << "Error: write operation failed!" << endl;
            }
            continue;
        }
        // If only one point in path, send acknowledgment and tell plotter to
        // ask for new points
        if (points == 1){
        	// Send confirmation to server
        	send(socket_desc, ack, strlen(ack)+1, 0);
            // write E to plotter
            if (write(out, E_cstr, strlen(E_cstr)) == -1){
                std::cerr << "Error: write operation failed!" << endl;
            }
            continue;
        }

        // Send confirmation to server
        send(socket_desc, ack, strlen(ack)+1, 0);

        // recieve waypoints from server and send through outpipe to plotter
        for(int i = 0; i<points; i++){

            // blocking call
            int rec_size = recv(socket_desc, inbound, BUFFER_SIZE, 0);
            std::cout << "Received: " << inbound << std::endl;

            char *w_ptr = strtok(inbound, " "); // read W character
            char com = w_ptr[0];
            w_ptr = strtok(NULL, " "); // read waypoint latitude in 100000th degrees
            long long pointlat = atoll(w_ptr); 
            w_ptr = strtok(NULL, " "); // read waypoint longitude in 100000th degrees
            long long pointlon = atoll(w_ptr);
            // convert coords to double in degrees, then to string
            double sendlat = static_cast<double>(pointlat/convert);
            double sendlon = static_cast<double>(pointlon/convert);
            std::string str_sendlat = std::to_string(sendlat);
            std::string str_sendlon = std::to_string(sendlon);
            // concatenate strings to send to plotter with newline character
            std::string send_str = str_sendlat +" "+ str_sendlon +"\n";

            // write waypoint to plotter up to newline character
            if (write(out, send_str.c_str(), send_str.size()) == -1)
                std::cerr << "Error: write operation failed!" << endl;

            // Send confirmation
            send(socket_desc, ack, strlen(ack)+1, 0);
        }

        // blocking call to receive E
        rec_size = recv(socket_desc, inbound, BUFFER_SIZE, 0);
        std::cout << "Received: " << inbound << std::endl;

        // write E to plotter to indicate completion
        if (write(out, inbound, strlen(inbound) ) == -1)
            std::cerr << "Error: write operation failed!" << endl;

    }

    // close socket
    close(socket_desc);


    // close pipes and table indexes
    close(in);
    close(out);
    unlink(inpipe);
    unlink(outpipe);
    return 0;
}
