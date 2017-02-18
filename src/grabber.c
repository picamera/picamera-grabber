#include <opencv2/opencv.hpp>
#include <time.h>
#include <iostream>
#include <stdio.h>
#include <stdint.h>
#include "ipc.h"
#include <semaphore.h>
#include "lz4.h"

using namespace std;
using namespace cv;

static VideoCapture cap;
static volatile bool can_run = false;
static volatile bool running = false;
static uint32_t xres = 0;
static uint32_t yres = 0;

#define NUM_FRAME_SLOTS 2

static uint32_t compress_frame(const uint8_t* src_data, unsigned long src_size, uint8_t* dest_buf, uint32_t max_dest_size)
{
    uint32_t compressed_size = LZ4_compress_limitedOutput((const char*)src_data, (char*)dest_buf, src_size, max_dest_size);
    if (compressed_size == 0)
    {
        printf("Compressing failed\n");
    }

    return compressed_size;
}

typedef struct
{
    uint32_t    memory_size;
    uint32_t    xres;
    uint32_t    yres;
    uint32_t    frames[NUM_FRAME_SLOTS];
    uint32_t    current_slot;
} mem_header_t;

typedef struct
{
    uint32_t    real_size;
    uint32_t    compressed_size;
    bool        compressed;
    uint8_t     packing[3]; // Unused explicit packing
} frame_header_t;

static void write_frame(Mat& frame, uint8_t* memory, uint32_t max_size)
{
    unsigned long frame_size = frame.total() * frame.elemSize();

    frame_header_t header;
    uint8_t* p = memory + sizeof(header);

    header.real_size = frame_size;

    uint32_t compressed_size = compress_frame(frame.data, frame_size, p, max_size);
    if (compressed_size == 0)
    {
        memcpy(p, frame.data, frame_size);
        header.compressed_size = frame_size;
        header.compressed = false;
    }
    else
    {
        header.compressed_size = compressed_size;
        header.compressed = true;
    }

    float change = (signed long)header.compressed_size - (signed long)header.real_size;
    change /= header.real_size;
    change *= 100.0;

    printf(" | %u -> %u (%0.1f%%)", header.real_size, header.compressed_size, change);

    p = memory;
    memcpy(p, &header, sizeof(header)); 
}

static void* grabber_thread(void *arg)
{
    running = true;

    clock_t last_time = clock();
    clock_t now_time = clock();
    int permissions = 777;

    printf("Bringing up IPC\n");
    ipc_sem_t* ipc_sem_notify = ipc_sem_bringup("/picamera_grabber_notifier", permissions, 0);
    if (ipc_sem_notify == NULL)
    {
        printf("IPC setup failure\n");
        return NULL;
    }

    ipc_sem_t* ipc_sem_generate = ipc_sem_bringup("/picamera_grabber_generate", permissions, 1);
    if (ipc_sem_generate == NULL)
    {
        printf("IPC setup failure\n");
        return NULL;
    }

    uint32_t est_frame_size = xres * yres * sizeof(int);
    uint32_t mem_size = sizeof(mem_header_t) + ((est_frame_size + sizeof(frame_header_t)) * NUM_FRAME_SLOTS);
    ipc_mem_t* ipc_mem = ipc_mem_bringup("/picamera_grabber_mem", permissions, mem_size);
    if (ipc_mem == NULL)
    {
        printf("IPC setup failure\n");
        return NULL;
    }

    sem_t *sem_notify = ipc_sem_get(ipc_sem_notify);
    sem_t *sem_generate = ipc_sem_get(ipc_sem_generate);
    void* memory = ipc_mem_get(ipc_mem);

    mem_header_t mem_header;
    mem_header.memory_size = mem_size;
    mem_header.xres = xres;
    mem_header.yres = yres;
    mem_header.current_slot = 0;
    mem_header.frames[0] = sizeof(frame_header_t) + sizeof(mem_header_t);
    for (uint32_t index=1; index<NUM_FRAME_SLOTS; index++)
    {
        mem_header.frames[index] = est_frame_size + mem_header.frames[index - 1];
    }
    memcpy(memory, &mem_header, sizeof(mem_header_t)); 

    uint8_t* frame_slots[NUM_FRAME_SLOTS];
    for (uint32_t index=0; index<NUM_FRAME_SLOTS; index++)
    {
        frame_slots[index] = (uint8_t*)memory + mem_header.frames[index];
    }

    printf("Grabbing is beginning!\n");

    uint32_t frame_index = 0;
    while (can_run)
    {
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_sec += 1;

        printf("%d+", frame_index);
        fflush(stdout);
        Mat frame;
        cap >> frame;
        write_frame(frame, frame_slots[frame_index], est_frame_size - sizeof(frame_header_t));

        if (!sem_timedwait(sem_generate, &ts))
        {
            fflush(stdout);
            if (!frame.empty())
            {
                now_time = clock();
                float diff = ((float)(now_time - last_time) / CLOCKS_PER_SEC);
                last_time = now_time;
                float fps = 1.0 / diff;
                printf(" | FPS: %0.2f", fps);
                
                mem_header.current_slot = frame_index;
                memcpy(memory, &mem_header, sizeof(mem_header_t)); 

                sem_post(sem_notify);
                ++frame_index;
                if (frame_index >= NUM_FRAME_SLOTS)
                {
                    frame_index = 0;
                }
            }

        }
        printf("\n");
    }

    printf("Bringing down IPC\n");
    ipc_sem_bringdown(ipc_sem_generate);
    ipc_sem_bringdown(ipc_sem_notify);
    ipc_mem_bringdown(ipc_mem);

    printf("Grabbing is stopping!\n");

    running = false;

    return NULL;
}

bool grabber_running(void)
{
    return running;
}

bool grabber_open(int index, int x_res, int y_res)
{
    if(!cap.open(index))
    {
        printf("Failed to open grabber\n");
        return false;
    }

    xres = x_res;
    yres = y_res;

    cap.set(CV_CAP_PROP_FRAME_WIDTH, xres);
    cap.set(CV_CAP_PROP_FRAME_HEIGHT, yres);

    can_run = true;

    pthread_t threadid;
    pthread_create(&threadid, NULL, grabber_thread, NULL);

    return true;
}

void grabber_close(void)
{
    can_run = false;
}
