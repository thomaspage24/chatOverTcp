#include "server.h"
#include "client.h"
#include <iostream>
#include <string>

// the plumbing is complete
int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "usage:\n"
                  << "  " << argv[0] << " server <port>\n"
                  << "  " << argv[0] << " client <host> <port>\n";
        return 1;
    }

    std::string mode = argv[1];

    try {
        if (mode == "server") {
            run_server(std::stoi(argv[2]));
        } else if (mode == "client") {
            if (argc < 4) { std::cerr << "need a host and port\n"; return 1; }
            run_client(argv[2], std::stoi(argv[3]));
        } else {
            std::cerr << "unknown mode: " << mode << "\n";
            return 1;
        }
    } catch (const std::exception& e) {
        std::cerr << "[!] error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}