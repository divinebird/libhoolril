#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>
#include <errno.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include "logger.h"
#include "transmitter.h"

const char sock_path[] = "\0hookril_tranlator";

typedef struct {
    void* next;
    TransData* transData;
} TransDataArray;

volatile uint8_t collectData = 0;
TransDataArray* dataArrayBegin = NULL;
pthread_mutex_t lock;

TransDataArray* getNextData() {
    pthread_mutex_lock(&lock);
    TransDataArray* curItem = dataArrayBegin;
    if(curItem != NULL) {
        dataArrayBegin = curItem->next;
    }
    pthread_mutex_unlock(&lock);
    return curItem;
}

void clearTransData() {
    collectData = 0;
    pthread_mutex_lock(&lock);
    while(dataArrayBegin != NULL) {
        TransDataArray* nextItem = dataArrayBegin->next;
        free(dataArrayBegin->transData);
        free(dataArrayBegin);
        dataArrayBegin = nextItem;
    }
    pthread_mutex_unlock(&lock);
}

uint8_t isTransmittionActive() {
//    pthread_mutex_lock(&lock);
    uint8_t state = collectData;
//    pthread_mutex_unlock(&lock);
    return state;
}

void putNextData(TransData* data) {
    SLOGD("Put data to array");
    pthread_mutex_lock(&lock);
    if(collectData) {
        if(dataArrayBegin == NULL) {
            dataArrayBegin = malloc(sizeof(TransDataArray));
            dataArrayBegin->next = NULL;
            dataArrayBegin->transData = data;
        }
        else {
            TransDataArray* nextItem = dataArrayBegin;
            while(nextItem->next != NULL)
                nextItem = nextItem->next;
            nextItem->next = malloc(sizeof(TransDataArray));
            nextItem = nextItem->next;
            nextItem->next = NULL;
            nextItem->transData = data;
        }
    }
    else
        free(data);
    pthread_mutex_unlock(&lock);
}

uint8_t sockThreadLoop = 1;
static void* socketThreadFunk(void* arg) {
    if (pthread_mutex_init(&lock, NULL) != 0)
    {
        LOGE("Mutex init faild");
        exit(EXIT_FAILURE);
    }
    
    LOGI("Start init daemon socket");
    struct sockaddr_un addr;
    int addr_len = sizeof(struct sockaddr_un);
    int serverSocketFd;

    if ((serverSocketFd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        LOGE("Error while init socket: %s", strerror(errno));
        exit(EXIT_FAILURE);
    }
                                                         
    memset(&addr, 0, sizeof (addr));
    addr.sun_family = AF_UNIX;
    if(sock_path[0] == 0) {
        strncpy(addr.sun_path + 1, sock_path + 1, UNIX_PATH_MAX - 1);
//        int orig_len = addr_len;
        addr_len -= UNIX_PATH_MAX - strlen(addr.sun_path + 1) - 1;
//        LOGI("This is abstract namespace socket, orig_len: %d, cur_len: %d", orig_len, addr_len);
    }
    else            
        strncpy(addr.sun_path, sock_path, UNIX_PATH_MAX);

    unlink(sock_path);
    if (bind(serverSocketFd, (struct sockaddr*)&addr, addr_len) < 0) {
        LOGE("Error while bind socket: %s", strerror(errno));
        exit(EXIT_FAILURE);
    }
    LOGI("End init daemon socket");    
    int clientFd;

    while (sockThreadLoop) {
        if (listen(serverSocketFd, 5) == 0) {
            if((clientFd = accept(serverSocketFd, NULL, NULL)) > 0) {
                SLOGD("Client connection established.");
                collectData = 1;
                while(sockThreadLoop) {
                    TransDataArray* curItem = getNextData();
                    if(curItem == NULL) {
                        sleep(0);
                        continue;
                    }
                    uint8_t leaveSendCycle = 0;
                    if(write(clientFd, curItem->transData, sizeof(curItem->transData)) < 0) {
                        LOGE("Error while send data: %s", strerror(errno));
                        leaveSendCycle = 1;
                    }
                    
                    free(curItem->transData);
                    free(curItem);
                    
                    if(leaveSendCycle) {
                        clearTransData();
                        break;
                    }
                }
            }
            else
                LOGE("Error while accept socket: %s", strerror(errno));
        }
        else
            LOGE("Error while listen socket: %s", strerror(errno));
    }
    
    return NULL;
}

int initSocket() {
    pthread_attr_t attr;
    pthread_t sockThreadId;
    collectData = 0;

    pthread_attr_init(&attr);
    if(pthread_create(&sockThreadId, &attr, &socketThreadFunk, NULL) < 0) {
        LOGE("Error while start new thread: %s", strerror(errno));
        return -1;
    }
    
    return 0;
}