

#include <vector>
#include <string>
#include <iostream>
#include <unordered_map>
#include <set>

#include "UAQMgr.h"



typedef struct 
{
    uint32_t name_len;
    char *name;
    tApoQuantType type;
}quantData;

std::vector<quantData> quList;
std::unordered_map<std::string, tApoQuantType> quDict;

pthread_mutex_t quants_mutex = PTHREAD_MUTEX_INITIALIZER;

void clearQuants()
{
    pthread_mutex_lock(&quants_mutex);
    quList.clear();
    quDict.clear();
    pthread_mutex_unlock(&quants_mutex);
}

void registerQuant(const char* name, tApoQuantType type)
{
    pthread_mutex_lock(&quants_mutex);
    quantData qu;
    qu.type = type;
    qu.name_len = strlen(name);
    qu.name = (char *)malloc(qu.name_len+1);
    memcpy(qu.name, name, qu.name_len);
    qu.name[qu.name_len] = '\0';
    quList.push_back(qu);

    quDict[name] = type;
    pthread_mutex_unlock(&quants_mutex);
}

tApoQuantType getQuantType(const char *name)
{
    auto it = quDict.find(name);
    if(it != quDict.end())
    {
        return it->second;
    }
    return ApoVoidType;
}


int setQuantBuffer(int start, unsigned char *buf, int maxSize, int *count)
{
    pthread_mutex_lock(&quants_mutex);
    int offset = 0;
    int i = start;
    for (; i < quList.size(); ++i) {
        quantData &qu = quList[i];
        uint32_t totalSize = sizeof(qu.name_len) + qu.name_len + sizeof(qu.type);

        // 检查是否有足够的空间来存储整个结构体
        if (offset + totalSize > maxSize) {
            break;
        }
        memcpy(buf + offset, &qu.name_len, sizeof(qu.name_len));
        offset += sizeof(qu.name_len);
        memcpy(buf + offset, qu.name, qu.name_len);
        offset += qu.name_len;
        memcpy(buf + offset, &qu.type, sizeof(qu.type));
        offset += sizeof(qu.type);
    }
    *count = i;
    pthread_mutex_unlock(&quants_mutex);
    return offset;
}

//----------------------------subQuants相关-------------------------------

std::set<std::string> subQuants;
pthread_mutex_t subQuants_mutex = PTHREAD_MUTEX_INITIALIZER;
static int checkStrValid(const char *src, std::string &dest)
{
    if(src == NULL)
        return -1;
    std::string s(src);
    if(s.empty())
        return -2;
    dest = s;
    return 0;
}

void addSubQuant(const char* name)
{
    const char *names[] = { name };
    addSubQuants(names, 1);
}

void addSubQuants(const char **names, int len)
{
    pthread_mutex_lock(&subQuants_mutex);
    if (names == nullptr) {
        std::cerr << "Error: names array is null" << std::endl;
        pthread_mutex_unlock(&subQuants_mutex);
        return;
    }
    for (int i = 0; i < len; i++) {
        std::string name;
        if(checkStrValid(names[i], name) == 0)
        {
            subQuants.insert(name);
        }
    }
    pthread_mutex_unlock(&subQuants_mutex);
}

void removeSubQuant(const char *name)
{
    const char *names[] = { name };
    removeSubQuants(names, 1);
}

void removeSubQuants(const char **names, int len)
{
    pthread_mutex_lock(&subQuants_mutex);
    if (names == nullptr) {
        std::cerr << "Error: names array is null" << std::endl;
        pthread_mutex_unlock(&subQuants_mutex);
        return;
    }
    for (int i = 0; i < len; i++) {
        std::string name;
        if(checkStrValid(names[i], name) == 0)
            subQuants.erase(name);
    }
    pthread_mutex_unlock(&subQuants_mutex);
}


int getSubQuants(char ***out)
{
    pthread_mutex_lock(&subQuants_mutex);
    // 确定集合的大小
    size_t size = subQuants.size();

    // 分配char**数组
    *out = (char**)malloc(size * sizeof(char*));
    if (*out == nullptr) {
        pthread_mutex_unlock(&subQuants_mutex);
        return -1; // 分配失败
    }

    // 分配每个字符串的内存并复制字符串
    size_t index = 0;
    for (const auto &quant : subQuants) {
        (*out)[index] = (char*)malloc((quant.size() + 1) * sizeof(char)); // +1 for null terminator
        if ((*out)[index] == nullptr) {
            // 释放已分配的内存
            for (size_t j = 0; j < index; ++j) {
                free((*out)[j]);
            }
            free(*out);
            pthread_mutex_unlock(&subQuants_mutex);
            return -1; // 分配失败
        }
        strcpy((*out)[index], quant.c_str());
        index++;
    }
    pthread_mutex_unlock(&subQuants_mutex);
    return size; // 返回数组的大小
}

void getChangedQuants(tApoClnt_Quantity *A, int lenA,
                      char **B,             int lenB,
                      char ***reSubQuants,  int *len1,
                      char ***removeQuants, int *len2)
{
    pthread_mutex_lock(&subQuants_mutex);
    std::set<std::string> setA;
    for(int i = 0;i<lenA;i++)
    {
        setA.insert(A[i].Name);
    }
    std::set<std::string> setC = subQuants;
    std::set<std::string> setBC(B, B + lenB);
    setBC.insert(setC.begin(), setC.end());

    std::vector<std::string> toAdd;
    std::vector<std::string> toRemove;

    // Find elements in setBC but not in setA
    for (const auto &item : setBC) {
        if (setA.find(item) == setA.end()) {
            toAdd.push_back(item);
        }
    }

    // Find elements in setA but not in setBC
    for (const auto &item : setA) {
        if (setBC.find(item) == setBC.end()) {
            toRemove.push_back(item);
        }
    }

    // Allocate memory for reSubQuants and removeQuants
    *len1 = toAdd.size();
    *len2 = toRemove.size();

    *reSubQuants = (char **)malloc(*len1 * sizeof(char *));
    *removeQuants = (char **)malloc(*len2 * sizeof(char *));

    for (int i = 0; i < *len1; i++) {
        (*reSubQuants)[i] = strdup(toAdd[i].c_str());
    }

    for (int i = 0; i < *len2; i++) {
        (*removeQuants)[i] = strdup(toRemove[i].c_str());
    }

    pthread_mutex_unlock(&subQuants_mutex);

}




