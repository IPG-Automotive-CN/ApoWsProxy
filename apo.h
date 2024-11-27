/******************************************************************************/
/*  apo.h								      */
/*  Communication library for online applications.			      */
/*  ------------------------------------------------------------------------  */
/*  (c) IPG Automotive GmbH    www.ipg-automotive.com   Fon: +49.721.98520-0  */
/*  Bannwaldallee 60     D-76185 Karlsruhe    Germany   Fax: +49.721.98520-99 */
/******************************************************************************/

#ifndef __APO_H__
#define __APO_H__

#include <stdio.h>

#ifdef __cplusplus
# define __APO_CPP_BEGIN      extern "C" {
# define __APO_CPP_END        }
__APO_CPP_BEGIN
#endif


/******************************************************************************/

enum {
    APO_STRLEN		= 64,
    APO_TXTLEN		= 256,
    APO_ADMMAX		= 16384,	// 16 KB
    APO_BIGMAX		= 1048576,	// 1 MB (ignored in APO >= 1.33.3)
    APO_ADMCHMAX	= 32,
    APO_MAXCONN		= 16
};


#define APO_NUMVER 20101
extern const int  ApoNumVersion;
extern const char ApoVersion[];
extern const char ApoArch[];
extern const char ApoCompFlags[];


/** ApocSetOption() / AposSetOption() / ApocGetSizeInfo() *********************/

enum {
    ApoBrokerDirs        = -1,		/* Server */

    ApoExplicitData      = -2,		/* Server */
    ApoChangeableDict    = -3,		/* Server */
    ApoAllowedPeers      = -4,		/* Server */
    ApoContactCheck      = -5,		/* Server */
    ApoDataSockRecvSize  = -6,		/* Client, GetSize [kB] */
    ApoCmdSockRecvSize   = -7,		/* Client, GetSize [kB] */
    ApoDictMsgSize       = -8,		/* GetSize */
    /*ApoDictDeltaSize   = -9,*/	/* GetSize */
    ApoDictEntryCount    = -10,		/* GetSize */
    ApoCmdMaxWrite       = -11,		/* Server */
    ApoDataSockSendSize  = -12,		/* Server, GetSize [kB] */
    ApoCmdSockSendSize   = -13,		/* Server, GetSize [kB] */
    ApoBackgroundIO      = -14,		/* Server [UNUSED] */
    ApoBackgroundPrio    = -15,		/* Server */
    ApoQTimeoutToIO      = -16,		/* Server [us] */
    ApoQSizeToIO         = -17,		/* Server, GetSize [kB] */
    ApoQSizeFromIO       = -18,		/* Server, GetSize [kB] */
    ApoQSizeAppMsgs      = -19,		/* Server, GetSize [kB] */
    ApoBackgroundPfx     = -20,		/* Server */
    ApoRelaxedRedefs     = -21,		/* Server */
    ApoCmdSendBufSize    = -22,		/* Client, Server, GetSize [kB] */
    ApoZWindowBits       = -23,		/* Server, 9..15, default 12 */
    ApoZMemLevel         = -24,		/* Server, 6..9, default 7 */
    ApoBackgroundCPU     = -25,		/* Server [int] */
    ApoPollOnlyMsgs      = -26,		/* Server [bool] */
    ApoBrokerUseAsIs     = -27,		/* Server [bool] */
    ApoBulkMsgSleep      = -28,		/* Server [ms] */
    ApoBulkMsgTimeout    = -29,		/* Server [ms] */
    ApoUseRealtimeMode   = -30,		/* Server [bool] */

    /* Undocumented options intended for internal use only: */
    _ApoBrokerStartCheck = -4096,	/* Args: bool */
    _ApoSlimPolling      = -4097,	/* Args: bool (not for XENO) */
    _ApoDebug            = -4098	/* Args: unsigned int */
};


/** Error handling and logging ************************************************/

enum {
    ApoErrOk = 0,
    ApoErrSys = -1,
    ApoErrSid = -2,
    ApoErrIndex = -3,
    ApoErrInt = -4,
    ApoErrUnreach = -5,
    ApoErrInit = -6,
    ApoErrDictOpen = -7,
    ApoErrDictClosed = -8,
    ApoErrDictDef = -9,
    ApoErrDictSize = -10,
    ApoErrDictQuant = -11,
    ApoErrDictProp = -12,
    ApoErrDown = -13,
    ApoErrMask = -14,
    ApoErrMaxConn = -15,
    ApoErrDictChanged = -16,
    ApoErrTimeout = -17,
    ApoErrUnknown = -18,
    ApoErrFeatureRexec = -19,
    ApoErrRefused = -20,
    ApoErrParameter = -21,
    ApoErrClientVer = -22,
    ApoErrBrokerVer = -23,
    ApoErrExec = -24,
    ApoErrMem = -25,
    ApoErrZlib = -26,
    ApoErrClientIndex = -27,
    ApoErrServerVer = -28
};

