#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <assert.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <getopt.h>
#include <errno.h>
#include <stdint.h>
#include "grabber.h"

using namespace std;

static void sig_int_handler(int s)
{
    grabber_close();
    while (grabber_running())
    {
        sleep(1);
    }

    exit(0);
}

static void errno_exit(const char *s)
{
    fprintf(stderr, "%s error %d, %s\n", s, errno, strerror(errno));
    exit(EXIT_FAILURE);
}

static void usage(FILE *fp, int argc, char **argv)
{
    fprintf(fp,
         "Usage: %s [options]\n\n"
         "Options:\n"
         "-d | --device index  Video device index\n"
         "-x | --xres          X resolution\n"
         "-y | --yres          Y resolution\n"
         "-l | --lz4           Use LZ4 compression\n"
         "-j | --jpeg          Use JPEG compression (default)\n"
         "-h | --help          Print this message\n"
         "",
         argv[0]);
}

static const char short_options[] = "d:x:y:ljh";

static const struct option long_options[] = {
    { "device", required_argument, NULL, 'd' },
    { "xres",   required_argument, NULL, 'x' },
    { "yres",   required_argument, NULL, 'y' },
    { "lz4",    required_argument, NULL, 'l' },
    { "jpeg",   required_argument, NULL, 'j' },
    { "help",   no_argument,       NULL, 'h' },
    { 0,        0,                 0,    0 }
};

int main(int argc, char** argv)
{
    int32_t device = -1;
    int32_t xres = -1;
    int32_t yres = -1;
    compression_t comp_type = CompNone;

    for (;;)
    {
        int idx;
        int c;

        c = getopt_long(argc, argv,
                        short_options, long_options, &idx);

        if (-1 == c)
        {
            break;
        }

        switch (c)
        {
            case 'd':
                errno = 0;
                device = strtol(optarg, NULL, 10);
                if (errno)
                {
                    errno_exit(optarg);
                }
                break;

            case 'x':
                errno = 0;
                xres = strtol(optarg, NULL, 10);
                if (errno)
                {
                    errno_exit(optarg);
                }
                break;

            case 'y':
                errno = 0;
                yres = strtol(optarg, NULL, 10);
                if (errno)
                {
                    errno_exit(optarg);
                }
                break;

            case 'l':
                comp_type = CompLZ4;
                break;

            case 'j':
                comp_type = CompJPEG;
                break;

            case 'h':
                usage(stdout, argc, argv);
                exit(EXIT_SUCCESS);

            default:
                usage(stderr, argc, argv);
                exit(EXIT_FAILURE);
        }
    }

    if ((device < 0) ||
        (xres <= 0) ||
        (yres <= 0))
    {
        usage(stderr, argc, argv);
        exit(EXIT_FAILURE);
    }


    if (grabber_open(device, xres, yres, comp_type))
    {
        struct sigaction sigIntHandler;
        sigIntHandler.sa_handler = sig_int_handler;
        sigemptyset(&sigIntHandler.sa_mask);
        sigIntHandler.sa_flags = 0;
        sigaction(SIGINT, &sigIntHandler, NULL);

        pause();
    }
}
