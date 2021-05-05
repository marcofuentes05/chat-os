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
// #include <commons.h>
#define PORT 8080
#define MAX_CLIENTS 2

using namespace std;

static int numConnections = 0;

static queue<pthread_t> pool;
static vector<int> sockets; // Podria ser un vector de struct para guardar username y socket fd

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void* threadFun( void *arg) {
  // printf("ENTERING THREAD\n");
  int new_socket = *((int *)(&arg));
  // printf("SOCKET INT: %d\n", new_socket);
  pthread_t thId = pthread_self();
  // printf("THREAD ID: %ld\n",thId);
  bool sigue = true;
  char buffer[1024] = {0};
  for (; ;) {
    int value = read(new_socket, buffer, 1024);
    if (buffer[0] == 0) {
      // sigue = false;
      break;
    }
    printf("Socket ID: %d\t%s\n", new_socket, buffer);
    // Clear buffer
    memset(buffer, 0, sizeof(buffer));
    
    char hello[] = "Server confirma de recibido";
    send(new_socket, hello, strlen(hello), 0);
    // printf("Hello message sent\n");
  }
  pthread_mutex_lock(&mutex);
  numConnections--;
  // printf("NUMCONNECTIONS--: %d\n", numConnections);
  pool.push(thId);
  pthread_mutex_unlock(&mutex);
  close(new_socket);
  // printf("EXITING THREAD\n");
  pthread_exit(NULL);
  // return NULL;
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