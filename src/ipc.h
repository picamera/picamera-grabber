#ifndef IPC_H
#define IPC_H

#include <semaphore.h>

typedef struct
{
    char* name;
    int permissions;
    int size;
    int memory_fd;
    void *memory;
} ipc_mem_t;

typedef struct
{
    char* name;
    sem_t *semaphore;
    int permissions;
} ipc_sem_t;

void* ipc_mem_get(const ipc_mem_t* ipc);
ipc_mem_t* ipc_mem_bringup(const char* name, int permissions, int size);
bool ipc_mem_bringdown(ipc_mem_t* ipc);

ipc_sem_t* ipc_sem_bringup(const char* name, int permissions, int initial);
bool ipc_sem_bringdown(ipc_sem_t* ipc);
sem_t* ipc_sem_get(const ipc_sem_t* ipc);

#endif
