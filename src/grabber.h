#ifndef GRABBER_H
#define GRABBER_H

typedef enum
{
    CompNone,
    CompLZ4,
    CompJPEG
} compression_t;

bool grabber_open(int index, int xres, int yres, compression_t type);
bool grabber_running(void);
void grabber_close(void);

#endif
