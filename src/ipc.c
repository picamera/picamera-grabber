#include <errno.h>
#include <semaphore.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdbool.h>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include "ipc.h"

void* ipc_mem_get(const ipc_mem_t* ipc)
{
    return ipc->memory;
}

bool ipc_mem_bringdown(ipc_mem_t* ipc)
{
    bool success = true;

    if (ipc->memory_fd >= 0)
    {
        int rc = munmap(ipc->memory, ipc->size);
        if (rc)
        {
            printf("Unmapping the memory failed; errno is %d\n", errno);
            success = false;
        }

        rc = close(ipc->memory_fd);
        if (rc)
        {
            printf( "Closing the memory's file descriptor failed; errno is %d\n", errno);
            success = false;
        }

        rc = shm_unlink(ipc->name);
        if (rc)
        {
            printf("Unlinking the memory failed; errno is %d\n", errno);
            success = false;
        }
    }

    free(ipc->name);
    free(ipc);

    return success;
}

ipc_mem_t* ipc_mem_bringup(const char* name, int permissions, int size)
{
    ipc_mem_t* ipc = (ipc_mem_t*)malloc(sizeof(ipc_mem_t)); 
    if (ipc == NULL)
    {
        return NULL;
    }

    size_t length = strlen(name);
    ipc->name = (char*)malloc(length);
    if (ipc->name == NULL)
    {
        return NULL;
    }

    strncpy(ipc->name, name, length);
    ipc->permissions = permissions;
    ipc->size = size;

    // We will unlink anyway incase the memory wasn't released property previously
    shm_unlink(ipc->name);

    ipc->memory_fd = shm_open(ipc->name, O_RDWR | O_CREAT | O_EXCL, permissions);
    if (ipc->memory_fd < 0)
    {
        printf("Unlinking the semaphore failed; errno is %d\n", errno);
        return NULL;
    }

    int rc = ftruncate(ipc->memory_fd, ipc->size);
    if (rc)
    {
        printf("Resizing the shared memory failed; errno is %d\n", errno);
        ipc_mem_bringdown(ipc);
        return NULL;
    }

    void* memory = mmap((void *)0, (size_t)ipc->size, PROT_READ | PROT_WRITE, MAP_SHARED, ipc->memory_fd, 0);
    if (memory == MAP_FAILED)
    {
        printf("MMapping the shared memory failed; errno is %d\n", errno);
        ipc_mem_bringdown(ipc);
        return NULL;
    }
    ipc->memory = memory;

    return ipc;
}

sem_t* ipc_sem_get(const ipc_sem_t* ipc)
{
    return ipc->semaphore;
}

bool ipc_sem_bringdown(ipc_sem_t* ipc)
{
    bool success = true;

    if (ipc->semaphore)
    {
        int rc = sem_close(ipc->semaphore);
        if (rc)
        {
            printf("Closing the semaphore failed; errno is %d\n", errno);
            success = false;
        }

        rc = sem_unlink(ipc->name);
        if (rc)
        {
            printf("Unlinking the semaphore failed; errno is %d\n", errno);
            success = false;
        }
    }
    free(ipc->name);
    free(ipc);

    return success;
}

ipc_sem_t* ipc_sem_bringup(const char* name, int permissions, int initial)
{
    ipc_sem_t* ipc = (ipc_sem_t*)malloc(sizeof(ipc_sem_t)); 
    if (ipc == NULL)
    {
        return NULL;
    }

    size_t length = strlen(name);
    ipc->name = (char*)malloc(strlen(name));
    if (ipc->name == NULL)
    {
        free(ipc);
        return NULL;
    }

    strncpy(ipc->name, name, length);
    ipc->permissions = permissions;

    // We will unlink anyway incase the semaphore wasn't released property previously
    sem_unlink(ipc->name);

    sem_t* semaphore = sem_open(ipc->name, O_CREAT, ipc->permissions, initial);
    if (semaphore == SEM_FAILED)
    {
        printf("Creating the semaphore failed; errno is %d\n", errno);
        ipc_sem_bringdown(ipc);
        return NULL;
    }
    ipc->semaphore = semaphore;

    return ipc;
}
