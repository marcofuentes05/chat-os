#! /bin/sh
g++ server.cpp -o server -lpthread
echo "\nCOMPILED\n"
./server 8080