#include <fstream>
#include <iostream>
#include <string>

#include "client.h"
#include "server.h"
#include "setup.h"
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
      Setup::WelcomeBanner();
      std::string username = Setup::PromptUsername();
      RunServer(std::stoi(argv[2]), username);

    } else if (run_mode == "client") {
      if (argc < 4) {
        std::cerr << "need a host and port\n";
        return 1;
      }

      Setup::WelcomeBanner();
      std::string username = Setup::PromptUsername();
      RunClient(argv[2], std::stoi(argv[3]), username);
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
