#include "WSServer.h"


#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <libwebsockets.h>


#include <queue>
#include <string>
#include <pthread.h>
#include <sstream>





pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;

#define MAX_BUFFER 1024
#define CHUNK_SIZE 4000

// This will store all connected WebSocket clients
struct lws *clients[MAX_BUFFER];

int cli_flags[MAX_BUFFER];

int client_count = 0;


int statusBufSize = 0;
unsigned char *statusBuf = NULL;


struct msg {
	void *payload = nullptr; /* is malloc'd */
	size_t len = 0;
};

static struct msg initMsg;
static struct msg statusMsg;


enum CLIENT_STATE {
    None = -1,
    Ready = 0,
    SendingFile = 1,
    SendFileFinished = 2,
    SendTestRun = 3,
    SendTestRunFinished = 4,
    SendingStatus = 5
};


/* one of these is created for each client connecting to us */

typedef struct per_session_data__minimal {
    per_session_data__minimal *next;
    struct lws *wsi;
    int state = 0;
    int sentChunks = 0;
}pssNode;

pssNode *pss_list = NULL;


static void sendWSTextMsg(struct lws *wsi, const char *text)
{
    unsigned char *buf = (unsigned char *)malloc(LWS_PRE + strlen(text));
    if (!buf) {
        printf("Error allocating memory\n");
        return;
    }
    memcpy(buf + LWS_PRE, text, strlen(text));
    lws_write(wsi, buf + LWS_PRE, strlen(text), LWS_WRITE_TEXT);
    free(buf);
}

static uint8_t **fileChunks = NULL;
int num_chunks = 0;
int lastChunkSize = 0;
static int saveOpenDriveFileBuffer()
{
    char *path = getOpenDrivePath();
    if(path == NULL)
    {
        printf("OpenDrive path is NULL\n");
        return -1;
    }
    FILE *file = fopen(path, "rb");
    if (!file) {
        printf("Error opening file\n");
        return -2;
    }

    // Get the file size
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    // Free the old 2D array if it exists
    if (fileChunks != NULL) {
        for (int i = 0; i < num_chunks; i++) {
            free(fileChunks[i]);
        }
        free(fileChunks);
    }

    // Calculate the number of chunks
    num_chunks = (file_size + CHUNK_SIZE - 1) / CHUNK_SIZE;

    // Allocate the 2D array
    fileChunks = (uint8_t **)malloc(num_chunks * sizeof(uint8_t *));
    for (int i = 0; i < num_chunks; i++) {
        fileChunks[i] = (uint8_t *)malloc(CHUNK_SIZE * sizeof(uint8_t));
    }

    // Read the file into the 2D array
    for (int i = 0; i < num_chunks; i++) {
        int n = fread(fileChunks[i], 1, CHUNK_SIZE, file);
        if(i == num_chunks - 1)
        {
            lastChunkSize = n;
        }
    }
    fclose(file);
    return 0;
}

static void sendOpenDriveFile(struct lws *wsi)
{
    char *path = getOpenDrivePath();
    if(path == NULL)
    {
        printf("OpenDrive path is NULL\n");
        return;
    }
    FILE *file = fopen(path, "rb");
    if(!file)
    {
        printf("Error opening file: %s\n", path);
        return;
    }
    unsigned char *buffer = (unsigned char *)malloc(LWS_PRE + CHUNK_SIZE);
    if (!buffer) {
        printf("Error allocating memory\n");
        fclose(file);
        return;
    }

    sendWSTextMsg(wsi, "start of file");
    while (!feof(file)) {
        size_t n = fread(buffer + LWS_PRE, 1, CHUNK_SIZE, file);
        if (n > 0) {
            lws_write(wsi, buffer + LWS_PRE, n, LWS_WRITE_BINARY);
        }
    }
    sendWSTextMsg(wsi, "end of file");
    free(buffer);
    fclose(file);
}

