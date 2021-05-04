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
// #include <commons.h>
#define PORT 8080
#define MAX_CLIENTS 2

void* threadFun( void *arg) {
  printf("ENTERING THREAD\n");
  char buffer[1024] = {0};
  int new_socket = *((int *)(&arg));
  int value = read(new_socket, buffer, 1024);
  char hello[] = "Hello from server";
  printf("%s\n", buffer);
  send(new_socket, hello, strlen(hello), 0);
  printf("Hello message sent\n");

  printf("EXITING THREAD\n");
  pthread_exit(NULL);
  return NULL;
}

int main(int argc, char const *argv[]) {
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
  char buffer[1024] = {0};

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
  pthread_t pool[MAX_CLIENTS];
  int numConnections = 0;
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(PORT);

  // Forcefully attaching socket to the port 8080
  if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
    perror("bind failed");
    exit(EXIT_FAILURE);
  }

  while (numConnections < MAX_CLIENTS) {
    if (listen(server_fd, MAX_CLIENTS) < 0) {
      perror("listen");
      exit(EXIT_FAILURE);
    }
    new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen);
    if (new_socket < 0) {
      perror("accept");
      exit(EXIT_FAILURE);
    }
    int ret = pthread_create(&pool[0], NULL, &threadFun, (int *)new_socket );
    if(ret!=0) {
      printf("Error: pthread_create() failed\n");
      exit(EXIT_FAILURE);
    }
    numConnections++;
  }

  // for(int i = 0 ; i < MAX_CLIENTS; i++) {
  //   pthread_join(pool[i], NULL);
  // }
  
  // valread = read(new_socket, buffer, 1024);
  // printf("%s\n", buffer);
  // send(new_socket, hello, strlen(hello), 0);
  // printf("Hello message sent\n");

  return 0;
}