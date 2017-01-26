#ifndef GRABBER_H
#define GRABBER_H

bool grabber_open(int index, int xres, int yres);
bool grabber_running(void);
void grabber_close(void);

#endif
