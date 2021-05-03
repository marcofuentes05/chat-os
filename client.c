#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
// #include <commons.h>
#define PORT 8080

char *addr = "18.116.36.10";

int main(int argc, char *argv[]) {
  if (argc <= 3) {
    printf("Lacking info\n");
    return -1;
  }
  char* userName = argv[1];
  char* serverIP = argv[2];
  int serverPort = atoi(argv[3]);
  char message[37];
  printf("CONNECTED TO: %s\nON PORT: %d\nWELCOME %s\n",serverIP, serverPort, userName);

  int sock = 0, valread;
  struct sockaddr_in serv_addr;
  char *hello = "Hello from client, my name is: ";
  strcat(message, hello);
  strcat(message, userName);
  char buffer[1024] = {0};
  if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    printf("\n Socket creation error \n");
    return -1;
  }

  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(serverPort);

  // Convert IPv4 and IPv6 addresses from text to binary form
  if (inet_pton(AF_INET, serverIP, &serv_addr.sin_addr) <= 0) {
    printf("\nInvalid address/ Address not supported \n");
    return -1;
  }

  if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
    printf("\nConnection Failed \n");
    return -1;
  }
  
  send(sock, message, strlen(message), 0);
  printf("Hello message sent\n");
  valread = read(sock, buffer, 1024);
  printf("%s\n", buffer);
  return 0;
}