const char *ApoStrError (int err);


typedef void (*tApoLogFunc) (const char *format, ...);

void ApoSetLogger (tApoLogFunc logfunc);

void ApoLogNothing  (const char *format, ...);
void ApoLogToScreen (const char *format, ...);


unsigned ApoSetDebug (unsigned flags);
unsigned ApoSetDebugStr (const char *flagstr);
unsigned *ApoGetDebugPtr (void);
void ApoSetDebugClient (int who);


/** Connection states *********************************************************/

enum {
    /* No connection. */
    ApoConnDown = 0x10,
    ApoConnError,
    ApoConnTimedout,
    ApoConnRejected,
    ApoConnDisconnected,

    /* Connection is being built up. */
    ApoConnPending = 0x20,

    /* Connection established. */
    ApoConnUp = 0x40,
    ApoConnUnsubscribed,
    ApoConnSubscribing,
    ApoConnReady,
    ApoConnDictChange
};

const char *ApoStrStatus (int status);


/** Client side functions *****************************************************/

int ApocInit (void);
int ApocInitWin (void); // Like ApocInit(), but without calling WSAStartup().

int ApocSetOption (int option, ...);

int ApocQueryServers (int timeout_ms, const char *hostname);
int ApocQueryDone (void);
unsigned ApocResolve (const char *hostname);

int ApocGetBrokerCount (void);

typedef struct tApobrokerInfo {
    const char *		Hostname;
    const char *		Arch;
    int				nServers;
    int				NumVersion;
    const char *		Features;
    unsigned long		StartTime;
    unsigned			NetAddress;
    const char *		NetHostname;
} tApoBrokerInfo;

const tApoBrokerInfo *ApocGetBroker (int index);

int ApocGetServerCount (void);

typedef struct tApoServerInfo {
    unsigned long		StartTime;
    unsigned long		Pid;
    unsigned short		nClients;
    unsigned short		maxClients;
    double			maxFreq;
    const char *		Username;
    const char *		Hostname;
    const char *		AppClass;
    const char *		Identity;
    const char *		Description;
    unsigned short		Port;
    unsigned short		Unused;
    unsigned			NetAddress;
    const char *		NetHostname;
} tApoServerInfo;

const tApoServerInfo *ApocGetServer (int index);


typedef struct tServerInfo *tApoSid;

tApoSid ApocOpenServer (int server);
int ApocConnect (tApoSid sid, unsigned long admmask);
void ApocCloseServer (tApoSid sid);
void ApocDisconnect (tApoSid sid);

void ApocPoll (void);
int  ApocWaitIO (int timeout_ms);

int ApocGetStatus (tApoSid sid, const char **pcause);
unsigned long ApocGetAppMask (tApoSid sid);
int ApocDictChanged (tApoSid sid);
int ApocShouldRestart (tApoSid sid);
int ApocGetRestartId (tApoSid id);


typedef enum {
    ApoDouble,			/* 64 bits */
    ApoFloat,			/* 32 bits */
    ApoLong,  ApoULong,		/* 32/64 bits depending on local arch */
    ApoShort, ApoUShort,	/* 16 bits */
    ApoChar,  ApoUChar,		/*  8 bits */
    ApoInt,   ApoUInt,		/* 32 bits */
    ApoLLong, ApoULLong,	/* 64 bits */

    ApoVoidType = 65535
} tApoQuantType;

typedef enum {
    ApoPropMono		= 0x01,	/* Quantity is strictly monotonic. */
    _ApoFirstState4	= 0x02,	/* !!FOR INTERNAL USE ONLY!!*/
    _ApoDouble8		= 0x04,	/* !!FOR INTERNAL USE ONLY!!*/
    _ApoDouble4		= 0x08,	/* !!FOR INTERNAL USE ONLY!!*/
    ApoPropWritable	= 0x10,	/* Quantity is writable */
    ApoPropDiscont	= 0x20
} tApoQuantProps;
#define ApoFirstState4(i) (((i << 4) & 0xF0) | _ApoFirstState4)

typedef struct {
    char *			Name;
    char *			Unit;
    tApoQuantType		Type;
    tApoQuantProps		Properties;
    int				nStates;
    int				FirstState;
    void *			Addr;		/* server-side only. */
} tApoQuantInfo;

