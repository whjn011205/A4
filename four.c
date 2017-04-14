#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <asm/uaccess.h>
#include <linux/ioctl.h>
#include <linux/file.h>


#define MAJOR_NUMBER 61

#define DEV_NAME "four"
#define NUM_BYTES 4194304
#define MSG_SIZE 60

#define SCULL_IOC_MAGIC 'k'
#define SCULL_IOC_MAXNR 4
#define SCULL_HELLO _IO(SCULL_IOC_MAGIC, 1)
#define SCULL_IOCSQSET _IOW(SCULL_IOC_MAGIC, 2, char*)
#define SCULL_IOCGQSET _IOR(SCULL_IOC_MAGIC, 3, char*)
#define SCULL_IOCXQSET _IOWR(SCULL_IOC_MAGIC, 4, char*)

/* forward declaration */
int four_open(struct inode *inode, struct file *filp);
int four_release(struct inode *inode, struct file *filp);
ssize_t four_read(struct file *filp, char *buf, size_t count, loff_t *f_pos);
ssize_t four_write(struct file *filp, const char *buf,size_t count, loff_t *f_pos);
loff_t four_llseek(struct file *filp, loff_t off, int whence);
static void four_exit(void);
long four_ioctl(struct file *filp, unsigned int cmd, unsigned long arg); 

/* definition of file_operation structure */
struct file_operations four_fops = {
	read: four_read,
	write: four_write,
	open: four_open,
	release: four_release,
	llseek: four_llseek,
	unlocked_ioctl : four_ioctl
};

char *four_data = NULL;
char *dev_msg = NULL;
long long int cur_pos = 0;

static int four_init(void)
{
	int result;
	// register the device
	result = register_chrdev(MAJOR_NUMBER, "four",&four_fops);
	if (result < 0) {
		return result;
	}

	// allocate 4MB of memory for storage
	// kmalloc is just like malloc, the second parameter is
	// the type of memory to be allocated.
	// To release the memory allocated by kmalloc, use kfree.
	four_data = kmalloc(sizeof(char)*NUM_BYTES, GFP_KERNEL);
	if (!four_data) {
		four_exit();
		// cannot allocate memory
		// return no memory error, negative signify a failure
		return -ENOMEM;
	}

	// allocate memory for the message
	dev_msg = kmalloc(MSG_SIZE, GFP_KERNEL);
	if(!dev_msg){
	 four_exit();
	 return -ENOMEM;
	}

	printk(KERN_ALERT "This is a 4MB device module\n");
	return 0;
}

static void four_exit(void)
{
	// if the pointer is pointing to something
	if (four_data) {
		// free the memory and assign the pointer to NULL
		kfree(four_data);
		four_data = NULL;
	}
	if(dev_msg){
		kfree(dev_msg);
		dev_msg = NULL;
     	}
	// unregister the device
	unregister_chrdev(MAJOR_NUMBER, "four");
	printk(KERN_ALERT "four device module is unloaded\n");
}

int four_open(struct inode *inode, struct file *filp)
{
	return 0; // always successful
}

int four_release(struct inode *inode, struct file *filp)
{
	return 0; // always successful
}

