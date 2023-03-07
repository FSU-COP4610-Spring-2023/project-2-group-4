#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/list.h>
#include <linux/string.h>
#include <linux/time.h>
#include <linux/ktime.h>
#include <linux/slab.h>

MODULE_LICENSE("Dual BSD/GPL");

#define BUF_LEN 10000

static struct proc_dir_entry* proc_entry;

static struct timespec64 init_time;
static struct timespec64 cur_time;
static struct timespec64 elap_time;
static bool flag = true;
static char msg[BUF_LEN];
static int procfs_buf_len;
static char* current_state;
static int groupIdCounter = 1;
static char customer_chars[] = "FOJSPCD"; //this is for converting the customer types to chars for output. 0 = freshmen, 1 = sophomore, etc...
enum bar_state{ OFFLINE, IDLE, LOADING, CLEANING, MOVING };
static char *state_table[] = {"OFFLINE", "IDLE", "LOADING", "CLEANING", "MOVING"};


typedef struct bar {
        enum bar_state state;
        struct list_head queue;
        struct list_head tables[4];
	int queue_size;
};

struct bar proof;

typedef struct group {
        int size;
        int type;
        struct timespec64 arrivalTime;
        int id;
	struct list_head list;
} Group;

char* group_to_string(Group* group);
char* queue_to_string(void);


static ssize_t procfile_read(struct file* file, char* ubuf, size_t count, loff_t *ppos) {
	 if (flag) { //if it is the first call to proc
                //get initial time
                ktime_get_real_ts64(&init_time);
                procfs_buf_len += sprintf(msg,"Current time = %lld.%.9ld\n", (long long) init_time.tv_sec,init_time.tv_nsec);
                flag = false;
        } else {
                procfs_buf_len = strlen(msg);
                //get current time
                ktime_get_real_ts64(&cur_time);

                //subtract init time from current time to get elapsed time (seconds)
                time64_t seconds_elapsed = cur_time.tv_sec - init_time.tv_sec;
                //subtract init time from current time to get elapsed time (nanoseconds)
                long nanoseconds_elapsed = cur_time.tv_nsec - init_time.tv_nsec;

                //set elapsed_time variable (seconds)
                elap_time.tv_sec = seconds_elapsed;

                //set elapsed_time variable (nanoseconds)
                elap_time.tv_nsec = nanoseconds_elapsed;

                //to account for overflow
                if(elap_time.tv_nsec < 0) {
                        elap_time.tv_sec -= 1;
                        elap_time.tv_nsec += 1000000000;
                }
                procfs_buf_len += sprintf(msg,"Current time = %lld.%.9ld\n", (long long) cur_time.tv_sec,cur_time.tv_nsec);
                procfs_buf_len += sprintf(msg + procfs_buf_len,"Time elapsed = %lld.%.9ld\n", (long long) elap_time.tv_sec,elap_time.tv_nsec); 
		current_state = state_table[proof.state];
		procfs_buf_len += sprintf(msg + procfs_buf_len,"Current waiter state: %s\n", current_state);
		procfs_buf_len += sprintf(msg + procfs_buf_len,"%s\n",queue_to_string());        	
	}
        if(*ppos > 0 || count < procfs_buf_len)
                return 0;
        if(copy_to_user(ubuf, msg, procfs_buf_len))
                return -EFAULT;
        *ppos = procfs_buf_len;
        
        return procfs_buf_len;

}


static struct proc_ops procfile_fops = { .proc_read = procfile_read, };


//SYSTEM CALLS

//open the BAR not the module
extern int (*STUB_initialize_bar)(void);
int initialize_bar(void) {
	if (proc_entry == NULL) return -ENOMEM;
	
	//initialize timer to zero
	
	return 0;
}


//customer arrival
extern int (*STUB_customer_arrival)(int,int);
int customer_arrival(int number_of_customers, int type) {
	//customers arrive in groups
	if ((type < 0 || type > 4) || (number_of_customers < 0 || number_of_customers > 8)) 
		return 1;
	
	
	//add groups to queue
	

	//create temporary group based on function call arguments
	struct group* tempGroup = kmalloc(sizeof(Group), __GFP_RECLAIM);
	tempGroup->size = number_of_customers;
	tempGroup->type = type;
	tempGroup->id = groupIdCounter; //use groupIdCounter to create unique group id
	groupIdCounter++; //increment id counter to keep id's unique
	ktime_get_real_ts64(&tempGroup->arrivalTime); //set arrival time to when they arrived
	tempGroup->list = proof.queue;	
	//am I allocating memory correctly here?

	list_add_tail(&tempGroup->list, &proof.queue); //add group to list*/
	proof.queue_size++;
	return 0;
}

//close the BAR not the module
extern int (*STUB_close_bar)(void);
int close_bar(void) { 
	if (proof.state == OFFLINE)
		return 1;
	proof.state = OFFLINE;
	return 0;
}
// Initializes the MODULE
static int bar_init(void) {
	proc_entry = proc_create("majorsbar", 0666, NULL, &procfile_fops);
	if(proc_entry == NULL)
		return -ENOMEM;
        STUB_initialize_bar = initialize_bar;
	STUB_customer_arrival = customer_arrival;
	STUB_close_bar = close_bar;
	proof.state=IDLE;
	INIT_LIST_HEAD(&proof.queue);
	return 0;
}
// exits the MODULE
static void bar_exit(void) {
	proc_remove(proc_entry);
	STUB_initialize_bar = NULL;
	STUB_customer_arrival = NULL;
	STUB_close_bar = NULL;
	proof.state = OFFLINE;
	return;
}

module_init(bar_init);

module_exit(bar_exit);

char* group_to_string(Group* group) {
	char result[100];
	
	struct list_head* tempCustomer;

	list_for_each(tempCustomer, &group->list) {
		char tempString[] = {customer_chars[group->type], '\0'};
		strcat(result, tempString);
		strcat(result, " ");
	}
	strcpy(result, "[ID: ");
	char tempString[] = {(char)group->id,'\0'};
	strcat(result, tempString);
	strcat(result,"]");
	return result;
}

char* queue_to_string(void) {
	char* result = kmalloc(100 * proof.queue_size, __GFP_RECLAIM);
	strcpy(result,"CURRENT QUEUE:\n");
	struct list_head* tempGroup;
	Group* curGroup;
	
	//traverse list of groups
	list_for_each(tempGroup, &proof.queue) {
		curGroup = list_entry(tempGroup, Group, list);
		strcat(result, group_to_string(curGroup));
		strcat(result,"\n");
	}
	return result;
}

int cleanTable(struct bar* b, int table) {
//        printf("cleaning table...\n");
        //if entire table is dirty, clean and return true
        //else return false
        return 0;
}
