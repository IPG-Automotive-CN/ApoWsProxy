#include <pthread.h>
#include <stdio.h>

extern "C"
{
    #include "ApoClntDemo.h"
}

int ac_ = 0;
char **av_ = NULL;

void* apo_client(void *args) 
{
    runAPO(ac_, av_);
}


int main(int ac, char **av)
{
    ac_ = ac;
    av_ = av;
    pthread_t apo_thread;
    pthread_create(&apo_thread, NULL, apo_client, NULL);

    printf("Main thread\n");
    while (1)
    {
        /* code */
    }
    return 0;
}
