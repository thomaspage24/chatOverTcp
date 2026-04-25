#include "connection.h"

#include <arpa/inet.h>  // htonl(), ntohl()
#include <stdlib.h>
#include <time.h>
#include <unistd.h>  // read(), write(), close()

#include <iostream>
#include <string>

#include "crypto.h"
#include "dh.h"

/**
 *   Writes exactly total_bytes bytes to a file descriptor (socket)
 *   The OS might not accept all bytes in one write() call, so we
 *   loop until everything is sent or the connection dies
 *   @param socket_fd   the file descriptor, (the socket to write to)
 *   @param data  pointer to the data we want to send
 *   @param total_bytes  how many bytes to send
 *   @return     true if all bytes were written, false if dead
 */
static bool WriteAll(int socket_fd, const void* data, size_t total_bytes) {
  const char* data_ptr = static_cast<const char*>(data);
  size_t bytes_remaining = total_bytes;

  while (bytes_remaining > 0) {
    ssize_t bytes_written = write(socket_fd, data_ptr, bytes_remaining);
    if (bytes_written <= 0) {
      return false;  // connetion died or error
    }
    data_ptr += bytes_written;
    bytes_remaining -= bytes_written;
  }
  return true;
}
/**
 * Reads exactly total_bytes from a socket into a buffer.
 * The os might not read all bytes in one read() call, so we
 * loop over until everything is read or the connection dies
 * @param socket_fd     the socket we read from
 * @param data          pointer to the data we want to read
 * @param total_bytes   how many bytes we need to read
 * @return              true if all bytes were read, false if connection dies
 */
static bool ReadAll(int socket_fd, void* data, size_t total_bytes) {
  // cast to char* so we can do pointer arithmetic (move through byte by byte)
  // cant be const since we write into it
  char* data_ptr = static_cast<char*>(data);
  size_t bytes_remaining = total_bytes;

  while (bytes_remaining > 0) {
    ssize_t bytes_read = read(socket_fd, data_ptr, bytes_remaining);
    if (bytes_read <= 0) return false;  // connection died
    data_ptr += bytes_read;
    bytes_remaining -= bytes_read;
  }
  return true;
}
/**
 * Sends length-prefixed message over the socket
 * @param incoming_msg   The message to send
 * @return      True if message was sent succesfully, false if the connection
 * died
 */
bool Connection::SendMsg(const std::string& incoming_msg) {
  std::vector<uint8_t> encrypted = Encrypt(incoming_msg, session_key_);

  // htonl = "host to network long"
  // converts the number to big-endian so both machines agree on byte order
  uint32_t net_total_bytes = htonl(static_cast<uint32_t>(encrypted.size()));

  if (!WriteAll(sockfd_, &net_total_bytes, sizeof(net_total_bytes))) {
    return false;
  }
  if (!WriteAll(sockfd_, encrypted.data(), encrypted.size())) {
    return false;
  }

  return true;
}
/**
 * Receives a complete message from the socket
 * @return the decrypted message, or empty if connection died
 */
std::string Connection::ReceiveMsg() {
  uint32_t net_total_bytes = 0;
  if (!ReadAll(sockfd_, &net_total_bytes, sizeof(net_total_bytes))) {
    return "";
  }

  // ntohl = "network to host long" — reverse of htonl
  uint32_t msg_total_bytes = ntohl(net_total_bytes);

  // sanity check — if someone sends us a 4GB message something is wrong
  if (msg_total_bytes == 0 || msg_total_bytes > 10 * 1024 * 1024) {
    return "";
  }

  std::vector<uint8_t> encrypted(msg_total_bytes);
  if (!ReadAll(sockfd_, encrypted.data(), msg_total_bytes)) {
    return "";
  }

  return Decrypt(encrypted, session_key_);
}
/**
 * Reads from stdin in a loop, sending each user_input_line
 * Exits if the user types "qyit", stdin closes or the connection dies
 *
 */
