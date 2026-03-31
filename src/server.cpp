#include "server.h"
#include "connection.h"
#include <iostream>
#include <stdexcept>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

void run_server(int port) {
    // create the socket
    // IPv4 -> tcp
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0)
        throw std::runtime_error("socket() failed");

    // this lets us restart the server quickly without waiting 60 seconds
    // (the OS keeps ports "in use" briefly after closing, this skips that)
    const int enable = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable));

    // set up the address — listen on any network interfaceb
    sockaddr_in addr{};
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port        = htons(static_cast<uint16_t>(port));

    if (bind(listen_fd, (sockaddr*)&addr, sizeof(addr)) < 0) {
        throw std::runtime_error("bind() failed — is the port already in use?");
    }
        
    // start listening, queue up to 1 pending connection
    listen(listen_fd, 1);
    std::cout << "[*] waiting on port " << port << "...\n";

    // blocks here until someone connects
    sockaddr_in client_addr{};
    socklen_t client_len = sizeof(client_addr);
    
    // lots of casting since most these methods take generic sginatures since they work with any address family

    int conn_fd = accept(listen_fd, (sockaddr*)&client_addr, &client_len);

    if (conn_fd < 0) {
        throw std::runtime_error("accept() failed");
    }
        
    // internet num (ip) to ascii
    std::cout << "[*] got connection from " << inet_ntoa(client_addr.sin_addr) << "\n";

    close(listen_fd); // dont need this anymore

    Connection conn(conn_fd);
    conn.run();
}