#include <setup.h>

#include <fstream>
#include <iostream>
#include <string>

namespace Setup {
/**
 * Prints a welcome banner to the user
 */
void WelcomeBanner() {
  std::string line;
  std::ifstream welcome_banner("assets/welcome.txt");
  if (welcome_banner.is_open()) {
    while (std::getline(welcome_banner, line)) {
      std::cout << line << '\n';
    }
    welcome_banner.close();
  } else {
    std::cout << "unable to open file";
  }
}
/**
 * Prompts user to enter a username for the app
 * @return string  username
 */
std::string PromptUsername() {
  std::string username;
  while (username.empty()) {
    std::cout << "Enter a username: ";
    std::cin >> username;
  }
  std::cin.ignore();
  return username;
}
}  // namespace Setup
