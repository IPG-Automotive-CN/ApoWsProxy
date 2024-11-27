#ifndef WSSERVER_H  // Check if WSSERVER_H is not defined
#define WSSERVER_H  // Define WSSERVER_H
#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "TestrunMgr.h"
#include "UAQMgr.h"


int run(int port);

int sendMsgInit();
int sendMsgX(unsigned char *msgX, uint32_t len);


int sendMsgQuants();
int setSubQuantsBuffer(unsigned char *msg, uint32_t len);

#ifdef __cplusplus
}
#endif


#endif  // End of the include guard