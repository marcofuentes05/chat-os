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

using namespace std;

static vector<int> vector;

void printQueue() {
  for(int i = 0 ; i < vector.size(); i++) {
    printf(" %d",vector.at(i));
  }
  printf("\n");
}

int main() {
  for(int i = 0 ; i < 10; i++) {
    que.push(i);
  }
  printQueue();
  que.erase(1);
  printQueue();
  return 0;
}