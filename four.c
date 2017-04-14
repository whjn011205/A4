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

#define DEVICE_NAME "fourmb_device"
#define DEVICE_SIZE 4194304
#define DEV_MSG_SIZE 60
#define MAJOR_NUMBER 61

#define SCULL_IOC_MAGIC 'k'
#define SCULL_IOC_MAXNR 4
#define SCULL_HELLO _IO(SCULL_IOC_MAGIC, 1)
#define SET_DEV_MSG _IOW(SCULL_IOC_MAGIC, 2, char*)
#define GET_DEV_MSG _IOR(SCULL_IOC_MAGIC, 3, char*)
#define WR_DEV_MSG _IOWR(SCULL_IOC_MAGIC, 4, char*)

/* forward declaration */
int fourmb_device_open(struct inode *inode, struct file *filp);
int fourmb_device_release(struct inode *inode, struct file *filp);
ssize_t fourmb_device_read(struct file *filp, char *buf, size_t count, loff_t *f_pos);
ssize_t fourmb_device_write(struct file *filp, const char *buf,size_t count, loff_t *f_pos);
loff_t fourmb_device_llseek(struct file *filp, loff_t off, int whence);
static void fourmb_device_exit(void);
long fourmb_device_ioctl(struct file *filp, unsigned int cmd, unsigned long arg); 

/* definition of file_operation structure */
struct file_operations fourmb_device_fops = {
	read: fourmb_device_read,
	write: fourmb_device_write,
	open: fourmb_device_open,
	release: fourmb_device_release,
	llseek: fourmb_device_llseek,
	unlocked_ioctl : fourmb_device_ioctl
};

char *fourmb_device_data = NULL;
char *dev_msg = NULL;
long long int cur_size = 0;

static int fourmb_device_init(void)
{
	int result;
	// register the device
	result = register_chrdev(MAJOR_NUMBER, "fourmb_device",&fourmb_device_fops);
	if (result < 0) {
		return result;
	}

	// allocate 4MB of memory for storage
	// kmalloc is just like malloc, the second parameter is
	// the type of memory to be allocated.
	// To release the memory allocated by kmalloc, use kfree.
	fourmb_device_data = kmalloc(sizeof(char)*DEVICE_SIZE, GFP_KERNEL);
	if (!fourmb_device_data) {
		fourmb_device_exit();
		// cannot allocate memory
		// return no memory error, negative signify a failure
		return -ENOMEM;
	}

	// allocate memory for the message
	dev_msg = kmalloc(DEV_MSG_SIZE, GFP_KERNEL);
	if(!dev_msg){
	 fourmb_device_exit();
	 return -ENOMEM;
	}

	printk(KERN_ALERT "This is a fourmb_device device module\n");
	return 0;
}

static void fourmb_device_exit(void)
{
	// if the pointer is pointing to something
	if (fourmb_device_data) {
		// free the memory and assign the pointer to NULL
		kfree(fourmb_device_data);
		fourmb_device_data = NULL;
	}
	if(dev_msg){
		kfree(dev_msg);
		dev_msg = NULL;
     	}
	// unregister the device
	unregister_chrdev(MAJOR_NUMBER, "fourmb_device");
	printk(KERN_ALERT "fourmb_device device module is unloaded\n");
}

int fourmb_device_open(struct inode *inode, struct file *filp)
{
	return 0; // always successful
}

int fourmb_device_release(struct inode *inode, struct file *filp)
{
	return 0; // always successful
}

