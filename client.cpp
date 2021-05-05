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
#define LENGTH 2048

char* name;
char addr[] = "18.116.36.10";
int sock = 0, valread;

//Esta funciona manejara el envio de mensajes al servidor
void* send_msg_handler(void* arg){
  char message[LENGTH] = {};
  char buffer[LENGTH + 32] = {};

  while(1){
    fgets(message, LENGTH, stdin);
    sprintf(buffer, "%s: %s\n", name, message);
    send(sock, buffer, strlen(buffer),0);
    bzero(message, LENGTH);
    bzero(buffer, LENGTH + 32);
  }
}

int main(int argc, char *argv[]) {
  if (argc <= 3) {
    printf("Lacking info\n");
    return -1;
  }
  char* userName = argv[1];
  name = userName;
  char* serverIP = argv[2];
  int serverPort = atoi(argv[3]);
  // char message[50] = "Hello from client, my name is: ";
  // printf("CONNECTED TO: %s\nON PORT: %d\nWELCOME %s\n",serverIP, serverPort, userName);

  struct sockaddr_in serv_addr;
  // char *hello = "Hello from client, my name is: ";
  // strcat(message, hello);
  //strcat(message, userName);
  //char buffer[1024] = {0};
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
  
  // send(sock, message, strlen(message), 0);
  // printf("Hello message sent\n");
  // valread = read(sock, buffer, 1024);
  // printf("%s\n", buffer);
  // return 0;
  pthread_t send_msg_thread;
  int send_msg_thread_success; 
  send_msg_thread_success = pthread_create(&send_msg_thread, NULL, send_msg_handler, NULL);
  if(send_msg_thread_success != 0){
    printf("ERROR: send_msg thread\n");
    return -1;
  }
  while(1){}
}