static void sendWSInitMsg(struct lws *wsi)
{
    sendWSTextMsg(wsi, "start of init");
    size_t fixedLen = sizeof(TestrunInfo) - sizeof(void*) - sizeof(char**);
    size_t dynLen = sizeof(TrafficInfo) * tsInfo.traffic_nObjs;

    size_t len = fixedLen + dynLen;
    unsigned char *buf = (unsigned char *)malloc(LWS_PRE + len);
    if (!buf) {
        printf("Error allocating memory\n");
        return;
    }

    memcpy(buf + LWS_PRE, &tsInfo, fixedLen);
    memcpy(buf + LWS_PRE + fixedLen, tsInfo.trafficObjs, dynLen);

    // for(int i = 0; i < tsInfo.traffic_nObjs; i++)
    // {
    //     printf("index %s: rcsClass = %d dimension x = %lf y = %lf z = %lf\n",
    //     tsInfo.trafficNames[i],
    //     tsInfo.trafficObjs[i].rcsClass,
    //     tsInfo.trafficObjs[i].dimension[0], 
    //     tsInfo.trafficObjs[i].dimension[1], tsInfo.trafficObjs[i].dimension[2]);
    // }
    // printf("buf len : %d, contens :",len);
    // for (size_t i = 0; i < 100; i++) {
    //     printf("%02x ", buf[LWS_PRE + i]);
    // }
    // printf("\n");

    lws_write(wsi, buf + LWS_PRE, len, LWS_WRITE_BINARY);
    free(buf);
}



// Callback for handling WebSocket events
static int callback(struct lws *wsi, enum lws_callback_reasons reason, void *user, void *in, size_t len) 
{
    switch (reason) {
        case LWS_CALLBACK_ESTABLISHED:  // A new connection was established
        {
            pthread_mutex_lock(&queue_mutex);
            pssNode *newNode = (pssNode *)malloc(sizeof(pssNode));
            newNode->next = pss_list;
            pss_list = newNode;                        
            pthread_mutex_unlock(&queue_mutex);
            
            newNode->wsi = wsi;
            newNode->sentChunks = 0;
            newNode->state = CLIENT_STATE::None;
            if(getOpenDrivePath() != NULL)
            {
                newNode->state = CLIENT_STATE::Ready;
            }
            lws_callback_on_writable(wsi);
            printf("New connection\n");
            break;
        }
        case LWS_CALLBACK_RECEIVE:  // Data has been received
            // Echo the data back to the client
            // lws_write(wsi, (unsigned char*)in, len, LWS_WRITE_TEXT);
            printf("Received data: %s\n", (char *)in);
            break;
        case LWS_CALLBACK_CLOSED:  // A connection was closed
        {
            /* remove our closing pss from the list of live pss */
            pthread_mutex_lock(&queue_mutex);
            pssNode *p = pss_list, *last = NULL;
            while (p) {
                if (p->wsi == wsi) {
                    if (last) {
                        last->next = p->next;
                    } else {
                        pss_list = p->next;
                    }
                    free(p);
                    break;
                }
                last = p;
                p = p->next;
            }
            pthread_mutex_unlock(&queue_mutex);
            break;
        }
        case LWS_CALLBACK_SERVER_WRITEABLE: 
        {
            pthread_mutex_lock(&queue_mutex);
            pssNode *p = pss_list;
            while(p)
            {
                if (p->wsi == wsi) 
                    break;
                p = p->next;
            }
            
            if(!p || p->state == CLIENT_STATE::None)
            {
                printf("pss not found\n");
                pthread_mutex_unlock(&queue_mutex);
                break;
            }

            if(p->state == CLIENT_STATE::SendingStatus)
            {
                if(statusBufSize > 0)
                {
                    lws_write(wsi, statusBuf + LWS_PRE, statusBufSize, LWS_WRITE_BINARY);
                }
                pthread_mutex_unlock(&queue_mutex);
                break;
            }

            if(p->state == CLIENT_STATE::Ready)
            {
                sendWSTextMsg(wsi, "start of file");
                p->state = CLIENT_STATE::SendingFile;
            }else if(p->state == CLIENT_STATE::SendingFile)
            {
                if(p->sentChunks < num_chunks)
                {
                    int len = CHUNK_SIZE;
                    if(p->sentChunks == num_chunks - 1)
                    {
                        len = lastChunkSize;
                    }

                    unsigned char *buffer = (unsigned char *)malloc(LWS_PRE + len);
                    if (!buffer) {
                        printf("Error allocating memory\n");
                        pthread_mutex_unlock(&queue_mutex);
                        break;
                    }
                    memcpy(buffer + LWS_PRE, fileChunks[p->sentChunks], len);
                    lws_write(wsi, buffer + LWS_PRE, len, LWS_WRITE_BINARY);
                    free(buffer);
                    p->sentChunks++;
                }else
                {
                    sendWSTextMsg(wsi, "end of file");
                    p->state = CLIENT_STATE::SendFileFinished;
                }
            }else if(p->state == CLIENT_STATE::SendFileFinished)
            {
                printf("Send OpenDrive File Finished!\n");
                sendWSTextMsg(wsi, "start of init");
                p->state = CLIENT_STATE::SendTestRun;
            }else if(p->state == CLIENT_STATE::SendTestRun)
            {
                sendWSInitMsg(wsi);
                p->state = CLIENT_STATE::SendTestRunFinished;
            }else if(p->state == CLIENT_STATE::SendTestRunFinished)
            {
                sendWSTextMsg(wsi, "end of init");
                p->state = CLIENT_STATE::SendingStatus;
            }
            lws_callback_on_writable(wsi);
            pthread_mutex_unlock(&queue_mutex);
            break;
        }

        default:
            break;
    }
    return 0;
}

