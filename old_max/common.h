#include <time.h>

struct GardenerTask {
    int plot_i;
    int plot_j;
    int gardener_id;
    int working_time;
    int status;
};

struct FieldSize {
    int rows;
    int columns;
};

enum event_type { MAP, ACTION, META_INFO, SERVER_INFO };

struct Event {
    char timestamp[26];
    char buffer[1024];
    enum event_type type;
};

struct Observer {
    int socket;
    int is_new;
    int is_active;
};

#define EMPTY_PLOT_COEFFICIENT 2
#define BUFFER_SIZE 256

int createClientSocket(char *server_ip, int server_port);
void sendHandleRequest(int client_socket, struct GardenerTask task);