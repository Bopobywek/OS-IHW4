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
#include "common.h"

const char *shared_object = "/posix-shared-object";
const char *sem_shared_object = "/posix-sem-shared-object";
int pipe_fd[2];
int fg_pipe_fd[2];
int sg_pipe_fd[2];
int results_pipe_fd[2];

int createServerSocket(in_addr_t sin_addr, int port) {
    int server_socket;
    struct sockaddr_in server_address;

    if ((server_socket = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
        perror("Unable to create server socket");
        exit(-1);
    }

    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);
    server_address.sin_port = htons(port);

    if (bind(server_socket, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
        perror("Unable to bind address");
        exit(-1);
    }

    return server_socket;
}

int createMulticastSenderSocket(in_addr_t sin_addr, int port, int multicast_tll) {
    int multicast_socket;
    struct sockaddr_in multicast_address;

    if ((multicast_socket = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
        perror("Unable to create multicast socket");
        exit(-1);
    }

    if (setsockopt(multicast_socket, IPPROTO_IP, IP_MULTICAST_TTL, (void *)&multicast_tll,
                   sizeof(multicast_tll)) < 0) {
        perror("Unable to create multicast socket");
        exit(-1);
    }

    memset(&multicast_address, 0, sizeof(multicast_address));
    multicast_address.sin_family = AF_INET;
    multicast_address.sin_addr.s_addr = sin_addr;
    multicast_address.sin_port = htons(port);

    return multicast_socket;
}

void printField(int *field, int columns, int rows) {
    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < columns; ++j) {
            if (field[i * columns + j] < 0) {
                printf("X ");
            } else {
                printf("%d ", field[i * columns + j]);
            }
        }
        printf("\n");
    }

    fflush(stdout);
}

void sprintField(char *buffer, int *field, int columns, int rows) {
    int offset = 0;
    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < columns; ++j) {
            if (field[i * columns + j] < 0) {
                offset += sprintf(buffer + offset, "X ");
            } else {
                offset += sprintf(buffer + offset, "%d ", field[i * columns + j]);
            }
        }
        offset += sprintf(buffer + offset, "\n");
    }
}

void setEventWithCurrentTime(struct Event *event) {
    time_t timer;
    struct tm *tm_info;
    timer = time(NULL);
    tm_info = localtime(&timer);
    strftime(event->timestamp, sizeof(event->timestamp), "%Y-%m-%d %H:%M:%S", tm_info);
}

void writeEventToPipe(struct Event *event) {
    if (write(pipe_fd[1], event, sizeof(*event)) < 0) {
        perror("Can't write to pipe");
        exit(-1);
    }
}

void handleGardenPlot(sem_t *semaphores, int *field, int columns, struct GardenerTask task) {
    sem_wait(semaphores + (task.plot_i / 2 * (columns / 2) + task.plot_j / 2));

    struct Event gardener_event;
    setEventWithCurrentTime(&gardener_event);
    gardener_event.type = ACTION;
    sprintf(gardener_event.buffer, "Gardener %d takes (row: %d, col: %d) plot\n", task.gardener_id,
            task.plot_i, task.plot_j);
    writeEventToPipe(&gardener_event);

    if (field[task.plot_i * columns + task.plot_j] == 0) {
        field[task.plot_i * columns + task.plot_j] = task.gardener_id;
        usleep(task.working_time * 1000);
    } else {
        usleep(task.working_time / EMPTY_PLOT_COEFFICIENT * 1000);
    }

    struct Event event;
    setEventWithCurrentTime(&event);
    sprintf(event.buffer, "\n");
    sprintField(event.buffer + 1, field, columns, columns);
    event.type = MAP;
    writeEventToPipe(&event);

    sem_post(semaphores + (task.plot_i / 2 * (columns / 2) + task.plot_j / 2));
}

struct HandleArgs {
    int server_socket;
    sem_t *semaphores;
    int *field;
    struct FieldSize field_size;
    int id;
    int *pipe_fd;
};

struct Response {
    int status;
    struct sockaddr_in client_address;
};

int writeToBuffer(void *object, char *buffer, int size) {
    int i = 0;
    for (; i < size; ++i) {
        buffer[i] = *((char *)(object) + i);
    }

    return i;
}

void *gardenerHandler(void *args) {
    struct HandleArgs params = *((struct HandleArgs *)args);
    while (1) {
        struct Request request;

        if (read(params.pipe_fd[0], &request, sizeof(request)) < 0) {
            perror("Can't read from pipe");
        }

        int status = 1;
        struct TaskResult result;
        if (request.task.status == 2) {
            result.size =
                writeToBuffer(&params.field_size, result.buffer, sizeof(params.field_size));
            result.client_address = request.client_address;

            write(results_pipe_fd[1], &result, sizeof(result));
        } else if (request.task.status == 0) {
            handleGardenPlot(params.semaphores, params.field, params.field_size.columns,
                             request.task);
            result.size = writeToBuffer(&status, result.buffer, sizeof(status));
            result.client_address = request.client_address;
            write(results_pipe_fd[1], &result, sizeof(result));
        } else if (request.task.status == 1) {
            struct Event finish_event;
            setEventWithCurrentTime(&finish_event);
            finish_event.type = ACTION;
            sprintf(finish_event.buffer, "Gardener %d finish work\n", request.task.gardener_id);
            writeEventToPipe(&finish_event);

            result.size = writeToBuffer(&status, result.buffer, sizeof(status));
            result.client_address = request.client_address;
            write(results_pipe_fd[1], &result, sizeof(result));
        }
    }
}

