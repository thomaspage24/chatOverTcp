#include "dh.h"

#include <arpa/inet.h>
#include <openssl/evp.h>  // EVP_PKEY — the modern key API
#include <openssl/sha.h>  // SHA256
#include <unistd.h>

#include <iostream>
#include <stdexcept>
#include <vector>

// ---------------------------------------------------------------------------
// raw send/recv helpers for the handshake
//
// we cant use send_msg/recv_msg here because those are encrypted
// and we dont have a key yet — thats what we're trying to establish!
// so we do simple length-prefixed raw sends for the public key exchange.
// ---------------------------------------------------------------------------

/**
 * Send raw bytes over socket with a 4 byte len prefix
 *
 * used during handshake before session key exists
 * not encrypted -> used to exchange public values
 *
 * @param socket_fd socket to send on
 * @param data      bytes to send
 * @param num_bytes how many
 * @return          true if all bytes sent, false if connection died
 */
static bool RawSend(int socket_fd, const uint8_t* data, size_t num_bytes) {
  uint32_t network_byte_length = htonl(static_cast<uint32_t>(num_bytes));
  if (write(socket_fd, &network_byte_length, sizeof(network_byte_length)) !=
      sizeof(network_byte_length))
    return false;

  size_t bytes_sent = 0;
  while (bytes_sent < num_bytes) {
    ssize_t result =
        write(socket_fd, data + bytes_sent, num_bytes - bytes_sent);
    if (result <= 0) return false;
    bytes_sent += result;
  }
  return true;
}

/**
 * Receives raw bytes from socket, reading 4 byte length header first
 *
 * Used during handshake before session key exists
 *
 * @param socket_fd socket to read from
 * @return          received bytes
 */
static std::vector<uint8_t> RawRecv(int socket_fd) {
  uint32_t network_byte_length = 0;
  if (read(socket_fd, &network_byte_length, sizeof(network_byte_length)) !=
      sizeof(network_byte_length))
    throw std::runtime_error("handshake failed reading length");

  uint32_t expected_bytes = ntohl(network_byte_length);

  // X25519 public keys are always 32 bytes
  // larger = sussy
  if (expected_bytes == 0 || expected_bytes > 4096)
    throw std::runtime_error("handshake got sussy length");

  std::vector<uint8_t> receive_buffer(expected_bytes);
  size_t bytes_received = 0;
  while (bytes_received < expected_bytes) {
    ssize_t result = read(socket_fd, receive_buffer.data() + bytes_received,
                          expected_bytes - bytes_received);
    if (result <= 0) throw std::runtime_error("handshake connection died");
    bytes_received += result;
  }
  return receive_buffer;
}

// ---------------------------------------------------------------------------
// dh_handshake — ECDH with X25519 via the EVP API
//
// X25519 replaces classic DH with a 256-bit elliptic curve.
// No prime or generator to hardcode
// Public keys are always exactly 32 bytes, shared secret is always 32 bytes.
//
// Handshake order:
//   server sends first, client receives first.
//   if both sent first they'd block forever waiting on each other.
// ---------------------------------------------------------------------------

/**
 * ECDH key exchange over socket using X25519
 * both sides call this right after connecting
 * result = 32 byte shared secret written into out_key
 *
 * @param socket_fd the connected socket
 * @param is_server true if this side is the server
 * @param out_key   buffer to write the 32 byte AES key into
 */
