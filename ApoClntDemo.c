
#ifdef _WIN32
# include <windows.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#ifdef _MSC_VER
# define strcasecmp(a, b) _stricmp(a,b)
#endif

#include "ApoClnt.h"
#include "GuiCmd.h"
#include "DVA.h"

#include "WSServer.h"
#include <unistd.h>
#include "TestrunMgr.h"
#include <arpa/inet.h>
#include <signal.h>


#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 54321
#define BUFFER_SIZE 1024
#define LOCAL_PORT 12346 // Define the local port to bind

int sockfd;
struct sockaddr_in server_addr;
struct sockaddr_in local_addr;

void
requsetTestrunPath()
{
    char send_buffer[BUFFER_SIZE] = "What is the current loaded path?";
    char recv_buffer[BUFFER_SIZE];
    // Send the request to the server
    sendto(sockfd, send_buffer, strlen(send_buffer), 0, (const struct sockaddr *) &server_addr, sizeof(server_addr));

    // Wait for the response from the server
    socklen_t len = sizeof(server_addr);
    printf("Get Current loaded testrun, Waiting for response...\n");
    int n = recvfrom(sockfd, recv_buffer, BUFFER_SIZE, 0, (struct sockaddr *) &server_addr, &len);
    recv_buffer[n] = '\0';
    printf("Server : %s\n", recv_buffer);


    char projectdir[256];
    char testrun[256];
    sscanf(recv_buffer, "projectdir:%[^,], testrun:%s", projectdir, testrun);

    printf("projectdir: %s\n", projectdir);
    printf("testrun: %s\n", testrun);

    if(loadTestrun(projectdir, testrun) != 0)
    {
        printf("Failed to load testrun\n");
        return;
    }

}

typedef struct tDVA_write_list {
    char *			Name;	
} tDVA_write_list;

simStatus gbl_status;

typedef struct traffic_pos{
    double x;
    double y;
    double z;
    char* name;
}traffic_pos;
typedef struct traffic{
    int traffic_num;
    traffic_pos* pos;
}traffic;

traffic* trf;


const char *result_str;
int result_size;

static void
error_and_exit (const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);

    exit(EXIT_FAILURE);
}


/** Basic APO client infrastructure *******************************************/

/* Connection parameters. */

enum { CmdChannel  = 2             };		/* CarMaker command channel. */
enum { ChannelMask = 1<<CmdChannel };

static const char *AppClass = "CarMaker";
static const char *AppHost  = "*";
static const char *AppUser  = "*";


/* Connection handle to the application (CarMaker in this case). */
static tApoSid Srv;


/* Time to wait between calls to ApoClnt_PollAndSleep().
   The duration must be fine-tuned to the actual subscription parameters used.
   If too short you may hog the CPU, if too long you may loose quantity data. */
static int SleepDelta_ms = 20;



static int
connection_ok (void)
{
    return (ApocGetStatus(Srv, NULL) & ApoConnUp) != 0;
}


static void
show_rejected_quantities (void)
{
    int i, nquants;
    const tApoClnt_Quantity *quants = ApoClnt_GetQuants(&nquants);
    for (i=0; i<nquants; i++) {
	if (quants[i].IsRejected)
	    printf("    Cannot subscribe to quantity '%s'\n", quants[i].Name);
    }
}



/*
 * ApoClnt_PollAndSleep()
 *
 * This function has to be provided by the client application.
 * It will be called from within the ApoClnt code whenever the
 * application needs to wait for something without blocking the
 * rest of the client.
 * 
 * You may adapt the function's code to your needs, provided that
 * the basic APO communication structure is kept as is.
 *
 * Don't assume this function being called with a fixed timing.
 */
void
ApoClnt_PollAndSleep (void)
{
    int ret = ApocWaitIO(SleepDelta_ms);
    if (ret == 1) {
        ApocPoll();	/* _Always_ poll, even if connection is down. */

        if (connection_ok()) {
            unsigned long vecid;
            int msglen, msgch;
            char msgbuf[APO_ADMMAX];

            while ((vecid = ApocGetData(Srv)) > 0) {
                if (ApoClnt_UpdateQuantVars(vecid))
                    return;

                /* Put your own quantity data handling code here. */
                /* ... */
            }
            if (ApocDictChanged(Srv))
            {
                printf("Dictionary changed\n");
                ApoClnt_Resubscribe(Srv);
            }
                

            while (ApocGetAppMsg(Srv, &msgch, msgbuf, &msglen) > 0) {
                // if (GuiCmd_HandleMsgs(msgch, msgbuf, msglen))
                //     continue;
                if (DVA_HandleMsgs(msgch, msgbuf, msglen))
                    continue;

                /* Put your own message handling code here. */
                /* ... */
            }
        }else{
            printf("connection is not ok\n");
        }
    }

    /* Put your code handling other regular tasks here. */
    /* ... */
}


