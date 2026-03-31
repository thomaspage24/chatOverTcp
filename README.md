[ Terminal A ]                    [ Terminal B ]
  server mode                       client mode
      |                                  |
      |——— TCP socket ———————————————————|
      |                                  |
  AES encrypt                       AES decrypt
  AES decrypt                       AES encrypt
      |                                  |
  DH key exchange ——— shared secret ———  |

# structure
  secure_chat/
├── CMakeLists.txt
└── src/
    ├── main.cpp          ← entry point, parses "server" / "client" mode
    ├── server.cpp/.h     ← bind, listen, accept
    ├── client.cpp/.h     ← resolve hostname, connect
    └── connection.cpp/.h ← all send/recv logic, threading