pthread_t grardeners_threads[2];
void runGardener(struct HandleArgs *args) {
    pthread_create(grardeners_threads + args->id, NULL, gardenerHandler, args);
}

int *getField(int field_size) {
    int *field;
    int shmid;

    if ((shmid = shm_open(shared_object, O_CREAT | O_RDWR, 0666)) < 0) {
        perror("Can't connect to shared memory");
        exit(-1);
    } else {
        if (ftruncate(shmid, field_size * sizeof(int)) < 0) {
            perror("Can't resize shared memory");
            exit(-1);
        }
        if ((field = mmap(0, field_size * sizeof(int), PROT_WRITE | PROT_READ, MAP_SHARED, shmid,
                          0)) < 0) {
            printf("Can't connect to shared memory\n");
            exit(-1);
        };
        printf("Open shared Memory\n");
    }

    return field;
}

void initializeField(int *field, int rows, int columns) {
    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < columns; ++j) {
            field[i * columns + j] = 0;
        }
    }

    int percentage = 10 + random() % 20;
    int count_of_bad_plots = columns * rows * percentage / 100;
    for (int i = 0; i < count_of_bad_plots; ++i) {
        int row_index;
        int column_index;
        do {
            row_index = random() % rows;
            column_index = random() % columns;
        } while (field[row_index * columns + column_index] == -1);

        field[row_index * columns + column_index] = -1;
    }
}

void createSemaphores(sem_t *semaphores, int count) {
    for (int k = 0; k < count; ++k) {
        if (sem_init(semaphores + k, 1, 1) < 0) {
            perror("sem_init: can not create semaphore");
            exit(-1);
        };

        int val;
        sem_getvalue(semaphores + k, &val);
        if (val != 1) {
            printf(
                "Ooops, one of semaphores can't set initial value to 1. Please, restart server.\n");
            shm_unlink(shared_object);
            exit(-1);
        }
    }
}

sem_t *createSemaphoresSharedMemory(int sem_count) {
    int sem_main_shmid;
    sem_t *semaphores;

    if ((sem_main_shmid = shm_open(sem_shared_object, O_CREAT | O_RDWR, 0666)) < 0) {
        perror("Can't connect to shared memory");
        exit(-1);
    } else {
        if (ftruncate(sem_main_shmid, sem_count * sizeof(sem_t)) < 0) {
            perror("Can't rezie shm");
            exit(-1);
        }
        if ((semaphores = mmap(0, sem_count * sizeof(sem_t), PROT_WRITE | PROT_READ, MAP_SHARED,
                               sem_main_shmid, 0)) < 0) {
            printf("Can\'t connect to shared memory for semaphores\n");
            exit(-1);
        };
        printf("Open shared Memory for semaphores\n");
    }

    return semaphores;
}

void writeInfoToConsole(int sock, struct sockaddr_in multicast_address) {
    while (1) {
        struct Event event;
        if (read(pipe_fd[0], &event, sizeof(event)) < 0) {
            perror("Can't read from pipe");
            exit(-1);
        }
        if (event.type == MAP) {
            printf("%s | %s\n", event.timestamp, event.buffer);
        }

        char buffer[sizeof(event.timestamp) + sizeof(event.buffer) + 3];
        int size = sprintf(buffer, "%s | %s\n", event.timestamp, event.buffer);

        if (sendto(sock, buffer, size, 0, (struct sockaddr *)&multicast_address,
                   sizeof(multicast_address)) != size) {
        }
    }
}

int multicast_socket;
pthread_t writer_thread;
struct sockaddr_in multicast_address;
int runWriter(in_addr_t sin_addr, int port, int multicast_tll) {
    multicast_socket = createMulticastSenderSocket(sin_addr, port, multicast_tll);
    pid_t chpid = fork();

    memset(&multicast_address, 0, sizeof(multicast_address));
    multicast_address.sin_family = AF_INET;
    multicast_address.sin_addr.s_addr = sin_addr;
    multicast_address.sin_port = htons(port);

    if (chpid == 0) {
        writeInfoToConsole(multicast_socket, multicast_address);
        exit(0);
    }

    return 0;
}

struct Args {
    int socket;
    sem_t *sem;
};

