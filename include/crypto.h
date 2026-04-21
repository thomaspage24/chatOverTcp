#pragma once
#include <cstdint>
#include <string>
#include <vector>

// AES-256 needs 32 byte key
// AES block size is always 16 bytes
const int kAesKeySize = 32;
const int kAesIvSize = 16;

/**
 * Encrypts a plaintext string using AES-256-CBC
 *
 * Generates random IV for every call,
 * encrypting same msg twice gives different ciphertext
 *
 * @param plaintext the message to encrypt
 * @param key       32 byte aes key
 * @return          [16 byte IV | ciphertext] as raw bytes
 */
std::vector<uint8_t> Encrypt(const std::string& plaintext,
                             const uint8_t key[kAesKeySize]);

/**
 * Decrypts a message encrpyted with encrypt()
 *
 * @param iv_and_ciphertext raw bytes returned by encrypt
 * @param key               the 32 byte key used to encrypt
 * @return                  the original plaintext string
 */
std::string Decrypt(const std::vector<uint8_t>& iv_and_ciphertext,
                    const uint8_t key[kAesKeySize]);
