#pragma once
#include <atomic>
#include <string>
#include <thread>

#include "crypto.h"

class Connection {
 public:
  Connection(int sockfd, bool is_server, std::string username);
  ~Connection();

  void Run();  // starts the chat

  bool SendMsg(const std::string& msg);
  std::string ReceiveMsg();

 private:
  int sockfd_;  // the socket file descriptor the OS gave us
  bool is_server_;
  uint8_t session_key_[kAesKeySize];
  std::string server_username_;
  std::string client_username_;

  // atomic means its safe to read/write from two threads
  // without them stepping on each other
  std::atomic<bool> done_{false};

  void SendLoop();     // reads your keyboard, sends over socket
  void ReceiveLoop();  // reads from socket, prints to screen
};