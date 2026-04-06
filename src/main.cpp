#include "server.h"
#include "client.h"
#include <iostream>
#include <string>
/**
 * Entry point for app
 * 
 * Usage: 
 *      ./main server <port>
 *      ./main client <host> <port>
 * 
 * @param argc  number of cli arguments
 * @param argv  array of command line arg strings
 * @return      0 on exit, 1 on error
 */
int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "usage:\n"
                  << "  " << argv[0] << " server <port>\n"
                  << "  " << argv[0] << " client <host> <port>\n";
        return 1;
    }

    std::string run_mode = argv[1];
    
    // catches runtime error thrown by server/client
    // e.g. bind() or connect() failed.
    try {
        if (run_mode == "server") {
            run_server(std::stoi(argv[2]));
        } else if (run_mode == "client") {
            if (argc < 4) { std::cerr << "need a host and port\n"; return 1; }
            run_client(argv[2], std::stoi(argv[3]));
        } else {
            std::cerr << "unknown run_mode: " << run_mode << "\n";
            return 1;
        }
        
    } catch (const std::exception& e) {
        std::cerr << "[!] error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}