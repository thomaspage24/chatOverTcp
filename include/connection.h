#pragma once
#include <string>
#include <thread>
#include <atomic>
#include "crypto.h"

class Connection {
public:
    Connection(int sockfd, bool is_server);
    ~Connection();

    void run(); // starts the chat

    bool send_msg(const std::string& msg);
    std::string receive_msg();

private:
    int sockfd_; // the socket file descriptor the OS gave us
    bool is_server_;
    uint8_t session_key_[AES_KEY_SIZE];

    // atomic means its safe to read/write from two threads
    // without them stepping on each other
    std::atomic<bool> done_{false};

    void send_loop(); // reads your keyboard, sends over socket
    void receive_loop(); // reads from socket, prints to screen
};