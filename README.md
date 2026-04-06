# secure_chat

Encrypted peer-to-peer chat over TCP using AES and Diffie-Hellman key exchange.

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

## Structure

```
secure_chat/
├── Makefile
├── include/
│   ├── server.h
│   ├── client.h
│   └── connection.h
└── src/
    ├── main.cpp         ← entry point, parses "server" / "client" mode
    ├── server.cpp       ← bind, listen, accept
    ├── client.cpp       ← resolve hostname, connect
    └── connection.cpp   ← send/recv logic, threading
```

## Usage

```bash
make
./output/main server <port>
./output/main client <host> <port>
```

## OpenSSL
[openssl in cpp tutorial](https://cppscripts.com/openssl-in-cpp)
