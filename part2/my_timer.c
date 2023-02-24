#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/ktime.h>

MODULE_LICENSE("GPL");
#define BUF_LEN 100
static int proc_buf_len;
static bool flag = true;
static struct timespec64 init_time;
static struct timespec64 cur_time;
static struct timespec64 elapsed_time; 

static char msg[BUF_LEN];

static struct proc_dir_entry* proc_entry;

static ssize_t myread(struct file* file, char* ubuf, size_t count, loff_t *ppos);

static ssize_t mywrite(struct file* file, const char* ubuf, size_t count, loff_t* ppos);

static struct proc_ops procfile_fops =
{
	//.owner = THIS_MODULE,
	.proc_read = myread,
	.proc_write = mywrite,
};

static ssize_t myread(struct file* file, char* ubuf, size_t count, loff_t *ppos)
{
	//printk(KERN_INFO "proc_read 66\n");

	if (flag) { //if it is the first call to proc
		//get initial time
		ktime_get_real_ts64(&init_time);
		proc_buf_len += sprintf(msg,"Current time = %lld.%.9ld\n", (long long) init_time.tv_sec,init_time.tv_nsec);
		flag = false;
	} else {
	        proc_buf_len = strlen(msg);
        	
		//get current time
        	ktime_get_real_ts64(&cur_time);

		//subtract init time from current time to get elapsed time (seconds)
        	time64_t seconds_elapsed = cur_time.tv_sec - init_time.tv_sec;
		//subtract init time from current time to get elapsed time (nanoseconds)
		long nanoseconds_elapsed = cur_time.tv_nsec - init_time.tv_nsec;

		//set elapsed_time variable (seconds)
	        elapsed_time.tv_sec = seconds_elapsed;

		//set elapsed_time variable (nanoseconds)
	        elapsed_time.tv_nsec = nanoseconds_elapsed;

		//to account for overflow
	        if(elapsed_time.tv_nsec < 0) {
                	elapsed_time.tv_sec -= 1;
                	elapsed_time.tv_nsec += 1000000000;
        	}

		proc_buf_len += sprintf(msg,"Current time = %lld.%.9ld\n", (long long) cur_time.tv_sec,cur_time.tv_nsec);
	        proc_buf_len += sprintf(msg + proc_buf_len,"Time elapsed = %lld.%.9ld\n", (long long) elapsed_time.tv_sec,elapsed_time.tv_nsec); 
	}

	if(*ppos > 0 || count < proc_buf_len)
		return 0;
	if(copy_to_user(ubuf, msg, proc_buf_len))
		return -EFAULT;
	*ppos = proc_buf_len;
	
	return proc_buf_len;
}

static ssize_t mywrite(struct file* file, const char* ubuf, size_t count, loff_t* ppos)
{
	printk(KERN_INFO "proc_write\n");
	if(count > BUF_LEN)
		proc_buf_len = BUF_LEN;
	else
		proc_buf_len = count;
	copy_from_user(msg, ubuf, proc_buf_len);
	printk(KERN_INFO "got from user %s\n", msg);
	return proc_buf_len;
}
static int my_timer_init(void)
{
	proc_entry = proc_create("timer", 0666, NULL, &procfile_fops);
	if(proc_entry == NULL)
		return -ENOMEM;
	return 0;
}

static void my_timer_exit(void)
{	
	proc_remove(proc_entry);
}

module_init(my_timer_init);
module_exit(my_timer_exit);
