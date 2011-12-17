
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/workqueue.h>
#include <linux/sched.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <asm/uaccess.h>

/*
 *	Prototypes
 */

int init_module(void);
void cleanup_module(void);
static int device_open(struct inode *, struct file *);
static int device_release(struct inode *, struct file *);
static ssize_t device_read(struct file *, char *, size_t, loff_t *);
static ssize_t device_write(struct file *, const char *, size_t, loff_t *);

int procfile_read(char *buffer, char **buffer_location, off_t offset, int buffer_length, int *eof, void *data);
int procfile_write(struct file *file, const char *buffer, unsigned long count, void *data);


#define SUCCESS 0
#define DEVICE_NAME "gotchi"
#define BUF_LEN 80
#define PROCFS_MAX_SIZE 1024
#define PROCFS_NAME "gotchi"
#define INTERVAL 1000
#define WQ_NAME "TamagotchiWQ.c"

/*
 *	Global vars
 */

/* chrdev related */
static int Major;	/* Major number assigned to device */
static int Device_Open = 0;
static char msg[BUF_LEN];
static char *msg_Ptr;

static struct file_operations fops = {
	.read = device_read,
	.write = device_write,
	.open = device_open,
	.release = device_release
};

/* procfs related */
static struct proc_dir_entry *Proc_File;
static char procfs_buffer[PROCFS_MAX_SIZE];
static unsigned long procfs_buffer_size = 0;

/* timer related */
static void intrpt_routine(void *);
static int die = 0;

static struct workqueue_struct *workqueue;
static struct work_struct Task;
static DECLARE_WORK(Task, intrpt_routine);

/* tamagotchi related */
static int Hunger = 0;
static int Boredom = 0;

/*
 *	Module Init and Cleanup
 */

int init_module(void){
	Major = register_chrdev(0, DEVICE_NAME, &fops);

	Proc_File = create_proc_entry(PROCFS_NAME, 0644, NULL);
	if( NULL == Proc_File ){
		remove_proc_entry(PROCFS_NAME, NULL);
		printk(KERN_ALERT "Error: couldn't init /proc/%s\n", PROCFS_NAME);
		return -ENOMEM;
	}

	Proc_File->read_proc	= procfile_read;
	Proc_File->write_proc	= procfile_write;
	Proc_File->mode		= S_IFREG | S_IRUGO;
	Proc_File->uid		= 0;
	Proc_File->gid		= 0;
	Proc_File->size		= 37;

	workqueue = create_work_queue(WQ_NAME);
	queue_delayed_work(workqueue, &Task, INTERVAL);

	printk("<1>Tamagotchi loaded\n");
	printk("<1>To read Tamagotchi status create a dev file with\n");
	printk("<1>'mknod /dev/%s c %d 0'.\n", DEVICE_NAME, Major);
	printk("<1>Remove the device file and unload module to finish\n");
	return 0;
}

void cleanup_module(void){
	unregister_chrdev(Major, DEVICE_NAME);
	remove_proc_entry(PROCFS_NAME, NULL);
	die = 1;
	cancel_delayed_work(&Task);
	flush_workqueue(workqueue);
	destroy_workqueue(workqueue);
	printk("<1>Tamagotchi unloaded.\n");
	return;
}

/*
 *	/dev io
 */

static int device_open(struct inode *inode, struct file *file){
	static int counter = 0;
	if(Device_Open)
		return -EBUSY;
	Device_Open++;
	//TODO: insert my output handler there.
	sprintf(msg, "I already told you %d times Hello world!\n", counter++);
	msg_Ptr = msg;
	try_module_get(THIS_MODULE);
	return 0;
}

static int device_release(struct inode *inode, struct file *file){
	Device_Open--;
	module_put(THIS_MODULE);
	return 0;
}

static ssize_t device_read(struct file *filp, char *buffer, size_t length, loff_t *offset){
	int bytes_read = 0;
	if(*msg_Ptr == 0)
		return 0;
	while(length && *msg_Ptr){
		put_user( *(msg_Ptr++), buffer++);
		length--;
		bytes_read++;
	}
	return bytes_read; 
}

static ssize_t device_write(struct file *flip, const char *buff, size_t length, loff_t *offset){
	printk(KERN_ALERT "Please stop writing to /dev/%s, you should cat \"what you want\" > /proc/%s instead\n", DEVICE_NAME, PROCFS_NAME );
	return 0;
}

/*
 *	Proc file output
 */

int procfile_read(char *buffer, char **buffer_location, off_t offset, int buffer_length, int *eof, void *data){
	printk(KERN_ALERT "Please stop reading from /proc/%s, you should cat /dev/%s instead\n", PROCFS_NAME, DEVICE_NAME);
	return 0;
}

int procfile_write(struct file *file, const char *buffer, unsigned long count, void *data){
	procfs_buffer_size = count;
	if (procfs_buffer_size > PROCFS_MAX_SIZE)
		procfs_buffer_size = PROCFS_MAX_SIZE;
	if ( copy_from_user(procfs_buffer, buffer, procfs_buffer_size) ) {
		return -EFAULT;
	}
	//TODO: Insert my command handler there
	return procfs_buffer_size;
}

/*
 *	Timer
 */
static void intrpt_routine(void *irrelevant){
	Hunger++;
	Boredom++;
	if (die == 0)
		queue_delayed_work(workqueue, &Task, INTERVAL);
}

MODULE_LICENSE("GPL");
