#include "connection.h"
#include <iostream>
#include <unistd.h>    // read(), write(), close()
#include <arpa/inet.h> // htonl(), ntohl()

// sometimes the OS doesnt send/receive everything in one go
// so we just keep looping until all the bytes are done

// fd is socket
static bool write_all(int fd, const void* buf, size_t len) {
    const char* ptr = static_cast<const char*>(buf);
    size_t remaining_bytes = len;

    while (remaining_bytes> 0) {
        ssize_t n = write(fd, ptr, remaining_bytes);
        if (n <= 0) {
            return false;
         }                   // connection died
        ptr += n;
        remaining_bytes -= n;
    }
    return true;
}

static bool read_all(int fd, void* buf, size_t len) {
    // instead we write into the buffer -> not a const ptr must be mutable
    char* ptr = static_cast<char*>(buf);
    size_t remaining_bytes = len;

    while (remaining_bytes > 0) {
        ssize_t n = read(fd, ptr, remaining_bytes);
        if (n <= 0) return false; // connection died
        ptr += n;
        remaining_bytes -= n;
    }
    return true;
}

/*
    Send and receive with length prefix
*/
bool Connection::send_message(const std::string& msg) {
    // htonl = "host to network long"
    // converts the number to big-endian so both machines agree on byte order
    uint32_t net_len = htonl(static_cast<uint32_t>(msg.size()));

    if (!write_all(sockfd_, &net_len, sizeof(net_len))) {
        return false;
    } 
    if (!write_all(sockfd_, msg.data(), msg.size())) {
        return false;
    }

    return true;
}

std::string Connection::receive_msg() {
    uint32_t net_len = 0;
    if (!read_all(sockfd_, &net_len, sizeof(net_len))){
        return "";
    } 

    // ntohl = "network to host long" — reverse of htonl
    uint32_t msg_len = ntohl(net_len);

    // sanity check — if someone sends us a 4GB message something is wrong
    if (msg_len == 0 || msg_len > 10 * 1024 * 1024) {
        return "";
    }

    std::string msg(msg_len, '\0');
    if (!read_all(sockfd_, msg.data(), msg_len)) {
        return "";
    }

    return msg;
}
/*
    The threads
*/
void Connection::send_loop() {
    std::string line;
    while (!done_ && std::getline(std::cin, line)) {
        if (line == "quit") {
            done_ = true;
            break;
        }
        if (!send_message(line)) {
            std::cerr << "\n[!] send failed\n";
            done_ = true;
            break;
        }
    }
    done_ = true;
}

void Connection::receive_loop() {
    while (!done_) {
        std::string msg = receive_msg();
        if (msg.empty()) {
            if (!done_) std::cout << "\n[*] other person disconnected\n";
            done_ = true;
            break;
        }
        // \r goes back to start of line so it overwrites the "> " prompt
        std::cout << "\r[them] " << msg << "\n> " << std::flush;
    }
}

Connection::Connection(int sockfd) : sockfd_(sockfd) {}

Connection::~Connection() {
    if (sockfd_ >= 0) close(sockfd_);
}

/*
    Where it all runs (lol!)
*/
void Connection::run() {
    std::cout << "[*] connected! type messages, 'quit' to exit\n> " << std::flush;

    // spin up receiv on another thread, this thread does send
    std::thread receive_thread(&Connection::receive_loop, this);

    send_loop(); // blocks here until done

    // closing the socket makes the receiv thread unblock and exit
    if (sockfd_ >= 0) {
        close(sockfd_);
        sockfd_ = -1;
    }

    // wait for recv thread to actually finish before we return
    // not doing this would be a bug — thread could access deleted memory
    if (receive_thread.joinable())  receive_thread.join();

    std::cout << "\n[*] bye\n";
}