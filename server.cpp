#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <string>
#include <queue>
#include <vector>
#include "new.pb.h"

#define PORT 8080
#define MAX_CLIENTS 5
#define BUFFER_SIZE 1024

using namespace std;
using namespace google::protobuf;

struct user {
  string name;
  int socket;
  string ip;
  string status;
};

static int numConnections = 0;

static queue<pthread_t> pool;
static vector<user> users;
pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;

void printUsers() {
  for(auto usr : users) {
    char myArray[usr.name.size() + 1]; //as 1 char space for null is also required
    strcpy(myArray, usr.name.c_str());
    printf("Name: %s\tSocket: %d\n", myArray, usr.socket);
  }
}

void removeUser (string name, int socket) {
  printUsers();
  for (int i = 0 ; i < users.size(); i++) {
    user usr = users.at(i);
    if (usr.name == name && usr.socket == socket) {
      users.erase(users.begin() + i);
      break;
    }
  }
  printUsers();
}

bool usernameAvailable(char username[]) {
  for (auto usr : users) {
    if (usr.name == username) { //TODO IP
      return false;
    }
  }
  return true;
}

void broadcast(string message, int senderSocket, string from) {
  chat::ServerResponse *response = new chat::ServerResponse();
  chat::MessageCommunication *responseMessage = new chat::MessageCommunication();
  response->set_option(4);
  response->set_code(200);
  responseMessage->set_message(message);
  responseMessage->set_recipient("everyone");
  responseMessage->set_sender(from);

  response->set_allocated_messagecommunication(responseMessage);
  string responseSerialized;
  response->SerializeToString(&responseSerialized);
  char tempBuffer[BUFFER_SIZE] = {0};
  strcpy(tempBuffer, responseSerialized.c_str());
  for(auto usr: users) {
    if (usr.socket != 0 && usr.socket != senderSocket) {
      send(usr.socket, tempBuffer, responseSerialized.size() + 1, 0);
    }
  }
}

int sendTo(string user, string message, string from) {
  chat::ServerResponse *response = new chat::ServerResponse();
  chat::MessageCommunication *responseMessage = new chat::MessageCommunication();
  response->set_option(4);
  response->set_code(200);
  responseMessage->set_message(message);
  responseMessage->set_recipient(user);
  responseMessage->set_sender(from);
  response->set_allocated_messagecommunication(responseMessage);
  string responseSerialized;
  response->SerializeToString(&responseSerialized);
  char tempBuffer[BUFFER_SIZE] = {0};
  strcpy(tempBuffer, responseSerialized.c_str());
  for( auto usr: users) {
    if (usr.name == user && usr.socket != 0) {
      send(usr.socket, tempBuffer, responseSerialized.size()+1, 0);
      return 1;
    }
  }
  return 0;
}

int sendErrorTo(int socket, string user, string message, string from) {
  chat::ServerResponse *response = new chat::ServerResponse();
  chat::MessageCommunication *responseMessage = new chat::MessageCommunication();
  response->set_option(4);
  response->set_code(500);
  response->set_servermessage("Usuario ya existe con ese nombre");
  responseMessage->set_message(message);
  responseMessage->set_recipient(user);
  responseMessage->set_sender(from);
  response->set_allocated_messagecommunication(responseMessage);
  string responseSerialized;
  response->SerializeToString(&responseSerialized);
  char tempBuffer[BUFFER_SIZE] = {0};
  strcpy(tempBuffer, responseSerialized.c_str());
  send(socket, tempBuffer, responseSerialized.size()+1, 0);
  return 1;
}

