#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <time.h>
#include <pthread.h>
#include <poll.h>

struct GardenerTask {
    int plot_i;
    int plot_j;
    int gardener_id;
    int working_time;
    int status;
};

struct Request {
    struct GardenerTask task;
    struct sockaddr_in client_address;
};

struct TaskResult {
    char buffer[1024];
    int size;
    struct sockaddr_in client_address;
};

struct FieldSize {
    int rows;
    int columns;
};

enum event_type { MAP, ACTION, META_INFO };

struct Event {
    char timestamp[26];
    char buffer[1024];
    enum event_type type;
};

#define EMPTY_PLOT_COEFFICIENT 2
#define BUFFER_SIZE 256

int createClientSocket(char *server_ip, int server_port);
void sendHandleRequest(int client_socket, struct GardenerTask task);
struct sockaddr_in getServerAddress(char *server_ip, int server_port);