int ApocGetQuantCount (tApoSid sid);
const tApoQuantInfo *ApocGetQuantInfo (tApoSid sid, int index);
const tApoQuantInfo *ApocQuantLookup (tApoSid sid, const char *Name);


typedef struct tApoSubscription {
    char *			Name;		/* In  */
    char **			Names;		/* In, if Name==NULL */
    const char *		SubscribedName;	/* Out */
    tApoQuantType		Type;		/* Out */
    const void *		Ptr;		/* Out */
} tApoSubscription;

int ApocSubscribe (tApoSid sid,
		   int nsubs, tApoSubscription subs[],
		   int clntbacklog, int servbacklog,
		   double freq, int useapptime);


unsigned long ApocGetData (tApoSid sid);
unsigned long ApocGetFloatVec (tApoSid serv, float *Vec, int VecSize);


int ApocSendAppMsg (tApoSid sid, int msgch, const void *msg, int msglen);
int ApocSendBigMsg (tApoSid sid, int msgch, const void *msg, int msglen);

unsigned long ApocGetAppMsg (tApoSid sid, int *admch, void *admbuf, int *admlen);


int  BrokerConnect    (char **prejectcause, const char *hostname, int force);
void BrokerDisconnect (void);
int  BrokerGetStatus  (void);

typedef int tProcHandle;

int  BrokerProc_Open  (tProcHandle *pph, char **perror,
		       char *const *envv, char *const *argv);
int  BrokerProc_Close (tProcHandle ph, int detach);

int         BrokerProc_GetCount (void);
tProcHandle BrokerProc_GetHandle (int i);
int         BrokerProc_IsValidHandle (tProcHandle ph);

char *const *BrokerProc_GetArgv (tProcHandle ph, int *argc);
char *const *BrokerProc_GetEnvv (tProcHandle ph, int *envc);
int  BrokerProc_GetPid  (tProcHandle ph);
int  BrokerProc_GetExitStatus (tProcHandle ph);

int  BrokerProc_TakeOutput (tProcHandle ph, char *buf, int bufsize);

const char *BrokerProc_GetOutput (tProcHandle ph, int *length);
void        BrokerProc_ClearOutput (tProcHandle ph);

int  BrokerProc_Kill (tProcHandle ph, int sig);
int  BrokerProc_Wait (tProcHandle ph, int timeout_ms);


/** Server side functions *****************************************************/

int AposInit    (int maxclients, int maxbacklog, unsigned long admmask, double maxfreq_hz);
int AposInitWin (int maxclients, int maxbacklog, unsigned long admmask, double maxfreq_hz); // Like AposInit(), but without calling WSAStartup().


int AposSetOption (int option, ...);
void AposSetBrokerDir (const char *dir); /* deprecated, use AposSetOption() */


typedef enum {
    ApoBrokerNotStarted     = -1,
    ApoBrokerAlreadyRunning =  0,
    ApoBrokerStarted        =  1,
    ApoBrokerUnknown        =  2
} tApoBrokerStatus;
tApoBrokerStatus AposGetBrokerStatus (const char **ppath);

void AposSetInfo (const char *appclass,
		  const char *identity, const char *description);

unsigned long AposGetPid (void);

void AposRestart (void);
void AposDisconnect (const char *cause);
void AposDisconnectClient (const char *cause, int who);

void AposPoll (double apptime);
int  AposWaitIO (int timeout_ms);

void AposSendAppMsg         (int admch, const void *admbuf, int admlen);
void AposSendAppMsgTo       (int admch, const void *admbuf, int admlen, int who);
void AposSafelySendAppMsg   (int admch, const void *admbuf, int admlen);
void AposSafelySendAppMsgTo (int admch, const void *admbuf, int admlen, int who);

int  AposCanTakeNewAppMsg   (int admlen);


unsigned long AposGetAppMsg     (int *admch, void *admbuf, int *admlen);
unsigned long AposGetAppMsgFrom (int *admch, void *admbuf, int *admlen, int *who);
void *AposGetLastAppMsgBuf (int *admlen);

int AposGetBigAppMsgSize (int who);
int AposSetBigAppMsgSize (int who, int size);


int AposClientStatus (int who);
int AposClientUp     (int who);
int AposClientReady  (int who);


void AposExit    (void);
void AposExitWin (void);


int AposDefDouble(const char *name, const char *unit,
		  tApoQuantProps props, double *p);
int AposDefDouble4(const char *name, const char *unit, /* double, transferred as float */
		   tApoQuantProps props, double *p);
int AposDefFloat (const char *name, const char *unit,
		  tApoQuantProps props, float *p);
