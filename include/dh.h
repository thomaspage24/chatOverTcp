#pragma once
#include <cstdint>

#include "crypto.h"  // for kAesKeySize

/**
 * Performs a Diffie-Hellman key exchange over the socket.
 *
 * Both sides call this after connecting. One side sends their
 * public value first (server), the other responds (client).
 * Both end up with the same shared secret.
 *
 * @param socket_fd   the connected socket
 * @param is_server   true if this side is the server, false if client
 * @param out_key     buffer to write the 32 byte shared key into
 */
void DhHandshake(int socket_fd, bool is_server, uint8_t out_key[kAesKeySize]);

// ## step 2 — dh.cpp

// openssl has a built in DH implementation. the steps are:
// ```
// 1. both sides load the same prime p and generator g
// 2. generate a random private key
// 3. compute the public value (g^private mod p)
// 4. send our public value to the other side
// 5. receive their public value
// 6. compute the shared secret (their_public^our_private mod p)
// 7. hash the shared secret down to 32 bytes for use as AES key