long fourmb_device_ioctl(struct file *filp, unsigned int cmd, unsigned long arg){
    
	int err = 0;
	int retval = 0;
	char *tmp = NULL;
	//printk(KERN_WARNING "start ioctl1\n");

	if(_IOC_TYPE(cmd) != SCULL_IOC_MAGIC) return -ENOTTY;
	if(_IOC_NR(cmd) > SCULL_IOC_MAXNR) return -ENOTTY;

	//printk(KERN_WARNING "start ioctl2\n");

	if (_IOC_DIR(cmd) & _IOC_READ)
		err = !access_ok(VERIFY_WRITE, (void *)arg, _IOC_SIZE(cmd));
	else if (_IOC_DIR(cmd) & _IOC_WRITE)
		err = !access_ok(VERIFY_READ, (void *)arg, _IOC_SIZE(cmd));
	if (err) 
		return -EFAULT;

	//printk(KERN_WARNING "start ioctl3\n");

	switch(cmd){
		case SCULL_HELLO:
		    printk(KERN_WARNING "hello from ioctl\n");
		    break;
		case SET_DEV_MSG:
		    if(copy_from_user(dev_msg, (char *)arg, DEV_MSG_SIZE)){
		        return -EFAULT;
		    }
		    printk(KERN_ALERT "ioctl set dev_msg: %s", dev_msg);
		    break;
		case GET_DEV_MSG:
		    if(copy_to_user((char *)arg, dev_msg, DEV_MSG_SIZE)){
		        return -EFAULT;
		    }
		    printk(KERN_ALERT "ioctl get dev_msg: %s", dev_msg);
		    break;
		case WR_DEV_MSG:
		    tmp = kmalloc(DEV_MSG_SIZE, GFP_KERNEL);
		        if(copy_from_user(tmp, (char *)arg,  DEV_MSG_SIZE)){
		        return -EFAULT;
		    }

		    if(copy_to_user((char *)arg, dev_msg, DEV_MSG_SIZE)){
		        return -EFAULT;
		    }
		    strcpy(dev_msg, tmp);
		    printk(KERN_ALERT "dev_msg after _IOWR is: %s\n", dev_msg);
		    kfree(tmp);
		    break;
		default:
		    return -ENOTTY;
	}

	return retval;
}


loff_t fourmb_device_llseek(struct file* filp, loff_t off, int whence) {
	
	loff_t newpos;
	
	printk(KERN_INFO "%s: fourmb_device_llseek f_pos = %lld, off = %lld, whence = %d", DEVICE_NAME, filp->f_pos, off, whence);

	switch(whence) {
		case 0: // SEEK_SET
		newpos = off;
		break;

		case 1: // SEEK_CUR
		newpos = filp->f_pos + off;
		break;

		case 2: // SEEK_END
		newpos = cur_size + off; // from end
		printk(KERN_WARNING "%s: cur_size = %lld\n", DEVICE_NAME, cur_size);
		break;

		default: // invalid argument
		printk(KERN_WARNING "%s: fourmb_device_llseek has invalid argument\n", DEVICE_NAME);
		return -EINVAL;
	}

	if (newpos < 0 || newpos > cur_size )
		return -EINVAL;


	filp->f_pos = newpos;
	printk(KERN_INFO "%s: fourmb_device_llseek seeked to newpos %llu\n", DEVICE_NAME, newpos);
	return newpos;
}

ssize_t fourmb_device_read(struct file *filp, char *buf, size_t count, loff_t *f_pos)
{
	ssize_t result = 0;
	unsigned int read_size;

        if(*f_pos >= DEVICE_SIZE || *f_pos<0 || fourmb_device_data[*f_pos] == '\0')
		goto finish;

	read_size = strnlen(fourmb_device_data, DEVICE_SIZE);
	if((*f_pos + (long long int) count) > read_size)
		count = read_size - *f_pos; 
	

	if(copy_to_user(buf, &(fourmb_device_data[*f_pos]), count)) {
		printk(KERN_WARNING "%s: copy_to_user failed\n", DEVICE_NAME);
		result = -EFAULT;
		goto finish;
	}

	*f_pos += count;
	result = count;
	cur_size -= count;
	if( cur_size<0 )
		cur_size =0;

finish:
	printk(KERN_INFO "%s: fourmb_device_read complete\n", DEVICE_NAME);
	return result;
}

ssize_t fourmb_device_write(struct file *filp, const char *buf,size_t count, loff_t *f_pos)
{
	
	ssize_t result = 0;

	if (*f_pos >= DEVICE_SIZE || *f_pos < 0) {
		result = -EINVAL;
		goto finish;
	}

	if ((*f_pos + (long long int) count) > DEVICE_SIZE)
		count = DEVICE_SIZE - *f_pos;

	if(copy_from_user(&(fourmb_device_data[*f_pos]), buf, count)) {
		printk(KERN_WARNING "%s: copy_from_user failed\n", DEVICE_NAME);
		result = -EFAULT;
		goto finish;
	}

	*f_pos += count;
	result = count;
	cur_size += count;
	if( cur_size > DEVICE_SIZE)
		cur_size = DEVICE_SIZE;

	if(*f_pos < DEVICE_SIZE)
		fourmb_device_data[*f_pos] = '\0';

finish:
	printk(KERN_INFO "%s: fourmb_device_write complete\n", DEVICE_NAME);
	return result;
}


MODULE_LICENSE("GPL");
module_init(fourmb_device_init);
module_exit(fourmb_device_exit);

