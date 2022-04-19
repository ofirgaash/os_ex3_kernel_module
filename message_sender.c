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
    char *msg;
    int channel_id;
    int msg_len;
    int fs;
    int res;

    // validate num of arguments
    if (argc != 4)
    {
        printf("Error: %s\n", strerror(EINVAL));
        return 1;
    }

    slot_filepath = argv[1];
    channel_id = atoi(argv[2]);
    msg = argv[3];
    msg_len = strlen(msg);

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
    
    // write
    res = write(fs, msg, msg_len);
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