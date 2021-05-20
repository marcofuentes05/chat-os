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

//Colors
#define ANSI_COLOR_RED "\x1b[31m"
#define ANSI_COLOR_CYAN "\x1b[36m"
#define ANSI_COLOR_GREEN "\x1b[32m"
#define ANSI_COLOR_RESET "\x1b[0m"

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
    printf(ANSI_COLOR_RESET"" ANSI_COLOR_RESET);
    getline(cin, userInput);
    char firtschar = userInput[0];
    chat::MessageCommunication *message = new chat::MessageCommunication(); 
    if(firtschar == '/'){
      string command = userInput.substr(0, userInput.find(" "));
      if(command == "/help"){
        printf(ANSI_COLOR_GREEN"Los comandos disponibles son:\n/changestate <estado>: para cambiar a los estados activo, inactivo, ocupado\n/users: para obtener todos los usuarios conectados\n/userinfo <user>: para obtener la informacion de un usuario\n@<nombre de usuario> para enviar un mensaje directo\n/help para imprimir este menu\n " ANSI_COLOR_RESET);
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
        printf(ANSI_COLOR_RED"COMANDO INVALIDO\n " ANSI_COLOR_RESET);
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
    bzero(buffer, LENGTH);
  }
}
//Esta funcion maneja la recepcion de mensajes del servidor
void* recieve_msg_handler(void* arg){
  char tempBuffer[LENGTH] = {};
  while(1){
      int receive = recv(sock, tempBuffer,LENGTH,0);
      if(receive > 0){
        string tempmessage(tempBuffer);
        chat::ServerResponse response;
        
        //parse the server message
        response.ParseFromString(tempmessage);
        int option = response.option();

        if(option == 1){
          if(response.code()==500){
            //Changing the string to something pritable for printf 
            string firstservermessage = response.servermessage();
            char serverResponse[firstservermessage.size() + 1];
            strcpy(serverResponse, firstservermessage.c_str());

            printf(ANSI_COLOR_RED"ERROR DE SERVIDOR\n" ANSI_COLOR_RESET);
            printf(ANSI_COLOR_RED"%s\n" ANSI_COLOR_RESET, serverResponse);
          }
        } else if(option==2){
          if(response.code()==200){
            chat::ConnectedUsersResponse usersInfo = response.connectedusers();
            for(int i = 0; i < usersInfo.connectedusers_size(); i++){
              const chat::UserInfo user = usersInfo.connectedusers(i);
              //Changing the data to something pritable for printf 
              string firstusername = user.username();
              char username[firstusername.size() + 1];
              strcpy(username, firstusername.c_str());

              string firststatus = user.status();
              char status[firststatus.size() + 1];
              strcpy(status, firststatus.c_str());

              string firstip = user.ip();
              char ip[firstip.size() + 1];
              strcpy(ip, firstip.c_str());
              
              printf(ANSI_COLOR_GREEN"%s  %s  %s\n " ANSI_COLOR_RESET, username, status, ip);
            }
          } else {
            //Changing the message to something printable for printf()
            string firstservermessage = response.servermessage();
            char serverResponse[firstservermessage.size() + 1];
            strcpy(serverResponse, firstservermessage.c_str());
            
            printf(ANSI_COLOR_RED"Error del servidor\n " ANSI_COLOR_RESET);
            printf("%s\n", serverResponse);
          }
        } else if(option==3){

          string firstservermessage = response.servermessage();
          char serverResponse[firstservermessage.size() + 1];
          strcpy(serverResponse, firstservermessage.c_str());

          if(response.code()==200){
            printf(ANSI_COLOR_GREEN"%s\n " ANSI_COLOR_RESET, serverResponse);
          }
          else{
            printf(ANSI_COLOR_RED"Error del servidor\n " ANSI_COLOR_RESET);
            printf(ANSI_COLOR_RED"%s\n " ANSI_COLOR_RESET, serverResponse);
          }
        } else if(option==4){
            if(response.code()==200){
            chat::MessageCommunication message = response.messagecommunication();
            
            //Changing the data for somethig printable for printf
            string firstsender = message.sender();
            char sender[firstsender.size() + 1];
            strcpy(sender, firstsender.c_str());

            string firstmessage = message.message();
            char data[firstmessage.size() + 1];
            strcpy(data, firstmessage.c_str());

            if(message.recipient()=="everyone"){
              printf(ANSI_COLOR_GREEN"%s: %s\n " ANSI_COLOR_RESET,sender,data);
            } else {
              printf(ANSI_COLOR_CYAN"@%s: %s\n " ANSI_COLOR_RESET, sender,data);
            }
          } else {
            string firstservermessage = response.servermessage();
            char serverResponse[firstservermessage.size() + 1];
            strcpy(serverResponse, firstservermessage.c_str());

            printf(ANSI_COLOR_RED"Error del servidor\n " ANSI_COLOR_RESET);
            printf(ANSI_COLOR_RED"%s\n " ANSI_COLOR_RESET, serverResponse);
          }
        } else if(option == 5){
          if(response.code()==200){
            chat::UserInfo userInfo = response.userinforesponse();
            
            //Changing the data for something usable por printf
            string firstusername = userInfo.username();
            char username[firstusername.size() + 1];
            strcpy(username, firstusername.c_str());

            string firststatus = userInfo.status();
            char status[firststatus.size() + 1];
            strcpy(status, firststatus.c_str());

            string firstip = userInfo.ip();
            char ip[firstip.size() + 1];
            strcpy(ip, firstip.c_str());

            printf(ANSI_COLOR_GREEN"%s  %s  %s\n " ANSI_COLOR_RESET, username, status, ip);
          } else {
            string firstservermessage = response.servermessage();
            char serverResponse[firstservermessage.size() + 1];
            strcpy(serverResponse, firstservermessage.c_str());
            printf(ANSI_COLOR_RED"Error del servidor\n " ANSI_COLOR_RESET);
            printf(ANSI_COLOR_RED"%s\n " ANSI_COLOR_RESET, serverResponse);
          }
        } else {
          
          printf(ANSI_COLOR_RED"SOMETHINGS WRONG I CAN FEEL IT\n " ANSI_COLOR_RESET);
        }
        tempmessage = "";
      } else if (receive == 0){
        break;
      }
      else{
        printf(ANSI_COLOR_RED"ERROR RECEIVING DATA FROM SERVER\n " ANSI_COLOR_RESET);
      }
      
      memset(tempBuffer, 0, sizeof(tempBuffer));
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

  printf("-------------------------\n");
  printf("    Bienvenido al Chat   \n");
  printf("-------------------------\n");
  printf("\n");
  printf("Los mensajes generales se imprimiran en este color\n");
  printf(ANSI_COLOR_CYAN"Los mensajes directos se imprimiran en este color\n " ANSI_COLOR_RESET);
  printf(ANSI_COLOR_GREEN"Los mensajes provenientes del servidor se imprimiran en este color\n " ANSI_COLOR_RESET);
  printf(ANSI_COLOR_RED"Los errores del servidor se imprimiran en este color\n " ANSI_COLOR_RESET);
  printf("\n");
  printf(ANSI_COLOR_GREEN"Los comandos disponibles son:\n/changestate <estado>: para cambiar a los estados activo, inactivo, ocupado\n/users: para obtener todos los usuarios conectados\n/userinfo <user>: para obtener la informacion de un usuario\n@<nombre de usuario> para enviar un mensaje directo\n/help para imprimir este menu\n " ANSI_COLOR_RESET);

  
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