static void
self_pause (int milliseconds)
{
    double tstart = ApoGetTime_ms();
    do {
	    ApoClnt_PollAndSleep();
    } while (ApoGetTime_ms()-tstart < milliseconds);
}


static int
setup_client (void)
{

	

    Srv = ApoClnt_Connect(AppClass, AppHost, AppUser, ChannelMask);
    if (Srv == NULL)
    {
        printf("Could not connect\n");
        return -1;
    }

    // if (!GuiCmd_IsReady(Srv))
    // {
    //     printf("GUI not ready\n");
    //     return -1;
    // }
    return 0;
}


static void
teardown_client (void)
{
    ApocCloseServer(Srv);
}


/******************************************************************************/

// static void
// show_result (void)
// {
//     const char *result, *s;
//     int size, status;

//     status = GuiCmd_GetResult(&result, &size);

//     switch (status) {
//       case GUI_EOK:      s = "GUI_EOK (= no error)";             break;
//       case GUI_ETCL:     s = "GUI_ETCL (= tcl error)";           break;
//       case GUI_ETIMEOUT: s = "GUI_ETIMEOUT (= timeout expired)"; break;
//       case GUI_ECONN:    s = "GUI_ECONN (= connection failed)";  break;
//       default:           s = "(unknown result status code)";     break;
//     }

//     printf("    Result status: %s\n", s);
//     printf("    Result size  : %d [characters]\n", size);
//     printf("    Result string: '%s'\n", result);
//     printf("\n");
// }



void connect_polling()
{
    if(Srv && connection_ok())
    {
        return;
    }
    int ret;
    do
    {
        // teardown_client();
        ret = setup_client();
        if (ret != 0)
        {
            printf("Connection failed, retrying...\n");
            
            sleep(1);
        }

    } while (ret != 0);
}

static void RemoveAllQuants()
{
    gbl_status.others_num = 0;
    if(gbl_status.others != NULL)
    {
        free(gbl_status.others);
        gbl_status.others = NULL;
    }
    ApoClnt_RemoveAllQuants();
}

static void addTrafficQuants()
{
    int n = tsInfo.traffic_nObjs;
    gbl_status.others_num = n;
    if(gbl_status.others != NULL)
    {
        free(gbl_status.others);
    }
    gbl_status.others = (position*)malloc(sizeof(position)*n);
    for(int i = 0;i<n;i++)
    {
        char tmpStr[100];
        char *objName = tsInfo.trafficNames[i];
        sprintf(tmpStr, "Traffic.%s.tx", objName);
        ApoClnt_AddQuant(tmpStr, &gbl_status.others[i].x, ApoDouble);
        sprintf(tmpStr, "Traffic.%s.ty", objName);
        ApoClnt_AddQuant(tmpStr, &gbl_status.others[i].y, ApoDouble);
        sprintf(tmpStr, "Traffic.%s.tz", objName);
        ApoClnt_AddQuant(tmpStr, &gbl_status.others[i].z, ApoDouble);
        sprintf(tmpStr, "Traffic.%s.rz", objName);
        ApoClnt_AddQuant(tmpStr, &gbl_status.others[i].heading, ApoDouble);
        // printf("addQuant index :%d name: %s z offset = %lf\n", i, objName, tsInfo.trafficObjs[i].offset);
    }
}


static void sendMsgStatus()
{
    //TODO
    gbl_status.ego.z -= 0.5;
    size_t fixedLen = sizeof(double)+sizeof(position)+sizeof(uint64_t);
    size_t dynLen = sizeof(position) * gbl_status.others_num;

    size_t len = fixedLen + dynLen;
    unsigned char *msg = (unsigned char *)malloc(len);
    // static int frameCount = 0;
    // if(frameCount++%60 == 0)
    // {
    //     for(int i = 0;i<10;i++)
    //     {
    //         printf("index %d: x = %lf y = %lf z = %lf\n", i, 
    //             gbl_status.others[i].x,
    //             gbl_status.others[i].y, gbl_status.others[i].z);
    //     }

    // }

    memcpy(msg, &gbl_status, fixedLen);
    memcpy(msg + fixedLen, gbl_status.others, dynLen);
    sendMsgX(msg, len);
    free(msg);
}

// typedef struct 
// {
//     uint32_t name_len;
//     char *name;
//     tApoQuantType type;
//     void *data;
// }quantData;

static int getDataSize(tApoQuantType type)
{
    int size = 0;
    switch (type) {
        case ApoDouble: 
        case ApoLLong:  
        case ApoULLong: size = 8; break;
        case ApoFloat:
        case ApoInt:    
        case ApoUInt:   size = 4; break;
        case ApoShort:  
        case ApoUShort: size = 2; break;
        case ApoChar:    
        case ApoUChar:  size = 1; break;
        //TODO
        // case ApoLong:break;
        // case ApoULong:break;
        default:        size = 0; break;
    }
    return size;
}
#define MAX_BUFFER_SIZE 4096