void *readTasks(void *args) {
    struct Args params = *((struct Args *)args);

    while (1) {
        struct pollfd fd;
        fd.fd = params.socket;
        fd.events = POLLIN;

        int result = poll(&fd, 1, -1);
        if (result < 0) {
            perror("Can't poll socket");
            exit(-1);
        }

        struct GardenerTask task;
        struct sockaddr_in client_address;
        socklen_t client_length = sizeof(client_address);

        sem_wait(params.sem);
        int received_size = 0;
        if ((received_size = recvfrom(params.socket, &task, sizeof(task), 0,
                                      (struct sockaddr *)&client_address, &client_length)) !=
            sizeof(task)) {
            perror("Can't receive task");
            sem_post(params.sem);
            continue;
        }
        sem_post(params.sem);

        struct Request request;
        request.task = task;
        request.client_address = client_address;

        int pipe_write_fd = task.gardener_id == 1 ? fg_pipe_fd[1] : sg_pipe_fd[1];

        if (write(pipe_write_fd, &request, sizeof(request)) < 0) {
            perror("Can't write to pipe");
        }
    }
}

void *resultSender(void *args) {
    int server_socket = *((int *)args);
    while (1) {
        struct TaskResult result;
        read(results_pipe_fd[0], &result, sizeof(result));
        sendto(server_socket, result.buffer, result.size, 0,
               (struct sockaddr *)&result.client_address, sizeof(result.client_address));
    }
}

pthread_t task_reader_thread;
void runTaskReader(struct Args *args) {
    if (pthread_create(&task_reader_thread, NULL, readTasks, (void *)args) < 0) {
        perror("Can't run task reader");
        exit(-1);
    }
}

pthread_t result_sender_thread;
void runResultSender(int *server_socket) {
    if (pthread_create(&result_sender_thread, NULL, resultSender, (void *)server_socket) < 0) {
        perror("Can't run result sender");
        exit(-1);
    }
}

int server_socket;
int children_counter = 0;

void sigint_handler(int signum) {
    printf("Server stopped\n");
    char exit_signal[] = "_exit";
    sendto(multicast_socket, exit_signal, sizeof(exit_signal), 0,
           (struct sockaddr *)&multicast_address, sizeof(multicast_address));

    shm_unlink(shared_object);
    shm_unlink(sem_shared_object);
    close(server_socket);
    close(multicast_socket);
    exit(0);
}

int main(int argc, char *argv[]) {

    if (argc != 6) {
        fprintf(stderr,
                "Usage:  %s <Server Address> <Server Port> <Multicast Address> <Multicast Port> "
                "<Square side size>\n",
                argv[0]);
        exit(1);
    }

    in_addr_t server_address;
    if ((server_address = inet_addr(argv[1])) < 0) {
        perror("Invalid server address");
        exit(-1);
    }

    in_addr_t multicast_address;
    if ((multicast_address = inet_addr(argv[3])) < 0) {
        perror("Invalid multicast address");
        exit(-1);
    }

    int server_port = atoi(argv[2]);
    if (server_port < 0) {
        perror("Invalid server port");
        exit(-1);
    }

    int multicast_port = atoi(argv[4]);
    if (server_port < 0) {
        perror("Invalid multicast port");
        exit(-1);
    }

    if (pipe(pipe_fd) < 0) {
        perror("Can't open pipe");
        exit(-1);
    }

    if (pipe(fg_pipe_fd) < 0) {
        perror("Can't open task pipe");
        exit(-1);
    }

    if (pipe(sg_pipe_fd) < 0) {
        perror("Can't open task pipe");
        exit(-1);
    }

    if (pipe(results_pipe_fd) < 0) {
        perror("Can't open task pipe");
        exit(-1);
    }

    runWriter(multicast_address, multicast_port, 1);

    signal(SIGINT, sigint_handler);

    int square_side_size = atoi(argv[5]);
    if (square_side_size > 10 || square_side_size < 2) {
        perror("Square side size should be in range [2, 10]");
        exit(-1);
    }

    int rows = 2 * square_side_size;
    int columns = 2 * square_side_size;
    int sem_count = rows * columns / 4 + 1;

    int *field = getField(rows * columns);
    initializeField(field, rows, columns);

    sem_t *semaphores = createSemaphoresSharedMemory(sem_count);
    createSemaphores(semaphores, sem_count);

    server_socket = createServerSocket(server_address, server_port);
    printField(field, columns, rows);

    struct FieldSize field_size;
    field_size.columns = columns;
    field_size.rows = rows;

    struct Args args;
    args.socket = server_socket;
    args.sem = semaphores + sem_count - 1;

    struct HandleArgs firstGardenerArgs;
    firstGardenerArgs.server_socket = server_socket;
    firstGardenerArgs.field = field;
    firstGardenerArgs.semaphores = semaphores;
    firstGardenerArgs.field_size = field_size;
    firstGardenerArgs.id = 0;
    firstGardenerArgs.pipe_fd = fg_pipe_fd;

    struct HandleArgs secondGardenerArgs;
    secondGardenerArgs.server_socket = server_socket;
    secondGardenerArgs.field = field;
    secondGardenerArgs.semaphores = semaphores;
    secondGardenerArgs.field_size = field_size;
    secondGardenerArgs.id = 1;
    secondGardenerArgs.pipe_fd = sg_pipe_fd;

    runTaskReader(&args);
    runResultSender(&server_socket);
    runGardener(&firstGardenerArgs);
    runGardener(&secondGardenerArgs);

    pthread_join(task_reader_thread, NULL);
    pthread_join(result_sender_thread, NULL);

    return 0;
}
