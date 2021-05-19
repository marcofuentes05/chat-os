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
// #include "petition.pb.h"

#define PORT 8080
#define MAX_CLIENTS 5
#define BUFFER_SIZE 1024

using namespace std;
struct user {
  string name;
  int socket;
};

static int numConnections = 0;

static queue<pthread_t> pool;
static vector<user> users;

void printUsers() {
  for(auto usr : users) {
    printf("Name: %s\tSocket: %d\n", usr.name, usr.socket);
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

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

bool usernameAvailable(char username[]) {
  for (auto usr : users) {
    if (usr.name == username) {
      return false;
    }
  }
  return true;
}

void broadcast(char message[], int senderSocket) {
  for(auto usr: users) {
    if (usr.socket != 0 && usr.socket != senderSocket) {
      send(usr.socket, message, strlen(message), 0);
    }
  }
}

int sendTo(string user, char message[]) {
  for( auto usr: users) {
    if (usr.name == user && usr.socket != 0) {
      send(usr.socket, message, strlen(message), 0);
      return 1;
    }
  }
  return 0;
}

void sendTo(int socket, char message[]) {
  send(socket, message, strlen(message), 0);
}

void* threadFun( void *arg) {
  pthread_t thId = pthread_self();
  int new_socket = *((int *)(&arg));
  user newUser;
  newUser.socket = new_socket;
  newUser.name = "test"; // del protocolo

  // De string a char[]
  char newUserName[newUser.name.length() + 1];
  strcpy(newUserName, newUser.name.c_str());

  // Rechazar la conexión si el usuario ya existe
  if (!usernameAvailable(newUserName)){
    removeUser(newUserName, new_socket);
    sendTo(new_socket, "ERROR - USUARIO EXISTENTE");
    pthread_mutex_lock(&mutex);
    numConnections--;
    pool.push(thId);
    pthread_mutex_unlock(&mutex);
    printf("Closing socket %d...\n", new_socket);
    close(new_socket);
    printf("Socket %d gone\n", new_socket);
    pthread_exit(NULL);
  }
  
  pthread_mutex_lock(&mutex);
  users.push_back(newUser);
  pthread_mutex_unlock(&mutex);
  char buffer[BUFFER_SIZE] = {0};
  for (;;) {
    int value = read(new_socket, buffer, BUFFER_SIZE);
    if (buffer[0] == 0) {
      break;
    }
    printf("Socket ID: %d\t%s\n", new_socket, buffer);
    broadcast(buffer, newUser.socket);
    // Clear buffer
    memset(buffer, 0, BUFFER_SIZE);
  }
  pthread_mutex_lock(&mutex);
  numConnections--;
  for(int i = 0 ; i < users.size(); i++) {
    if (users.at(i).socket == new_socket) {
      users.at(i).socket = 0;
    }
  }
  pool.push(thId);
  pthread_mutex_unlock(&mutex);
  printf("Closing socket %d...\n", new_socket);
  close(new_socket);
  printf("Socket %d gone\n", new_socket);
  pthread_exit(NULL);
}

int main(int argc, char const *argv[]) {
  // GOOGLE_PROTOBUF_VERIFY_VERSION;
  // chat::ClientPetition client_petition;
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
  pthread_mutex_lock(&mutex);
    for (int i = 0 ; i < MAX_CLIENTS; i++){
      pthread_t temp;
      pool.push(temp);
    }
  pthread_mutex_unlock(&mutex);
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
      pthread_mutex_lock(&mutex);
      pthread_t threadId = pool.front();
      pool.pop();
      pthread_mutex_unlock(&mutex);
      int ret = pthread_create(&threadId, NULL, &threadFun, (int *)new_socket );
      // printf("THREAD[0] ID: %ld\n", threadId);
      if(ret!=0) {
        printf("Error: pthread_create() failed\n");
        exit(EXIT_FAILURE);
      }
      pthread_mutex_lock(&mutex);
      numConnections++;
      pthread_mutex_unlock(&mutex);
    }
  }

  // for(int i = 0 ; i < MAX_CLIENTS; i++) {
  //   pthread_join(pool[i], NULL);
  // }
  while(!pool.empty()) {
    pthread_join(pool.front(), NULL);
    pool.pop();
  }
  pthread_mutex_destroy(&mutex);
  pthread_exit(NULL);
}