long four_ioctl(struct file *filp, unsigned int cmd, unsigned long arg){
    
	int err = 0;
	int retval = 0;
	char *tmp_msg = NULL;

	if(_IOC_TYPE(cmd) != SCULL_IOC_MAGIC) return -ENOTTY;
	if(_IOC_NR(cmd) > SCULL_IOC_MAXNR) return -ENOTTY;

	if (_IOC_DIR(cmd) & _IOC_READ)
		err = !access_ok(VERIFY_WRITE, (void *)arg, _IOC_SIZE(cmd));
	else if (_IOC_DIR(cmd) & _IOC_WRITE)
		err = !access_ok(VERIFY_READ, (void *)arg, _IOC_SIZE(cmd));
	if (err) 
		return -EFAULT;

	switch(cmd){
		case SCULL_HELLO:
		    printk(KERN_WARNING "ioctl: hello\n");
		    break;
		case SCULL_IOCSQSET:
		    if(copy_from_user(dev_msg, (char *)arg, MSG_SIZE)){
		        return -EFAULT;
		    }
		    printk(KERN_ALERT "IOW set dev_msg: %s", dev_msg);
		    break;
		case SCULL_IOCGQSET:
		    if(copy_to_user((char *)arg, dev_msg, MSG_SIZE)){
		        return -EFAULT;
		    }
		    printk(KERN_ALERT "IOR copy dev_msg: %s", dev_msg);
		    break;

		case SCULL_IOCXQSET:
		    tmp_msg = kmalloc(sizeof(char)*MSG_SIZE, GFP_KERNEL);
		        if(copy_from_user(tmp_msg, (char *)arg,  MSG_SIZE)){
		        return -EFAULT;
		    }

		    if(copy_to_user((char *)arg, dev_msg, MSG_SIZE)){
		        return -EFAULT;
		    }
		    strcpy(dev_msg, tmp_msg);
		    printk(KERN_ALERT "IOWR dev_msg: %s\n", dev_msg);
		    kfree(tmp_msg);
		    break;
		default:
		    return -ENOTTY;
	}

	return retval;
}


loff_t four_llseek(struct file* filp, loff_t off, int whence) {
	
	loff_t newpos;
	

	switch(whence) {
		case 0: // SEEK_SET
		newpos = off;
		break;

		case 1: // SEEK_CUR
		newpos = filp->f_pos + off;
		break;

		case 2: // SEEK_END
		newpos = cur_pos + off; // from end
		break;

		default: // invalid argument
		return -EINVAL;
	}

	if (newpos < 0 || newpos > cur_pos )
		return -EINVAL;

	filp->f_pos = newpos;
	return newpos;
}

ssize_t four_read(struct file *filp, char *buf, size_t count, loff_t *f_pos)
{
	ssize_t result = 0;
	unsigned int read_count=0;

        if(*f_pos >= NUM_BYTES || *f_pos<0 || four_data[*f_pos] == '\0')
		printk(KERN_INFO "%s: end of file\n", DEV_NAME);
		return result;

	read_count = strnlen(four_data, NUM_BYTES);
	if(*f_pos + count > read_count)
		count = read_count - *f_pos; 
	

	if(copy_to_user(buf, four_data+*f_pos, count)) {
		printk(KERN_WARNING "%s: read unsuccessful\n", DEV_NAME);
		return -EFAULT;
	}

	*f_pos += count;
	result = count;
	cur_pos -= count;
	if( cur_pos<0 )
		cur_pos =0;
	
	printk(KERN_INFO "%s: read done\n", DEV_NAME);
	return result;

}

ssize_t four_write(struct file *filp, const char *buf,size_t count, loff_t *f_pos)
{
	
	ssize_t result = 0;

	if (*f_pos >= NUM_BYTES || *f_pos < 0) {
		printk(KERN_INFO "%s: end of file\n", DEV_NAME);
		return -EINVAL;
	}

	if (*f_pos + count > NUM_BYTES)
		count = NUM_BYTES - *f_pos;

	if(copy_from_user(four_data+*f_pos, buf, count)) {
		printk(KERN_WARNING "%s: write unsuccessful\n", DEV_NAME);
		return -EFAULT;
	}

	*f_pos += count;
	result = count;
	cur_pos += count;
	if( cur_pos > NUM_BYTES)
		cur_pos = NUM_BYTES;

	if(*f_pos < NUM_BYTES)
		four_data[*f_pos] = '\0';
		
	printk(KERN_INFO "%s: write done\n", DEV_NAME);
	return result;

}


MODULE_LICENSE("GPL");
module_init(four_init);
module_exit(four_exit);