static void sendMsgSubQuants()
{
    unsigned char* buf = (unsigned char *)malloc(sizeof(unsigned char)*MAX_BUFFER_SIZE);
    char **quants = NULL;
    int n = getSubQuants(&quants);
    uint32_t offset = 0;
    for(int i = 0; i < n; i++)
    {
        char *name = quants[i];
        tApoClnt_Quantity* qu = ApoClnt_GetQuant(name);

        if(qu)
        {
            uint32_t len = strlen(qu->Name);

            uint32_t dataSize = getDataSize(qu->VarType);
            uint32_t totalSize = sizeof(len) + len + sizeof(tApoQuantType) + dataSize;

            // 检查是否有足够的空间来存储整个结构体
            if (offset + totalSize > MAX_BUFFER_SIZE) {
                printf("no enough memory to send subscribed quants!\n");
                free(buf);
                return;
            }
            memcpy(buf + offset, &len, sizeof(len));
            offset += sizeof(len);
            memcpy(buf + offset, qu->Name, len);
            offset += len;
            memcpy(buf + offset, &qu->VarType, sizeof(qu->VarType));
            offset += sizeof(qu->VarType);
            memcpy(buf + offset, qu->VarPtr, dataSize);
            offset += dataSize;
        }
    }
    setSubQuantsBuffer(buf, offset);
    free(buf);
}

int sc_state;
#define SC_STATE_SIMULATE 8

static void 
registerAllQuants()
{
    clearQuants();
    int n = ApocGetQuantCount(Srv);
    int total = 0;
    for (int i = 0; i < n; i++) {
        const tApoQuantInfo *qinf = ApocGetQuantInfo(Srv, i);
        registerQuant(qinf->Name, qinf->Type);
    }
}

static char **specialQuants = NULL;
static int specialCount = 0;


//直接检索整个已订阅Quants的列表与UAQMgr维护的列表作对比，若出现不同则重新订阅
static void 
refreshSubQuants()
{

    char **addQuants = NULL;
    char **removeQuants = NULL;
    int len1 = 0;
    int len2 = 0;
    int lenA = 0;
    const tApoClnt_Quantity *A = ApoClnt_GetQuants(&lenA);
    

    getChangedQuants(A           , lenA,
                    specialQuants, specialCount,
                    &addQuants   , &len1,
                    &removeQuants, &len2);

    if(len1 > 0)
    {
        for(int i = 0; i < len1; i++)
        {
            char *name = addQuants[i];
            printf("addWebSubQuant :%s\n", name);
            tApoQuantType type = getQuantType(name);
            int size = getDataSize(type);
            if(size > 0)
            {
                void *var = malloc(size);
                ApoClnt_AddQuant(name, var, type);
            }
        }
    }

    if(len2 > 0)
    {
        for(int i = 0; i < len2; i++)
        {
            char *name = removeQuants[i];
            printf("RemoveWebSubQuant : %s\n", name);
            ApoClnt_RemoveQuant(name);
        }
    }

    if(len1 > 0 || len2 > 0)
    {
        ApoClnt_Subscribe(Srv, 0, 0, 50.0, 1);
        show_rejected_quantities();
    }
}


