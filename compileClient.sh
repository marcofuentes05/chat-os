#! /bin/sh
g++ client.cpp -o client -lpthread
echo "COMPILED"
./client marco 127.0.0.1 8080