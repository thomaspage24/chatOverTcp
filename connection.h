#pragma once
#include <string>
#include <thread>
#include <atomic>

class Connection {
public:
    Connection(int sockfd);
    ~Connection();

    void run(); // starts the chat

    // these are public because we'll need them later for the key exchange
    bool send_message(const std::string& msg);
    std::string receive_msg();

private:
    int sockfd_; // the socket file descriptor the OS gave us

    // atomic means its safe to read/write from two threads
    // without them stepping on each other
    std::atomic<bool> done_{false};

    void send_loop(); // reads your keyboard, sends over socket
    void receive_loop(); // reads from socket, prints to screen
};