struct lws_protocols protocols[] = {
    {
        "ws",
        callback,
        0,
        4096,
    },
    { NULL, NULL, 0, 0 }  // Terminator
};

int run(int port)
{
    struct lws_context_creation_info info;
    memset(&info, 0, sizeof(info));
    info.port = port;
    info.protocols = protocols;
    info.gid = -1;
    info.uid = -1;

    // Create the WebSocket server
    struct lws_context *context = lws_create_context(&info);

    // Main event loop
    while (1) {
        lws_service(context, 50);
    }

    lws_context_destroy(context);
    return 0;
}


int sendMsgInit()
{
    pthread_mutex_lock(&queue_mutex);
    saveOpenDriveFileBuffer();

    printf("sendMsgInit\n");
    pssNode *p = pss_list;
    while(p)
    {
        p->state = CLIENT_STATE::Ready;
        p->sentChunks = 0;
        lws_callback_on_writable(p->wsi);
        p = p->next;
    }
    pthread_mutex_unlock(&queue_mutex);
    return 0;
}

int sendMsgX(unsigned char *msgX, uint32_t len)
{
    pthread_mutex_lock(&queue_mutex);
    if(statusBuf)
    {
        free(statusBuf);
    }
    statusBuf = (unsigned char *)malloc(LWS_PRE + len);
    if (!statusBuf) {
        printf("Error allocating memory\n");
        return -1;
    }
    memcpy(statusBuf + LWS_PRE, msgX, len);
    statusBufSize = len;
    pssNode *p = pss_list;
    while(p)
    {
        lws_callback_on_writable(p->wsi);
        p = p->next;
    }
    pthread_mutex_unlock(&queue_mutex);
    return 0;
}



// unsigned char *buf = (unsigned char *)malloc(LWS_PRE + 13);
            // char *msg = "Hello, World!";
            // memcpy(buf + LWS_PRE, msg, 13);

            // lws_write(wsi, buf + LWS_PRE, 13, LWS_WRITE_TEXT);

            //sendOpenDriveFile(wsi);

            // int needInit = 0;
            // int i = 0;
            // for (; i < client_count; i++) {
            //     if (clients[i] == wsi) {
            //         needInit = cli_flags[i];
            //         break;
            //     }
            // }

            // if(needInit)
            // {
            //     sendWSInitMsg(wsi);
            //     cli_flags[i] = 0;
            // }else if(statusBufSize > 0)
            // {
            //     lws_write(wsi, statusBuf + LWS_PRE, statusBufSize, LWS_WRITE_BINARY);
            //     statusBufSize = 0;
            //     free(statusBuf);
            // }