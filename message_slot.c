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


//================== DATA STRUCTURES  ===========================

typedef struct channel_node 
{
  int id;
  int data_len;
  char data[128];
  struct channel_node *next;
} channel_node;


typedef struct msg_slot 
{
  int channel;
  channel_node *head;
} msg_slot;

msg_slot all_slots[257];


static channel_node* new_channel_node(int id)
{
  channel_node *node = (channel_node *)kmalloc(sizeof(channel_node), GFP_KERNEL);

  if (node == NULL)
    printk("message_slot: kmalloc did not work (ne_channel_node)\n");
  
  node->id = id;
  node->data_len = 0;
  node->next = NULL;

  return node;
}


static int get_curr_channel_node(struct file* file, channel_node **node_dbl_ptr)
{
  msg_slot *slot;
  channel_node *node_ptr;
  
  slot = file->private_data;
  if (slot == NULL)
    return -1;
  
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
  printk("message_slot: open started\n");
  if (iminor(inode) > 256)
    return -1;

  file->private_data = &(all_slots[iminor(inode)]);

  
  printk("message_slot: open ended successfully\n");
  
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
  
  code = get_curr_channel_node(file, &tmp);
  if (code != 0)
    return code;

  if (tmp->data_len == 0)
  {
    return -EWOULDBLOCK;
  }
  if (tmp->data_len > length)
  {
    return -ENOSPC;
  }

  if (copy_to_user(buffer, tmp->data, tmp->data_len) != 0)
  {
    printk("message_slot: copy_to_user failed. data_len=%d\n", tmp->data_len);
    return -1;
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
  
  code = get_curr_channel_node(file, &tmp);
  if (code != 0)
    return code;

  printk("message_slot: 'write' found curr channel node\n");

  if (length == 0  ||  length > 128)
  {
    printk("message_slot: 'write' length is bad!!\n");
    return -EMSGSIZE;
  }

  if (copy_from_user(tmp->data, buffer, length) != 0)
  {
    printk("message_slot: copy_from_user failed\n");
    return -1;
  }
  tmp->data_len = length;
  
  printk("message_slot: 'write' writing completed\n");

  // return the number of input characters used
  return length;
}

//----------------------------------------------------------------
static long device_ioctl( struct   file* file,
                          unsigned int   ioctl_command_id,
                          unsigned long  ioctl_param )
{
  channel_node *tmp;

  if (ioctl_command_id != MSG_SLOT_CHANNEL)
  {
    printk("message_slot: ioctl WRONG cmd_id\n");
    return -555;
  }


  if (ioctl_param < 1)
  {
    printk("message_slot: ioctl INVALID channel number\n");
    return -2;
  }

  if (file->private_data == NULL)
  {
    printk("message_slot: ioctl INVALID file\n");
    return -3;
  }


  // set the channel
  ((msg_slot *)(file->private_data))->channel = ioctl_param;

  // create the channel node, or verify it exists
  tmp = ((msg_slot *)(file->private_data))->head;

  if (tmp == NULL)
  {
    tmp = 
    ((msg_slot *)(file->private_data))->head = new_channel_node(ioctl_param);
    return 0;
  }

  while (tmp->next != NULL)
  {
    if (tmp->id == ioctl_param)
      return 0;
    tmp = tmp->next;
  }

  tmp->next = new_channel_node(ioctl_param);

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
  printk("message_slot init\n");  
  rc = register_chrdev( MAJOR_NUM, DEVICE_RANGE_NAME, &Fops );
  // printk("message_slot init ended\n");  
  // printk("message_slot init: ioctl is %lu\n", MSG_SLOT_CHANNEL);  
  return rc;
}

//---------------------------------------------------------------
static void __exit driver_cleanup(void)
{
  printk("message_slot cleanup\n");
  unregister_chrdev(MAJOR_NUM, DEVICE_RANGE_NAME);
}

//---------------------------------------------------------------
module_init(driver_init);
module_exit(driver_cleanup);

//========================= END OF FILE =========================