void Connection::SendLoop() {
  std::string user_input_line;
  while (!done_ && std::getline(std::cin, user_input_line)) {
    if (user_input_line == "quit") {
      done_ = true;
      break;
    }
    if (!SendMsg(user_input_line)) {
      std::cerr << "\n[!] send failed\n";
      done_ = true;
      break;
    }
  }
  done_ = true;
}
/**
 * @brief Get the local timestamp of the connection
 *
 * @return std::string of day/date/time
 */
std::string GetTimestamp() {
  time_t raw_time;
  struct tm* time_info;
  time(&raw_time);
  time_info = localtime(&raw_time);

  char buffer[10];
  strftime(buffer, sizeof(buffer), "%H:%M:%S", time_info);
  return {buffer};
}
/**
 * Listens on the socket in a loop, printing each received message to stdout
 * Exits if connection dies or done_ is set by the other thread
 *
 */
void Connection::ReceiveLoop() {
  const std::string kReset = "\033[0m";
  const std::string kRed = "\033[31m";
  const std::string kGreen = "\033[32m";
  const std::string kYellow = "\033[33m";
  const std::string kBlue = "\033[34m";
  const std::string kMagenta = "\033[35m";
  const std::string kCyan = "\033[36m";
  const std::string kBold = "\033[1m";
  while (!done_) {
    std::string incoming_msg = ReceiveMsg();
    if (incoming_msg.empty()) {
      if (!done_) std::cout << "\n[*] other person disconnected\n";
      done_ = true;
      break;
    }

    // \r goes back to start of user_input_line so it overwrites the "> " prompt
    std::string display_name = is_server_ ? client_username_ : server_username_;
    std::cout << kBold << kGreen << "\r[" << GetTimestamp() << "]" << kReset
              << kBold << kYellow << " [" << display_name << "] " << kReset
              << kMagenta << incoming_msg << kReset << "\n> " << std::flush;
  }
}

/**
 * Takes ownership of a connected socket
 * closed when objecte is destroyed
 */
Connection::Connection(int sockfd, bool is_server, std::string username)
    : sockfd_(sockfd), is_server_(is_server) {
  if (is_server_) {
    server_username_ = username;
    std::cout << server_username_ << " server";
  }

  else {
    client_username_ = username;
    std::cout << client_username_ << " client";
  }
}
/**
 * Closes socket if not closed already
 */
Connection::~Connection() {
  if (sockfd_ >= 0) close(sockfd_);
}

/**
 * Where it all runs (lol!)
 * Starts chat session.
 * Launches receive_loop on a background thread then runs send_loop on *THIS*
 * thread. Blocks until user quits or the connection dies
 */
void Connection::Run() {
  DhHandshake(sockfd_, is_server_, session_key_);

  // exchange usernames — server sends first, then client sends back
  std::string my_username = is_server_ ? server_username_ : client_username_;
  if (is_server_) {
    SendMsg(my_username);
    client_username_ = ReceiveMsg();
  } else {
    server_username_ = ReceiveMsg();
    SendMsg(my_username);
  }

  const std::string kRed = "\033[31m";
  const std::string kReset = "\033[0m";
  std::cout << "[*] connected! type messages, 'quit' to exit\n"
            << kRed << "> " << kReset << std::flush;

  // Receive loop runs on its own thread - it blocks waiting for incoming
  // messages this thread runs send_loop blocking on std::getline
  std::thread receive_thread(&Connection::ReceiveLoop, this);

  SendLoop();  // blocks here until done

  // closing the socket closing the socket unblocks the read call
  // causing receive loop to see empty and message
  if (sockfd_ >= 0) {
    close(sockfd_);
    sockfd_ = -1;
  }

  // wait for recv thread to actually finish before we return
  // skipping this would be a bug — thread could access deleted/freed memory
  if (receive_thread.joinable()) receive_thread.join();

  std::cout << "\n[*] bye\n";
}