void DhHandshake(int socket_fd, bool is_server, uint8_t out_key[kAesKeySize]) {
  // --- step 1: generate our X25519 key pair ---
  // EVP_PKEY_X25519 tells OpenSSL which curve to use
  // keygen picks a random private key and computes the public key from it
  EVP_PKEY_CTX* keygen_ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_X25519, nullptr);
  if (!keygen_ctx) throw std::runtime_error("EVP_PKEY_CTX_new_id() failed");

  if (EVP_PKEY_keygen_init(keygen_ctx) != 1) {
    EVP_PKEY_CTX_free(keygen_ctx);
    throw std::runtime_error("EVP_PKEY_keygen_init() failed");
  }

  EVP_PKEY* our_key = nullptr;
  if (EVP_PKEY_keygen(keygen_ctx, &our_key) != 1) {
    EVP_PKEY_CTX_free(keygen_ctx);
    throw std::runtime_error("EVP_PKEY_keygen() failed");
  }
  EVP_PKEY_CTX_free(keygen_ctx);

  // --- step 2: get our public key as raw bytes to send ---
  // X25519 public keys are always exactly 32 bytes
  size_t pub_len = 32;
  std::vector<uint8_t> our_public_bytes(pub_len);
  if (EVP_PKEY_get_raw_public_key(our_key, our_public_bytes.data(), &pub_len) !=
      1) {
    EVP_PKEY_free(our_key);
    throw std::runtime_error("failed to get raw public key");
  }

  // --- step 3: exchange public keys ---
  // server sends first, client receives first — both sides must agree on this
  // order
  std::vector<uint8_t> their_public_bytes;

  if (is_server) {
    if (!RawSend(socket_fd, our_public_bytes.data(), our_public_bytes.size())) {
      EVP_PKEY_free(our_key);
      throw std::runtime_error("failed to send public key");
    }
    their_public_bytes = RawRecv(socket_fd);
  } else {
    their_public_bytes = RawRecv(socket_fd);
    if (!RawSend(socket_fd, our_public_bytes.data(), our_public_bytes.size())) {
      EVP_PKEY_free(our_key);
      throw std::runtime_error("failed to send public key");
    }
  }

  // --- step 4: load their public key from the raw bytes we received ---
  EVP_PKEY* their_key = EVP_PKEY_new_raw_public_key(EVP_PKEY_X25519, nullptr,
                                                    their_public_bytes.data(),
                                                    their_public_bytes.size());
  if (!their_key) {
    EVP_PKEY_free(our_key);
    throw std::runtime_error("failed to load their public key");
  }

  // --- step 5: derive the shared secret ---
  // both sides compute: shared = our_private * their_public (on the curve)
  // elliptic curve guarantees both sides get the same result
  EVP_PKEY_CTX* derive_ctx = EVP_PKEY_CTX_new(our_key, nullptr);
  if (!derive_ctx) {
    EVP_PKEY_free(our_key);
    EVP_PKEY_free(their_key);
    throw std::runtime_error("EVP_PKEY_CTX_new() failed");
  }

  if (EVP_PKEY_derive_init(derive_ctx) != 1 ||
      EVP_PKEY_derive_set_peer(derive_ctx, their_key) != 1) {
    EVP_PKEY_CTX_free(derive_ctx);
    EVP_PKEY_free(our_key);
    EVP_PKEY_free(their_key);
    throw std::runtime_error("failed to init key derivation");
  }

  // call derive with null output first to find out the secret length
  size_t secret_len = 0;
  EVP_PKEY_derive(derive_ctx, nullptr, &secret_len);

  std::vector<uint8_t> raw_shared_secret(secret_len);
  if (EVP_PKEY_derive(derive_ctx, raw_shared_secret.data(), &secret_len) != 1) {
    EVP_PKEY_CTX_free(derive_ctx);
    EVP_PKEY_free(our_key);
    EVP_PKEY_free(their_key);
    throw std::runtime_error("key derivation failed");
  }

  EVP_PKEY_CTX_free(derive_ctx);
  EVP_PKEY_free(our_key);
  EVP_PKEY_free(their_key);

  // --- step 6: hash the shared secret to get a clean 32-byte AES key ---
  // SHA-256 gives us exactly 32 bytes from whatever the curve output is
  SHA256(raw_shared_secret.data(), secret_len, out_key);

  std::cout << "[*] key exchange complete\n";
}
