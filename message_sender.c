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

    slot_filepath = argv[1];
    channel_id = atoi(argv[2]);
    msg = argv[3];
    msg_len = strlen(msg) + 1;

    fs = open(slot_filepath, O_RDWR);
    printf("open done\n");
    
    res = ioctl(fs, MSG_SLOT_CHANNEL, channel_id);
    printf("ioctl done\n");
    
    res = write(fs, msg, msg_len);
    printf("write done\n");
    
    return 0;
}