#include "WSServer.h"


#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <libwebsockets.h>

#include <sstream>
#include <iostream>

#include "Protocol_UAQ_Subscribe.cpp"

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



// -------------------start of vhost2--------------------------------------
enum CLIENT_STATE2 {
    NONE = 0,
    BeginSendQuants = 1,
    SendingQuants = 2,
    SendingSubQuants = 3,
};


#define MAX_RX_BUFFER_SIZE 4096

pthread_mutex_t pss_mutex2 = PTHREAD_MUTEX_INITIALIZER;

typedef struct per_session_data2 {
    per_session_data2 *next;
    struct lws *wsi;
    int state = 0;
    int sentQuants = 0;
}pssNode2;

pssNode2 *pss_list2 = NULL;


inline pssNode2* findNode(pssNode2 *list, struct lws *wsi) {
    pssNode2 *p = list;
    while (p) {
        if (p->wsi == wsi) {
            break;
        }
        p = p->next;
    }
    return p;
}

inline void deleteNode(pssNode2 **list, struct lws *wsi) {
    if (list == NULL || *list == NULL) {
        return;
    }
    pssNode2 *p = *list;
    pssNode2 *previous = NULL;
    // 遍历链表寻找匹配的节点
    while (p) {
        if (p->wsi == wsi) {
            if (previous == NULL) {
                // 删除的是头节点
                *list = p->next;
            } else {
                // 删除的是中间或尾部节点
                previous->next = p->next;
            }
            free(p); // 释放节点内存
            return;
        }
        previous = p;
        p = p->next;
    }
}

unsigned char *quantsBuf = NULL;
int quantsBufSize = 0;

int sendMsgQuants()
{
    pthread_mutex_lock(&pss_mutex2);

    printf("sendMsgQuants\n");
    pssNode2 *p = pss_list2;
    while(p)
    {
        p->state = CLIENT_STATE2::BeginSendQuants;
        p->sentQuants = 0;
        lws_callback_on_writable(p->wsi);
        p = p->next;
    }
    pthread_mutex_unlock(&pss_mutex2);
    return 0;
}

int setSubQuantsBuffer(unsigned char *msg, uint32_t len)
{
    pthread_mutex_lock(&pss_mutex2);
    if(quantsBuf)
    {
        free(quantsBuf);
    }
    quantsBuf = (unsigned char *)malloc(LWS_PRE + len);
    if (!quantsBuf) {
        printf("Error allocating memory\n");
        return -1;
    }
    memcpy(quantsBuf + LWS_PRE, msg, len);

    quantsBufSize = len;
    pssNode2 *p = pss_list2;
    while(p)
    {
        lws_callback_on_writable(p->wsi);
        p = p->next;
    }
    pthread_mutex_unlock(&pss_mutex2);
    return 0;
}