static void
registerSpecialQuants()
{
    if(specialQuants != NULL && specialCount > 0)
    {
        // 释放 specialQuants 中的内存
        for (int i = 0; i < specialCount; i++) {
            free(specialQuants[i]);
        }
        printf("free specialQuants\n");
        free(specialQuants);
        specialCount = 0;
    }

    const tApoClnt_Quantity * subQuants = ApoClnt_GetQuants(&specialCount);

    // 分配内存给 specialQuants 字符串数组
    specialQuants = (char **)malloc(specialCount * sizeof(char *));
    if (specialQuants == NULL) {
        printf("Error allocating memory for specialQuants\n");
        return;
    }


    // 复制 subQuants 中的 Name 到 specialQuants
    for (int i = 0; i < specialCount; i++) {
        specialQuants[i] = strdup(subQuants[i].Name);
        if (specialQuants[i] == NULL) {
            printf("Error duplicating string for specialQuants[%d]\n", i);
            // 释放已分配的内存
            for (int j = 0; j < i; j++) {
                free(specialQuants[j]);
            }
            printf("free specialQuants\n");
            free(specialQuants);
            specialCount = 0;
            return;
        }
    }
    // printf("---------------\n");
    // for (int i = 0; i < specialCount; i++) {
    //     printf("i : %d, specialQuantName :%s\n", i, specialQuants[i]);
    // }
}
static void
demo_Test (void)
{  
    RemoveAllQuants();
    ApoClnt_AddQuant("SC.State", &sc_state, ApoInt);
    ApoClnt_Subscribe(Srv, 0, 0, 50.0, 1);

    printf("Waiting for simulation to start...\n");
    while(sc_state != SC_STATE_SIMULATE)
    {
        if(!connection_ok())
        {
            return;
        }
        self_pause(200);
    }
    registerAllQuants();

    requsetTestrunPath();
    sendMsgInit();
    sendMsgQuants();

    ApoClnt_AddQuant("Time",          &gbl_status.time,         ApoDouble);
    ApoClnt_AddQuant("Car.tx",        &gbl_status.ego.x,        ApoDouble);
    ApoClnt_AddQuant("Car.ty",        &gbl_status.ego.y,        ApoDouble);
    ApoClnt_AddQuant("Car.tz",        &gbl_status.ego.z,        ApoDouble);
    ApoClnt_AddQuant("Car.Yaw",       &gbl_status.ego.heading,  ApoDouble);
    addTrafficQuants();
    ApoClnt_Subscribe(Srv, 0, 0, 50.0, 1);
    show_rejected_quantities();
    registerSpecialQuants();
    do {
        refreshSubQuants();
        sendMsgStatus();
        sendMsgSubQuants();

        if(sc_state != SC_STATE_SIMULATE)
        {
            printf("Simulation Status is not running\n");
            return;
        }
        if(!connection_ok())
        {
            return;
        }

        self_pause(30);
    } while (1);
}

static void
usage_and_exit (void)
{
    printf("\n");
    printf("Usage: ApoClntDemo [options] example\n");
    printf("\n");
    printf("Available options:\n");
    printf("  -apphost h    Connect to application on host h\n");
    printf("  -appuser u    Connect to application start by user u\n");
    printf("  -apclass c    Connect to application of class c\n");
    printf("\n");
    printf("Available examples:\n");
    printf("  Basics\n");
    printf("  StartStop\n");
    printf("  QuantSubscribe\n");
    printf("  DVARead\n");
    printf("  DVAWrite\n");
    printf("  DVAWrite_CM6\n");
    printf("\n");

    exit(EXIT_SUCCESS);
}

void* websocket_thread(void *args) 
{
    int port = *(int*)args;
    run(port);
}


int 
initUdpSocket()
{
    // Create a UDP socket
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        printf("socket creation failed\n");
        return -1;
    }

     // Bind the socket to a local port
    memset(&local_addr, 0, sizeof(local_addr));
    local_addr.sin_family = AF_INET;
    local_addr.sin_port = htons(LOCAL_PORT);
    local_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sockfd, (const struct sockaddr *)&local_addr, sizeof(local_addr)) < 0) {
        printf("bind failed\n");
        close(sockfd);
        return -2;
    }


    // Fill server information
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
    return 0;
}

void cleanup()
{
    // Close socket
    if (sockfd >= 0) {
        close(sockfd);
    }
}

void signal_handler(int signum)
{
    printf("Caught signal %d, exiting...\n", signum);
    cleanup();
    teardown_client();
    exit(EXIT_SUCCESS);
}

int
main (int ac, char **av)
{

    ApoInitLocale();
    if (ApocInit() != ApoErrOk)
    {
        printf("Fail to initialize APO library\n");
        return -1;
    }

    const char *example = "";
    initUdpSocket();
    // Register signal handler for SIGINT and SIGTERM
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    int wsport = 8765;
    while (*++av != NULL) {
        if(strcmp(*av, "-wsport") == 0 && *(av+1) != NULL)
        {
            wsport = atoi(*++av);
        }
        else if(strcmp(*av, "-appclass") == 0 && *(av+1) != NULL) {
            AppClass = *++av;
        } else if (strcmp(*av, "-apphost")  == 0 && *(av+1) != NULL) {
            AppHost = *++av;
        } else if (strcmp(*av, "-appuser")  == 0 && *(av+1) != NULL) {
            AppUser = *++av;
        } else if (strcmp(*av, "-help") == 0 || strcmp(*av, "-h") == 0) {
            usage_and_exit();
        } else if (**av == '-') {
            printf("Unknown option: '%s'\n", *av);
            printf("Run with '-help' to see the usage.\n");
            exit(EXIT_FAILURE);
        } else {
            example = *av;
            break;
        }
    }
    if (strcmp(AppHost, "*") == 0) {
        printf("No host specified, using 'localhost' as default!\n");
        AppHost = "localhost";
    }
    
    pthread_t t;
    pthread_create(&t, NULL, websocket_thread, &wsport);

    while(1)
    {
        connect_polling();
        demo_Test();
    }

    return EXIT_SUCCESS;
}

