#include <string.h>
#include <stdlib.h>
#include <stdio.h> 
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include "message_slot.h"



int main(int argc, char *argv[])
{
    char *slot_filepath;
    char msg[128];
    int channel_id;
    int msg_len;
    int fs;
    int res;

    // validate num of arguments
    if (argc != 3)
    {
        printf("Error: %s\n", strerror(EINVAL));
        return 1;
    }

    slot_filepath = argv[1];
    channel_id = atoi(argv[2]);
    
    // open
    fs = open(slot_filepath, O_RDWR);
    if (fs == -1)
    {
        printf("Error: %s\n", strerror(errno));
        return 1;
    }

    // set channel
    res = ioctl(fs, MSG_SLOT_CHANNEL, channel_id);
    if (res != 0)
    {
        printf("Error: %s\n", strerror(errno));
        return 1;
    }
    
    // read
    msg_len = read(fs, msg, 128);
    if (msg_len <= 0)
    {
        printf("Error: %s\n", strerror(errno));
        return 1;
    }

    // print msg
    res = write(1, msg, msg_len);
    if (res != msg_len)
    {
        printf("Error: %s\n", strerror(errno));
        return 1;
    }

    // close device
    if (close(fs) != 0)
    {
        printf("Error: %s\n", strerror(errno));
        return 1;
    }

    return 0;
}