static int callback2(struct lws *wsi, enum lws_callback_reasons reason, void *user, void *in, size_t len)
{
    switch (reason) {
        case LWS_CALLBACK_ESTABLISHED:  // A new connection was established
        {
            pthread_mutex_lock(&pss_mutex2);
            printf("new node added\n");
            pssNode2 *newNode = (pssNode2 *)malloc(sizeof(pssNode2));
            newNode->next = pss_list2;
            pss_list2 = newNode;                        
             
            newNode->wsi = wsi;
            newNode->sentQuants = 0;
            newNode->state = CLIENT_STATE2::NONE;

            pthread_mutex_unlock(&pss_mutex2);
            break;
        }

        case LWS_CALLBACK_CLOSED:  // A connection was closed
        {
            pthread_mutex_lock(&pss_mutex2);
            deleteNode(&pss_list2, wsi);
            pthread_mutex_unlock(&pss_mutex2);
            break;
        }

        case LWS_CALLBACK_RECEIVE:  // Data has been received
        {
            pthread_mutex_lock(&pss_mutex2);
            char *text = (char*)malloc(len+1);
            memcpy(text, in, len);
            text[len] = '\0';

            printf("Received message: %s\n", text);

            pssNode2* p = findNode(pss_list2, wsi);
            if(!p)
            {
               printf("node not founded\n");
               pthread_mutex_unlock(&pss_mutex2);
               break;
            }

            if(strcmp(text, "GetAllQuantInfos") == 0)
            {
                p->state = CLIENT_STATE2::BeginSendQuants;
                p->sentQuants = 0;
                lws_callback_on_writable(wsi);
            }else 
            {
                char quantName[256]; // Adjust size as needed
                if (sscanf(text, "Add Quants Subscribe : %s", quantName) == 1)
                {
                    addSubQuant(quantName);
                }else if (sscanf(text, "Remove Quants Subscribe : %s", quantName) == 1)
                {
                    removeSubQuant(quantName);
                }
            }
            pthread_mutex_unlock(&pss_mutex2);
            break;
        }

        case LWS_CALLBACK_SERVER_WRITEABLE: 
        {
            pthread_mutex_lock(&pss_mutex2);
            pssNode2* p = findNode(pss_list2, wsi);
            if(!p || p->state == CLIENT_STATE2::NONE)
            {
                if(!p)
                    std::cout<<"pssNode2 not found!"<<std::endl;
                pthread_mutex_unlock(&pss_mutex2);
                break;
            }

            if(p->state == CLIENT_STATE2::SendingSubQuants)
            {
                if(quantsBufSize > 0)
                {
                    lws_write(wsi, quantsBuf + LWS_PRE, quantsBufSize, LWS_WRITE_BINARY);
                }
                pthread_mutex_unlock(&pss_mutex2);
                break;
            }
            
            if(p->state == CLIENT_STATE2::BeginSendQuants)
            {
                sendWSTextMsg(wsi, "begin send quants");
                p->state = CLIENT_STATE2::SendingQuants;
            }
            else if(p->state == CLIENT_STATE2::SendingQuants)
            {
                unsigned char buf[MAX_RX_BUFFER_SIZE];
                int count = 0;
                int offset = setQuantBuffer(p->sentQuants, buf+LWS_PRE, MAX_RX_BUFFER_SIZE-LWS_PRE, &count);
                
                if(offset == 0)
                {
                    sendWSTextMsg(wsi, "end send quants");
                    p->state = CLIENT_STATE2::SendingSubQuants;
                    pthread_mutex_unlock(&pss_mutex2);
                    break;
                }
                
                int n = lws_write(wsi, buf + LWS_PRE, offset, LWS_WRITE_BINARY);
                if(n < offset)
                {
                    std::cerr<<"Error Writing Buffer"<<std::endl;
                }else
                {
                    //printf("send Quanlist count = %d\n", count);
                    p->sentQuants = count;
                }
            }
            
            lws_callback_on_writable(wsi);
            pthread_mutex_unlock(&pss_mutex2);
            break;


        }
            
        default: break;
    }
    return 0;
}



// static int callback3(struct lws *wsi, enum lws_callback_reasons reason, void *user, void *in, size_t len)
// {
//     switch (reason) {
//         case LWS_CALLBACK_ESTABLISHED: printf("new connection\n"); break;
//         case LWS_CALLBACK_CLOSED: printf("a connection closed\n"); break;
//         case LWS_CALLBACK_RECEIVE:
//             printf("Receive message : %s\n", (char*)in);
//             break;
//         case LWS_CALLBACK_SERVER_WRITEABLE:printf("there is sth need to write \n");break;
//         default: break;
//     }
//     return 0;
// }

struct lws_protocols protocols2[] = 
{
    {
        "ws2",
        callback2,
        0,                   // size_t per_session_data_size;
        4096,                // size_t rx_buffer_size
    },
    { NULL, NULL, 0, 0 }  // Terminator
};

//--------------------------end of vhost2-------------------------------
































pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;

#define MAX_BUFFER 1024
#define CHUNK_SIZE 4000

int statusBufSize = 0;
unsigned char *statusBuf = NULL;


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
            
            if(!p)
            {
                printf("pss not found\n");
                pthread_mutex_unlock(&queue_mutex);
                break;
            }

            if(p->state == CLIENT_STATE::None)
            {
                printf("node state is None skip write\n");
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
    struct lws_vhost *vhost;
    struct lws_vhost *vhost2;
    memset(&info, 0, sizeof(info));

    uint64_t opts = 0;
	info.options = opts | LWS_SERVER_OPTION_EXPLICIT_VHOSTS;

    info.port = port;
    info.protocols = protocols;
    info.gid = -1;
    info.uid = -1;

    // Create the WebSocket server
    struct lws_context *context = lws_create_context(&info);

    if (context == NULL) {
		lwsl_err("libwebsocket init failed\n");
		return -1;
	}

    vhost = lws_create_vhost(context, &info);
	if (!vhost) {
		lwsl_err("vhost creation failed\n");
		return -1;
	}

    info.port++;
    info.protocols = protocols2;
    vhost2 = lws_create_vhost(context, &info);
    if (!vhost2) {
		lwsl_err("vhost2 creation failed\n");
		return -1;
	}
    
    
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

