#undef __KERNEL__
#define __KERNEL__
#undef MODULE
#define MODULE


#include <linux/kernel.h>   
#include <linux/module.h>   
#include <linux/fs.h>       
#include <linux/uaccess.h>  
#include <linux/string.h>   
#include <linux/slab.h>

MODULE_LICENSE("GPL");

#include "message_slot.h"

#define DEVICE_RANGE_NAME "ofir_dev"


//================== DATA STRUCTURES & ==========================
//================== HELPER FUNCTIONS  ==========================

typedef struct channel_node 
{
  unsigned long id;
  int data_len;
  char data[128];
  struct channel_node *next;
} channel_node;


typedef struct msg_slot 
{
  unsigned long channel;
  channel_node *head;
} msg_slot;

msg_slot all_slots[257];


static int new_channel_node(unsigned long id, channel_node **node_dbl_ptr)
{
  *node_dbl_ptr = (channel_node *)kmalloc(sizeof(channel_node), GFP_KERNEL);

  if (*node_dbl_ptr == NULL)
  {
    // printk("message_slot: kmalloc did not work (ne_channel_node)\n");
    return -1;
  }
  
  (*node_dbl_ptr)->id = id;
  (*node_dbl_ptr)->data_len = 0;
  (*node_dbl_ptr)->next = NULL;

  return 0;
}


static int get_curr_channel_node(struct file* file, channel_node **node_dbl_ptr)
{
  msg_slot *slot;
  channel_node *node_ptr;
  
  slot = file->private_data;
  if (slot == NULL)
  {
    // printk("message_slot: (get_curr_channel) fs does not contain slot info\n");
    return -1;
  }
  
  node_ptr = slot->head;
  while (node_ptr != NULL  &&  node_ptr->id != slot->channel)
    node_ptr = node_ptr->next;

  if (node_ptr == NULL)
    return -1;

  *(node_dbl_ptr) = node_ptr;

  return 0;
}


//================== DEVICE FUNCTIONS ===========================
static int device_open( struct inode* inode,
                        struct file*  file )
{
  // // printk("message_slot: open started\n")
  if (iminor(inode) > 256)
    return -1;

  // // printk("message_slot (open): iminor is %d\n", iminor(inode));
  file->private_data = &(all_slots[iminor(inode)]);

  
  // // printk("message_slot: open ended successfully\n");
  
  return 0;
}

//---------------------------------------------------------------
static int device_release( struct inode* inode,
                           struct file*  file)
{
  file->private_data = NULL;
  return 0;
}

//---------------------------------------------------------------
static ssize_t device_read( struct file* file,
                            char __user* buffer,
                            size_t       length,
                            loff_t*      offset )
{
  channel_node *tmp;
  int code;
  
  // validate user parameters
  if (length < 1  ||  length > 128  ||  buffer == NULL)
  {
    // printk("message_slot: (read) invalid user parameters\n");
    return -EINVAL;
  }

  // get current channel and validate its data
  code = get_curr_channel_node(file, &tmp);
  if (code != 0)
  {
    // printk("message_slot: (read) problem accessing channel\n");
    return -EINVAL;
  }
  if (tmp->data_len == 0)
  {
    // printk("message_slot: (read) channel has no message\n");
    return -EWOULDBLOCK;
  }
  if (tmp->data_len > length)
  {
    // printk("message_slot: (read) buffer to small for msg\n");
    return -ENOSPC;
  }

  // finally, read message
  if (copy_to_user(buffer, tmp->data, tmp->data_len) != 0)
  {
    // printk("message_slot: copy_to_user failed. data_len=%d\n", tmp->data_len);
    return -EAGAIN;
  }
  
  return tmp->data_len;
}

//---------------------------------------------------------------
static ssize_t device_write( struct file*       file,
                             const char __user* buffer,
                             size_t             length,
                             loff_t*            offset)
{
  channel_node *tmp;
  int code;
    
  // validate user parameters
  if (buffer == NULL)
  {
    // printk("message_slot: (write) invalid user parameters\n");
    return -EINVAL;
  }
  if (length == 0  ||  length > 128)
  {
    // printk("message_slot: (write) length is bad!!\n");
    return -EMSGSIZE;
  }

  // retrieve current channel
  code = get_curr_channel_node(file, &tmp);
  if (code != 0)
    return -EINVAL;


  // write into the channel
  if (copy_from_user(tmp->data, buffer, length) != 0)
  {
    // printk("message_slot: (write) copy_from_user failed\n");
    return -EAGAIN;
  }
  tmp->data_len = length;
  

  // return the number of input characters used
  return length;
}

//----------------------------------------------------------------
static long device_ioctl( struct   file* file,
                          unsigned int   ioctl_command_id,
                          unsigned long  ioctl_param )
{
  channel_node *tmp;
  int res;

  if (ioctl_command_id != MSG_SLOT_CHANNEL  ||  ioctl_param < 1)
  {
    // printk("message_slot (ioctl):  wrong cmd_id OR invalid channel id\n");
    return -EINVAL;
  }

  if (file->private_data == NULL)
  {
    // printk("message_slot: (ioctl) INVALID file\n");
    return -3;
  }


  // set the channel
  ((msg_slot *)(file->private_data))->channel = ioctl_param;

  // create the channel node, or verify it exists
  tmp = ((msg_slot *)(file->private_data))->head;

  if (tmp == NULL)
  {
    // attempt to create a new channel_node
    res = new_channel_node(ioctl_param, &tmp);
    if (res != 0)
    {
      // printk("message_slot: (ioctl) kmalloc failed\n");
      return -1;
    }

    ((msg_slot *)(file->private_data))->head = tmp;
    return 0;
  }

  while (tmp->next != NULL)
  {
    if (tmp->id == ioctl_param)
      return 0;
    tmp = tmp->next;
  }

  // attempt to create a new channel_node
  res = new_channel_node(ioctl_param, &(tmp->next));
  if (res != 0)
  {
    // printk("message_slot: (ioctl) kmalloc failed\n");
    return -1;
  }

  return 0;
}

//==================== DEVICE SETUP =============================

struct file_operations Fops =
{
  .owner	  = THIS_MODULE, 
  .read           = device_read,
  .write          = device_write,
  .open           = device_open,
  .unlocked_ioctl = device_ioctl,
  .release        = device_release,
};

//---------------------------------------------------------------
static int __init driver_init(void)
{  
  int rc;

  // printk("message_slot init\n");  

  rc = register_chrdev( MAJOR_NUM, DEVICE_RANGE_NAME, &Fops );
  
  if (rc < 0)
    printk(KERN_ERR "message_slot init failed\n");
  

  return rc;
}

//---------------------------------------------------------------
static void __exit driver_cleanup(void)
{
  channel_node *tmp1, *tmp2;
  int i;
  
  // printk("message_slot cleanup\n");

  // free all allocated memory
  for (i = 0; i < 257; i++)
  {
    if (all_slots[i].head != NULL)
    {
      tmp1 = all_slots[i].head;
      tmp2 = tmp1->next;
      kfree(tmp1);

      while (tmp2 != NULL)
      {
        tmp1 = tmp2;
        tmp2 = tmp1->next;
        kfree(tmp1);
      }
    }
  }

  // printk("message_slot cleanup - freed memory\n");

  unregister_chrdev(MAJOR_NUM, DEVICE_RANGE_NAME);
}

//---------------------------------------------------------------
module_init(driver_init);
module_exit(driver_cleanup);

//========================= END OF FILE =========================
