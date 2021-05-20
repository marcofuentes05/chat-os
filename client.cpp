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
    getline(cin, userInput);
    char firtschar = userInput[0];
    chat::MessageCommunication *message = new chat::MessageCommunication(); 
    if(firtschar == '/'){
      string command = userInput.substr(0, userInput.find(" "));
      cout<< command;
      if(command == "/help"){
        printf("Los comandos disponibles son:\n/changestate <estado>: para cambiar a los estados activo, inactivo, ocupado\n/users: para obtener todos los usuarios conectados\n/userinfo <user>: para obtener la informacion de un usuario\n@<nombre de usuario> para enviar un mensaje directo\n/help para imprimir este menu\n");
      }else if(command == "/changestate"){
        //fill the set status protocol
        chat::ChangeStatus *changestatus = new chat::ChangeStatus();
        changestatus->set_username(name);
        string data = userInput.substr(userInput.find(" ")+1, userInput.size());
        changestatus->set_status(data);

        //fill the client petition protocol
        chat::ClientPetition *petition = new chat::ClientPetition();
        petition->set_option(3);
        petition->set_allocated_change(changestatus);
        petition->SerializeToString(&petitionserializer);
        strcpy(buffer,petitionserializer.c_str());

        send(sock, buffer, petitionserializer.size() +1 ,0);

      }else if(command == "/users"){
        //fill the user request protocol
        chat::UserRequest *inforequest = new chat::UserRequest();
        inforequest->set_user("everyone");

        //fill the client petition protocol
        chat::ClientPetition *petition = new chat::ClientPetition();
        petition->set_option(2);
        petition->set_allocated_users(inforequest);
        petition->SerializeToString(&petitionserializer);
        strcpy(buffer,petitionserializer.c_str());

        send(sock, buffer, petitionserializer.size() +1 ,0);

      }else if(command == "/userinfo"){
        //fill the user request protocol
        chat::UserRequest *inforequest = new chat::UserRequest();
        string wantedUsr = userInput.substr(userInput.find(" ")+1, userInput.size());
        inforequest->set_user(wantedUsr);

        //fill the client petition protocol
        chat::ClientPetition *petition = new chat::ClientPetition();
        petition->set_option(2);
        petition->set_allocated_users(inforequest);
        petition->SerializeToString(&petitionserializer);
        strcpy(buffer,petitionserializer.c_str());

        send(sock, buffer, petitionserializer.size() +1 ,0);

      }else{
        printf("COMANDO INVALIDO\n");
      }
    }
    
    //Direct Message
    else if(firtschar == '@'){
      chat::ClientPetition *petition = new chat::ClientPetition();
      //obtain the recipient
      string recipient = userInput.substr(1, userInput.find(" ")-1);
      
      //Fill the message protocol
      string data = userInput.substr(userInput.find(" ")+1, userInput.size());
      message->set_message(data);
      message->set_sender(name);
      message->set_recipient(recipient);
      
      //Fill the client petition protocol
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

//main
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
  string petitionserializer;
  char regbuffer[LENGTH] = {};

  struct sockaddr_in serv_addr;
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

  //send the register petition
  chat::ClientPetition *petition = new chat::ClientPetition();
  chat::UserRegistration *reg = new chat::UserRegistration();
  reg->set_username(name);
  reg->set_ip(serverIP);

  petition->set_option(1);
  petition->set_allocated_registration(reg);

  petition->SerializeToString(&petitionserializer);
  strcpy(regbuffer,petitionserializer.c_str());
  send(sock, regbuffer, petitionserializer.size() +1 ,0);


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