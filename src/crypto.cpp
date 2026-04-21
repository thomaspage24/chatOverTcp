#include "crypto.h"

#include <openssl/evp.h>   // EVP = envelope
#include <openssl/rand.h>  // RAND_bytes, cryptographically secure random byteswh

#include <stdexcept>

std::vector<uint8_t> Encrypt(const std::string& plaintext,
                             const uint8_t key[kAesKeySize]) {
  // --- generate a random IV (initialisation vector) ---
  // fresh IV every message — same plaintext encrypted twice = different
  // ciphertext
  uint8_t iv[kAesIvSize];
  if (RAND_bytes(iv, kAesIvSize) != 1)
    throw std::runtime_error("failed to generate random IV");

  // ---create an openssl context ---
  // EVP_CIPHER_CTX is the workspace that holds all encryption state
  // must be freed after use
  EVP_CIPHER_CTX* cipher_ctx = EVP_CIPHER_CTX_new();
  if (!cipher_ctx) throw std::runtime_error("failed to create cipher context");

  // ---set up AES-256-CBC encryption ---
  // EVP_aes_256_cbc() tells openssl which algorithm to use
  // nullptr = use default engine, key and iv are our 32 and 16 byte values
  if (EVP_EncryptInit_ex(cipher_ctx, EVP_aes_256_cbc(), nullptr, key, iv) !=
      1) {
    EVP_CIPHER_CTX_free(cipher_ctx);
    throw std::runtime_error("failed to init encryption");
  }

  // --- encrypt the plaintext ---
  // ciphertext can be slightly larger than plaintext because of padding
  // worst case: plaintext + one full extra block (16 bytes)
  std::vector<uint8_t> ciphertext(plaintext.size() + kAesIvSize);
  int chunk_size = 0;

  // EVP_EncryptUpdate does the actual encryption
  // you can call it multiple times for large inputs
  if (EVP_EncryptUpdate(cipher_ctx, ciphertext.data(), &chunk_size,
                        reinterpret_cast<const uint8_t*>(plaintext.data()),
                        static_cast<int>(plaintext.size())) != 1) {
    EVP_CIPHER_CTX_free(cipher_ctx);
    throw std::runtime_error("encryption failed");
  }

  int total_bytes = chunk_size;

  // EVP_EncryptFinal_ex flushes any remaining bytes and adds PKCS7 padding
  // always call this after EncryptUpdate
  if (EVP_EncryptFinal_ex(cipher_ctx, ciphertext.data() + total_bytes,
                          &chunk_size) != 1) {
    EVP_CIPHER_CTX_free(cipher_ctx);
    throw std::runtime_error("encryption finalise failed");
  }

  total_bytes += chunk_size;
  ciphertext.resize(total_bytes);  // trim buffer to actual size

  EVP_CIPHER_CTX_free(cipher_ctx);

  // ---prepend the IV ---
  // receiver needs the IV to decrypt
  // final layout: [ 16 byte IV | ciphertext ]
  std::vector<uint8_t> iv_and_ciphertext;
  iv_and_ciphertext.insert(iv_and_ciphertext.end(), iv, iv + kAesIvSize);
  iv_and_ciphertext.insert(iv_and_ciphertext.end(), ciphertext.begin(),
                           ciphertext.end());

  return iv_and_ciphertext;
}

std::string Decrypt(const std::vector<uint8_t>& iv_and_ciphertext,
                    const uint8_t key[kAesKeySize]) {
  // make sure there are enough bytes to even contain an IV
  if (iv_and_ciphertext.size() <= static_cast<size_t>(kAesIvSize))
    throw std::runtime_error("ciphertext too short to contain an IV");

  // --- step 1: peel off the IV from the front ---
  const uint8_t* iv = iv_and_ciphertext.data();
  const uint8_t* ciphertext = iv_and_ciphertext.data() + kAesIvSize;
  int ciphertext_len = static_cast<int>(iv_and_ciphertext.size() - kAesIvSize);

  // --- step 2: create context and set up for decryption ---
  EVP_CIPHER_CTX* cipher_ctx = EVP_CIPHER_CTX_new();
  if (!cipher_ctx) throw std::runtime_error("failed to create cipher context");

  // same algorithm, same key, same IV
  if (EVP_DecryptInit_ex(cipher_ctx, EVP_aes_256_cbc(), nullptr, key, iv) !=
      1) {
    EVP_CIPHER_CTX_free(cipher_ctx);
    throw std::runtime_error("failed to init decryption");
  }

  // --- step 3: decrypt ---
  std::vector<uint8_t> plaintext(ciphertext_len);
  int chunk_size = 0;

  if (EVP_DecryptUpdate(cipher_ctx, plaintext.data(), &chunk_size, ciphertext,
                        ciphertext_len) != 1) {
    EVP_CIPHER_CTX_free(cipher_ctx);
    throw std::runtime_error("decryption failed");
  }

  int total_bytes = chunk_size;

  // EVP_DecryptFinal_ex strips the padding and flushes remaining bytes
  // if the key is wrong this call will fail — thats how we detect a bad key
  if (EVP_DecryptFinal_ex(cipher_ctx, plaintext.data() + total_bytes,
                          &chunk_size) != 1) {
    EVP_CIPHER_CTX_free(cipher_ctx);
    throw std::runtime_error("decryption failed — wrong key?");
  }

  total_bytes += chunk_size;
  plaintext.resize(total_bytes);

  EVP_CIPHER_CTX_free(cipher_ctx);

  // convert raw bytes back to a readable string
  return std::string(plaintext.begin(), plaintext.end());
}