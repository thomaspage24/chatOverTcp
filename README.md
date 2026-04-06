# secure_chat

Encrypted peer-to-peer chat over TCP using AES and Elliptic Curve Diffie-Hellman key exchange.

## Architecture

```
[ Terminal A ]                    [ Terminal B ]
  server mode                       client mode
      |                                  |
      |——— TCP socket ———————————————————|
      |                                  |
  AES encrypt                       AES decrypt
  AES decrypt                       AES encrypt
      |                                  |
  DH key exchange ——— shared secret ———  |
```
## Usage

```bash
make
./output/main server <port>
./output/main client <host> <port>
```

## OpenSSL
[openssl in cpp documentation](https://cppscripts.com/openssl-in-cpp)
# TODO

Make containers? Docker
test over wifi and 2 different machines, rather than talking to myself machine 