int AposDefLLong (const char *name, const char *unit, int nstates,
		  tApoQuantProps props, long long *p);
int AposDefULLong(const char *name, const char *unit, int nstates,
		  tApoQuantProps props, unsigned long long *p);
int AposDefLong  (const char *name, const char *unit, int nstates,
		  tApoQuantProps props, long *p);
int AposDefULong (const char *name, const char *unit, int nstates,
		  tApoQuantProps props, unsigned long *p);
int AposDefInt   (const char *name, const char *unit, int nstates,
		  tApoQuantProps props, int *p);
int AposDefUInt  (const char *name, const char *unit, int nstates,
		  tApoQuantProps props, unsigned int *p);
int AposDefShort (const char *name, const char *unit, int nstates,
		  tApoQuantProps props, short *p);
int AposDefUShort(const char *name, const char *unit, int nstates,
		  tApoQuantProps props, unsigned short *p);
int AposDefChar  (const char *name, const char *unit, int nstates,
		  tApoQuantProps props, char *p);
int AposDefUChar (const char *name, const char *unit, int nstates,
		  tApoQuantProps props, unsigned char *p);
int AposUndef (const char *name);
void AposPurge (void);
int AposReady (void);

int AposSetQuantUpdate (int who, int automatic);
int AposGetQuantUpdate (int who);
int AposSendDataVec (int who, double apptime);

void AposNewData (double apptime);


typedef double (*tApoGetFunc) (const void *ptr);
typedef void   (*tApoSetFunc) (double val, void *ptr);
void *AposGetVar (const char *name,
		  tApoQuantType *type, tApoGetFunc *getfunc, tApoSetFunc *setfunc);



/** Misc. stuff ***************************************************************/

int ApoInitLocale (void);

int ApoGetTypeInfo (tApoQuantType type,
		    tApoGetFunc *getfunc, tApoSetFunc *setfunc, int *size);

double ApoGetTime_ms (void); /* Must not be used before ApocInit()/AposInit()! */
void   ApoSleep_ms (int milliseconds);

unsigned long ApoGetPid (void);

void AposSetPollTask (void);


/*
**  Tcl-Interface.
*/

/* Tcl-related stuff below can be activated by #including <tcl.h> */
#ifdef TCL_VERSION

int Apoc_Init (Tcl_Interp *interp);
int Apos_Init (Tcl_Interp *interp);

#endif


/*
 * Undocumented stuff.
 */
int  ApocGetSizeInfo (tApoSid serv, int which);
int  AposGetSizeInfo (int which);

int  AposGetQuantCount (void);
const tApoQuantInfo *AposGetQuantInfo (int index);
const tApoQuantInfo *AposQuantLookup (const char *Name);

int  AposGetDictId (void);
void AposDictParkValues (void);
void AposDict2File (FILE *fp);


typedef struct {
    unsigned		nIOPolls;
    struct tApoQueueStat {
	int		Used,  Size;
	int		nMsgs, maxMsgs;
	int		nAdded, nTaken;
    } ToIO, FromIO, AppMsgsFromIO;

    int			nClients;

    int			nQuantities;
    int			nMsgsPerSecond;
    int			BytesPerSecond;

    int			AppMsgRecv_nMsgs;
    int			AppMsgRecv_nBytes;
    int			AppMsgRecv_nOverflows;

    int			AppMsgSend_nMsgs;
    int			AppMsgSend_nOrigMsgs;
    int			AppMsgSend_nBytes;
} tApoIOStat;

void AposGetIOStat (tApoIOStat  *st); /* DEPRECATED. DON'T USE! */


typedef struct {
    unsigned		nIOPolls;
    struct tApoQueueStat2 {
	unsigned	Used,  Size;
	unsigned	nMsgs, maxMsgs;
	unsigned	nAdded, nTaken;
    } ToIO, FromIO, AppMsgsFromIO;

    int			nClients;

    int			nQuantities;
    int			nMsgsPerSecond;
    int			BytesPerSecond;

    unsigned		AppMsgRecv_nMsgs;
    unsigned		AppMsgRecv_nBytes;
    unsigned		AppMsgRecv_nOverflows;

    unsigned		AppMsgSend_nMsgs;
    unsigned		AppMsgSend_nOrigMsgs;
    unsigned		AppMsgSend_nBytes;

    char		Unused[140];
} tApoIOStat2;

void AposGetIOStat2 (tApoIOStat2 *st);


/******************************************************************************/

#ifdef __cplusplus
__APO_CPP_END
#endif

#endif /* __APO_H__ */


