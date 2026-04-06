#include "client.h"
#include "connection.h"
#include <iostream>
#include <stdexcept>
#include <cstring>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
/**
 * Connects to a server at host:port and starts chat session
 * 
 * resolves hostname, creates a socket, connects, then hands off to
 * Connection::run() for chat session
 * Blocks until chat session ends
 * 
 * @param host hostname or ip addr
 * @param port port number to connect to
 */
void RunClient(const std::string& host, int port) {
    // getaddrinfo handles both "localhost" and "192.168.1.x" style addresses
    // returns linked list of address_infos -> we use first 
    addrinfo hints{};                   // struct from C networking lib, zero intiailise
    hints.ai_family   = AF_INET;        // IPv4 only
    hints.ai_socktype = SOCK_STREAM;    // TCP only 

    addrinfo* address_info = nullptr;
    std::string port_str = std::to_string(port);
    
    // we have to convert to c strings bcs c function
    if (getaddrinfo(host.c_str(), port_str.c_str(), &hints, &address_info) != 0)
        throw std::runtime_error("couldnt resolve host: " + host);

    int client_socket = socket(address_info->ai_family, 
                                address_info->ai_socktype, 
                                address_info->ai_protocol);
    
    if (client_socket < 0) {
        freeaddrinfo(address_info);
        throw std::runtime_error("socket() failed");
    }

    std::cout << "[*] connecting to " << host << ":" << port << "...\n";

    if (connect(client_socket, address_info->ai_addr, address_info->ai_addrlen) < 0) {
        freeaddrinfo(address_info);
        close(client_socket);
        throw std::runtime_error("connect() failed — is the server running?");
    }

    freeaddrinfo(address_info);
    std::cout << "[*] connected!\n";

    Connection conn(client_socket, false);
    conn.Run();
}