void* threadFun( void *arg) {
  pthread_t thId = pthread_self();
  int new_socket = *((int *)(&arg));
  char tempBuffer[BUFFER_SIZE] = {0};
  int tempValue = read(new_socket, tempBuffer, BUFFER_SIZE);
  chat::ClientPetition tempPetition;
  string tempBufferStr(tempBuffer);
  tempPetition.ParseFromString(tempBufferStr);
  string firstDebug = tempPetition.DebugString();

  char debugChar[firstDebug.size() + 1];
  strcpy(debugChar, firstDebug.c_str());

  printf("debug: %s\n", debugChar);
  if (tempPetition.option() == 1) {
    user newUser;
    newUser.name = tempPetition.mutable_registration()->username();
    newUser.socket = new_socket;
    newUser.ip = tempPetition.mutable_registration()->ip();
    newUser.status = "Disponible";
    // De string a char[]
    char newUserName[newUser.name.length() + 1];
    strcpy(newUserName, newUser.name.c_str());

    // Rechazar la conexi??n si el usuario ya existe
    if (!usernameAvailable(newUserName)){
      removeUser(newUserName, new_socket); //TODO REVISAR ESTE ORDEN
      string err = "ERROR - USUARIO EXISTENTE";

      char errChar[err.size() + 1];
      strcpy(errChar, err.c_str());

      string from = "SERVIDOR";
      sendErrorTo(new_socket, newUser.name, errChar, from);
      pthread_mutex_lock(&mutex1);
      numConnections--;
      pool.push(thId);
      pthread_mutex_unlock(&mutex1);
      printf("Closing socket %d...\n", new_socket);
      close(new_socket);
      printf("Socket %d gone\n", new_socket);
      pthread_exit(NULL);
    }

    pthread_mutex_lock(&mutex1);
    users.push_back(newUser);
    pthread_mutex_unlock(&mutex1);
    char buffer[BUFFER_SIZE] = {0};
    for (;;) {
      int value = read(new_socket, buffer, BUFFER_SIZE);
      if (buffer[0] == 0) {
        break;
      }

      chat::ClientPetition request;
      string bufferStr(buffer);
      request.ParseFromString(bufferStr);
      string debug = request.DebugString();
      char debugChar2[debug.size() + 1];
      strcpy(debugChar2, debug.c_str());
      printf("%s\n", debugChar2);
      int option = request.option();
      printf("RECEIVED OPTION: %u\n", option);

      // option 1: Registro de Usuarios
      // option 2: Usuarios Conectados
      // option 3: Cambio de Estado
      // option 4: Mensajes
      // option 5: Informacion de un usuario en particular
      switch (option) {
        case 1:{
          printf("OPTION 1\n");
          // Se supone que esta ya nunca se ejecutar?? porque ya se ejecut?? antes.
          break;
        }
        case 2:{
          printf("OPTION 2\n");
          chat::ServerResponse *response = new chat::ServerResponse();
          response->set_option(2);
          response->set_code(200);
          response->set_servermessage("Llega lista de usuarios");
          chat::ConnectedUsersResponse *connectedUsers = new chat::ConnectedUsersResponse();
          for(auto usr : users) {
            chat::UserInfo *usrInfo = connectedUsers->add_connectedusers();
            usrInfo->set_username(usr.name);
            usrInfo->set_status(usr.status);
            usrInfo->set_ip(usr.ip);
          }
          response->set_allocated_connectedusers(connectedUsers);
          string responseSerialized;
          response->SerializeToString(&responseSerialized);
          strcpy(buffer, responseSerialized.c_str());
          send(new_socket, buffer, responseSerialized.size() + 1, 0);
          break;
        }
        case 3:{
          printf("OPTION 3\n");
          string username = request.mutable_change()->username();
          string status = request.mutable_change()->status();
          for (int i = 0 ; i < users.size(); i++){
            
          // for(auto usr : users) {
            if (users.at(i).name == username) {
              pthread_mutex_lock(&mutex1);
              users.at(i).status = status;
              pthread_mutex_unlock(&mutex1);
            }
          }
          chat::ServerResponse *response = new chat::ServerResponse();
          response->set_option(3);
          response->set_code(200);
          response->set_servermessage("Cambio de estado exitoso");
          string responseSerialized;
          response->SerializeToString(&responseSerialized);
          strcpy(buffer, responseSerialized.c_str());
          send(new_socket, buffer, responseSerialized.size() + 1, 0);
          break;
        }
        case 4:{
          string recipient = request.mutable_messagecommunication()->recipient();
          string message = request.mutable_messagecommunication()->message();
          string sender = request.mutable_messagecommunication()->sender();
          // message = sender + " para " + recipient + ": " + message;
          if(recipient=="everyone"){
            broadcast(message, new_socket, sender);
          } else {
            int indicator = sendTo(recipient, message, sender);
            if (indicator == 0) {
              chat::ServerResponse *response = new chat::ServerResponse();
              response->set_code(500);
              response->set_servermessage("Usuario inexistente");
              string responseSerialized;
              response->SerializeToString(&responseSerialized);
              char tempBuffer[BUFFER_SIZE] = {0};
              strcpy(tempBuffer, responseSerialized.c_str());
              send(new_socket, tempBuffer, responseSerialized.size() + 1, 0);
            }
          }
          break;
        }
        case 5:{
          string user = request.mutable_users()->user();
          chat::UserInfo *usrInfo = new chat::UserInfo();
          chat::ServerResponse *response = new chat::ServerResponse();
          response->set_option(5);
          response->set_code(200);
          usrInfo->set_username(user);
          for (auto usr : users) {
            if(usr.name == user) {
              usrInfo->set_status(usr.status);
              usrInfo->set_ip(usr.ip);
              break;
            }
          }
          response->set_allocated_userinforesponse(usrInfo);
          response->set_servermessage("Todo bien");
          string responseSerialized;
          response->SerializeToString(&responseSerialized);
          strcpy(buffer, responseSerialized.c_str());
          send(new_socket, buffer, responseSerialized.size() + 1, 0);
          break;
        }
      }

      // broadcast(buffer, newUser.socket, newUser.name);
      // Clear buffer
      memset(buffer, 0, BUFFER_SIZE);
    }
    pthread_mutex_lock(&mutex1);
    numConnections--;

    removeUser(newUserName, new_socket);
    pool.push(thId);
    pthread_mutex_unlock(&mutex1);
    printf("Closing socket %d...\n", new_socket);
    close(new_socket);
    printf("Socket %d gone\n", new_socket);
    pthread_exit(NULL);
  } else {
    pthread_mutex_lock(&mutex1);
    numConnections--;
    pool.push(thId);
    pthread_mutex_unlock(&mutex1);
    printf("Closing socket %d...\n", new_socket);
    close(new_socket);
    printf("Socket %d gone\n", new_socket);
    pthread_exit(NULL);
  }
}

