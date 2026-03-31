#include "client.h"
#include "connection.h"
#include <iostream>
#include <stdexcept>
#include <cstring>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

void run_client(const std::string& host, int port) {
    // getaddrinfo handles both "localhost" and "192.168.1.x" style addresses
    addrinfo hints{};
    hints.ai_family   = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    addrinfo* result = nullptr;
    std::string port_str = std::to_string(port);
    
    // we have to convert to c strings bcs c function
    if (getaddrinfo(host.c_str(), port_str.c_str(), &hints, &result) != 0)
        throw std::runtime_error("couldnt resolve host: " + host);

    int sockfd = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    
    if (sockfd < 0) {
        freeaddrinfo(result);
        throw std::runtime_error("socket() failed");
    }

    std::cout << "[*] connecting to " << host << ":" << port << "...\n";

    if (connect(sockfd, result->ai_addr, result->ai_addrlen) < 0) {
        freeaddrinfo(result);
        close(sockfd);
        throw std::runtime_error("connect() failed — is the server running?");
    }

    freeaddrinfo(result);
    std::cout << "[*] connected!\n";

    Connection conn(sockfd);
    conn.run();
}