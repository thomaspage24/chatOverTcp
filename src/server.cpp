#include "server.h"
#include "connection.h"
#include <iostream>
#include <stdexcept>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
/**
 * Starts the server on a given port
 * 
 * binds to port, waits for a client to connect
 * hands off to Connection::run for chat session
 * Blocks until the chat session ends
 * 
 * @param port  the port number to listen on
 */
void run_server(int port) {
    // AF_INET = IPv4, SOCK_STREAM = TCP
    int listening_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (listening_socket < 0)
        throw std::runtime_error("socket() failed");

    // this lets us restart the server quickly without waiting 60 seconds
    // (the OS keeps ports "in use" briefly after closing, this skips that)
    const int enable = 1;
    setsockopt(listening_socket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable));

    // set up the address — listen on any network interface
    sockaddr_in server_address{};
    server_address.sin_family      = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port        = htons(static_cast<uint16_t>(port));

    if (bind(listening_socket, (sockaddr*)&server_address, sizeof(server_address)) < 0) {
        throw std::runtime_error("bind() failed — is the port already in use?");
    }
        
    // start listening, queue up to 1 pending connection
    listen(listening_socket, 1);
    std::cout << "[*] waiting on port " << port << "...\n";

    // blocks here until someone connects
    sockaddr_in client_address{};
    socklen_t client_address_len = sizeof(client_address);
    
    // accept() takes a generic sockaddr* because it works with any address
    // we pass our IPv4 sockaddr_in cast to sockaddr* 
    int client_socket = accept(listening_socket, (sockaddr*)&client_address, &client_address_len);

    if (client_socket < 0) {
        throw std::runtime_error("accept() failed");
    }
        
    // convert binary IP addr to string e.g. 192.0.0.1
    std::cout << "[*] got connection from " << inet_ntoa(client_address.sin_addr) << "\n";

    close(listening_socket); // dont need this anymore

    Connection conn(client_socket, true);
    conn.run();
}