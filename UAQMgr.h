#ifndef __UAQMGR_H__
#define __UAQMGR_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include "ApoClnt.h"
#include <stdint.h>
#include <pthread.h>
#include <string.h>

void clearQuants();
void registerQuant(const char *name, tApoQuantType type);
int setQuantBuffer(int start, unsigned char *buf, int maxSize, int *count);
tApoQuantType getQuantType(const char *name);


void addSubQuant(const char* name);
void addSubQuants(const char **names, int len);
void removeSubQuant(const char *name);
void removeSubQuants(const char **names, int len);
int getSubQuants(char ***out);

void getChangedQuants(tApoClnt_Quantity *A, int lenA,
                      char **B,             int lenB,
                      char ***reSubQuants,  int *len1,
                      char ***removeQuants, int *len2);



#ifdef __cplusplus
}
#endif

#endif

