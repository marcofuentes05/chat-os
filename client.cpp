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
#include "new.pb.h"
using namespace std;
using namespace google::protobuf;

#define PORT 8080
#define LENGTH 2048

char* name;
char addr[] = "18.116.36.10";
int sock = 0, valread;

//Esta funcion maneja el envio de mensajes al servidor
void* send_msg_handler(void* arg){
  GOOGLE_PROTOBUF_VERIFY_VERSION;
  string userInput;
  char buffer[LENGTH] = {};

  while(1){
    string petitionserializer;
    cin >> userInput;
    char firtschar = userInput[0];
    chat::MessageCommunication *message = new chat::MessageCommunication(); 
    //Direct Message
    if(firtschar == '@'){
      chat::ClientPetition *petition = new chat::ClientPetition();
      string recipient = userInput.substr(1, userInput.find(" "));
      message->set_message(userInput);
      message->set_sender(name);
      message->set_recipient(recipient);
      petition->set_option(4);
      petition->set_allocated_messagecommunication(message);
      petition->SerializeToString(&petitionserializer);
      strcpy(buffer,petitionserializer.c_str());
      send(sock, buffer, petitionserializer.size() +1 ,0);
    }
    //Broadcast
    else{ 
      chat::ClientPetition *petition = new chat::ClientPetition();
      message->set_message(userInput);
      message->set_sender(name);
      message->set_recipient("everyone");
      petition->set_option(4);
      petition->set_allocated_messagecommunication(message);
      petition->SerializeToString(&petitionserializer);
      strcpy(buffer,petitionserializer.c_str());
      send(sock, buffer, petitionserializer.size() +1 ,0);
    }
  }
}
//Esta funcion maneja la recepcion de mensajes del servidor
void* recieve_msg_handler(void* arg){
  char message[LENGTH] = {};
  while(1){
      int receive = recv(sock, message,LENGTH,0);
      if(receive > 0){
        printf("%s", message);
      } else if (receive == 0){
        break;
      }
      else{
        printf("ERROR RECEIVING DATA FROM SERVER\n");
      }
      memset(message, 0, sizeof(message));
  }
}

int main(int argc, char *argv[]) {
  if (argc <= 3) {
    printf("Lacking info\n");
    return -1;
  }
  GOOGLE_PROTOBUF_VERIFY_VERSION;
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

  pthread_t recieve_msg_thread;
  int recieve_msg_thread_success;
  recieve_msg_thread_success = pthread_create(&recieve_msg_thread, NULL, recieve_msg_handler, NULL);
  if(recieve_msg_thread_success != 0){
    printf("ERROR: recieve_msg thread\n");
  }

  while(1){}
}