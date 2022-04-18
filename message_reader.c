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

    slot_filepath = argv[1];
    channel_id = atoi(argv[2]);
    
    fs = open(slot_filepath, O_RDWR);
    res = ioctl(fs, MSG_SLOT_CHANNEL, channel_id);
    if (res != 0)
    {
        printf("reader's ioctl got %d\n", res);
        return res;
    }

    msg_len = read(fs, msg, 128);


    res = write(1, msg, msg_len);

    return 0;
}