int main(int argc, char const *argv[]) {
  GOOGLE_PROTOBUF_VERIFY_VERSION;
  chat::ClientPetition client_petition;
  if (argc <= 1) {
    printf("No port specified\n");
    return -1;
  }
  int port = atoi(argv[1]);
  printf("%d\n", port );
  int server_fd, new_socket, valread;
  struct sockaddr_in address;
  int opt = 1;
  int addrlen = sizeof(address);
  char buffer[BUFFER_SIZE] = {0};

  // Creating socket file descriptor
  if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
    perror("socket failed");
    exit(EXIT_FAILURE);
  }

  // Forcefully attaching socket to the port 8080
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
    perror("setsockopt");
    exit(EXIT_FAILURE);
  }

  // Thread pool
  // pthread_t pool[MAX_CLIENTS];
  pthread_mutex_lock(&mutex1);
    for (int i = 0 ; i < MAX_CLIENTS; i++){
      pthread_t temp;
      pool.push(temp);
    }
  pthread_mutex_unlock(&mutex1);
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(PORT);

  // Forcefully attaching socket to the port 8080
  if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
    perror("bind failed");
    exit(EXIT_FAILURE);
  }

  while (true) {
    if (listen(server_fd, MAX_CLIENTS) < 0) {
      perror("listen");
      exit(EXIT_FAILURE);
    }
    if (numConnections <= MAX_CLIENTS) {
      new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen);
      if (new_socket < 0) {
        perror("accept");
        exit(EXIT_FAILURE);
      }
      pthread_mutex_lock(&mutex1);
      pthread_t threadId = pool.front();
      pool.pop();
      pthread_mutex_unlock(&mutex1);
      int ret = pthread_create(&threadId, NULL, &threadFun, (int *)new_socket );
      // printf("THREAD[0] ID: %ld\n", threadId);
      if(ret!=0) {
        printf("Error: pthread_create() failed\n");
        exit(EXIT_FAILURE);
      }
      pthread_mutex_lock(&mutex1);
      numConnections++;
      pthread_mutex_unlock(&mutex1);
    }
  }

  // for(int i = 0 ; i < MAX_CLIENTS; i++) {
  //   pthread_join(pool[i], NULL);
  // }
  while(!pool.empty()) {
    pthread_join(pool.front(), NULL);
    pool.pop();
  }
  pthread_mutex_destroy(&mutex1);
  pthread_exit(NULL);

  ShutdownProtobufLibrary();

}
