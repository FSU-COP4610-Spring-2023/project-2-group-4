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
#include <linux/kthread.h>
#include <linux/sched.h>
#include <linux/mutex.h>
#include <linux/delay.h>

MODULE_LICENSE("Dual BSD/GPL");


//DATA FOR PROC BULLSHIT
#define BUF_LEN 10000
static char msg[BUF_LEN];
static struct proc_dir_entry* proc_entry;
static int procfs_buf_len;



//DATA FOR TIMER
static struct timespec64 init_time;
static struct timespec64 cur_time;
static struct timespec64 elap_time;
static struct timespec64 arrival_time;
static bool flag = true;



//DATA FOR BAR
static char* current_state; //current state of bar
static int groupIdCounter = 1;
static char customer_chars[] = "FOJSPCD"; //this is for converting the customer types to chars for output. 0 = freshmen, 1 = sophomore, etc...
static int customer_times[] = {5,10,15,20,25};
enum bar_state{ OFFLINE, IDLE, LOADING, CLEANING, MOVING };
static char *state_table[] = {"OFFLINE", "IDLE", "LOADING", "CLEANING", "MOVING"};
static int CLEAN_TABLE = 10;
static int MOVING_TABLE = 2;
static int LOADING_TABLE = 1;
int currentOccupancy = 0;
int customersServiced = 0;
int fr = 0;
int so = 0;
int ju = 0;
int se = 0;
int pr = 0;
int curSeat = 0;

//DATA FOR THREADING/LOCKS
struct mutex barMutex;
struct task_struct *main_thread;
int current_table; //table the waiter is currently on
int read_p = 1; //??
int closedFlag = 0;

typedef struct group {
        int size;
        int type;
        struct timespec64 arrivalTime;
        int id;
        struct list_head list;
} Group;



typedef struct bar {
        enum bar_state state;
        struct list_head queue;
        Group tables[4][8];
        int clean_seats[4];
        int occupied_seats[4];
        int queue_size;
        int num_customers;
};

struct bar proof;


void updateTableConditions(void) {
        //if people are done drinking, we want to remove them and make that spot dirty
        if (mutex_lock_interruptible(&barMutex) == 0) {
                ktime_get_real_ts64(&cur_time);
                int table = 0, seat = 0;
                while(table < 4) {
                        while(seat < 8) { //check each seat if the customer has finished drinking
                                if (proof.tables[table][seat].type < 5 &&
                                                (long long) customer_times[proof.tables[table][seat].type] +
                                                        proof.tables[table][seat].arrivalTime.tv_sec <= cur_time.tv_sec) {
                                        currentOccupancy--;
                                        customersServiced++;
                                        if (proof.tables[table][seat].type == 0)
                                                fr--;
                                        if (proof.tables[table][seat].type == 1)
                                                so--;
                                        if (proof.tables[table][seat].type == 2)
                                                ju--;
                                        if (proof.tables[table][seat].type == 3)
                                                se--;
                                        if (proof.tables[table][seat].type == 4)
                                                pr--;
                                        proof.tables[table][seat].type = 6; //the seat turns dirty
                                }
                                seat++;
                        }
                        seat = 0;
                        table++;

                }
                mutex_unlock(&barMutex);
        }
}



static ssize_t procfile_read(struct file* file, char* ubuf, size_t count, loff_t *ppos) {
        int lock_return_val;

        //if (mutex_lock_interruptible(&barMutex) == 0) { //acquire lock to access table info


                if (flag) { //if it is the first call to proc
                        //get initial time
                        ktime_get_real_ts64(&init_time);
                        procfs_buf_len += sprintf(msg,"Current time = %lld.%.9ld\n", (long long) init_time.tv_sec,init_time.tv_nsec);
                        flag = false;
                }
                read_p = !read_p;
                if(read_p) {
                        //mutex_unlock(&barMutex);
                        return 0;
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

                        //mutex_unlock(&barMutex);

                        updateTableConditions();

                        //lock_return_val = mutex_lock_interruptible(&barMutex);

                        current_state = state_table[proof.state];
                        procfs_buf_len += sprintf(msg + procfs_buf_len,"Current waiter state: %s\n", current_state);
                        procfs_buf_len += sprintf(msg + procfs_buf_len,"Number of customers waiting: %d\n", proof.num_customers);
                        procfs_buf_len += sprintf(msg + procfs_buf_len,"Number of groups waiting: %d\n", proof.queue_size);
                        procfs_buf_len += sprintf(msg + procfs_buf_len,"Number of customers serviced: %d\n", customersServiced);
                        procfs_buf_len += sprintf(msg + procfs_buf_len,"Current occupancy: %d\n", currentOccupancy);


                        procfs_buf_len += sprintf(msg + procfs_buf_len,"Bar status: ");
                        if (fr != 0)
                                procfs_buf_len += sprintf(msg + procfs_buf_len,"%d F, ", fr);
                        if (so != 0)
                                procfs_buf_len += sprintf(msg + procfs_buf_len,"%d O, ", so);
                        if (ju != 0)
                                procfs_buf_len += sprintf(msg + procfs_buf_len,"%d J, ", ju);
                        if (se != 0)
                                procfs_buf_len += sprintf(msg + procfs_buf_len,"%d S, ", se);
                        if (pr != 0)
                                procfs_buf_len += sprintf(msg + procfs_buf_len,"%d P", pr);

                        struct list_head *temp;
                        struct group *tempg;
                        procfs_buf_len += sprintf(msg + procfs_buf_len, "\nContents of Queue\n");
                        //traverse queue
                        list_for_each(temp, &proof.queue) {
                                tempg = list_entry(temp, Group, list);
                                int i = tempg->size;
                                while( i > 0) {
                                        procfs_buf_len += sprintf(msg + procfs_buf_len, "%c ", customer_chars[tempg->type]);
                                        i--;
                                }
                                procfs_buf_len += sprintf(msg + procfs_buf_len, "(GroupID: %d)\n", tempg->id);
                        }

                        int table;
                        int seat;
                        while( table < 4) {
                                if(table == current_table) procfs_buf_len += sprintf(msg + procfs_buf_len, "[*] " );
                                else procfs_buf_len += sprintf(msg + procfs_buf_len, "[ ] ");

                                while( seat < 8) {
                                        procfs_buf_len += sprintf(msg + procfs_buf_len, "%c ", customer_chars[proof.tables[table][seat].type]);
                                        seat++;
                                }
                                seat = 0;
                                table++;
                                procfs_buf_len += sprintf(msg + procfs_buf_len, "\n");
                        }
                        //mutex_unlock(&barMutex);
                //}
        }
        if(*ppos > 0 || count < procfs_buf_len) return 0;
        if(copy_to_user(ubuf, msg, procfs_buf_len)) return -EFAULT;
        *ppos = procfs_buf_len;
        return procfs_buf_len;
}


static struct proc_ops procfile_fops = { .proc_read = procfile_read, };

int seatGroup(void) {

        ktime_get_real_ts64(&arrival_time);
        //attempt to acquire lock
        if (mutex_lock_interruptible(&barMutex) == 0) {
                if (proof.queue_size == 0) {
                        mutex_unlock(&barMutex);
                        return 1; //queue is empty, nobody to seat
                }


                //grab first group from queue
                struct group* tempGroup;
                tempGroup = list_first_entry(&proof.queue,Group,list);


                if (proof.clean_seats[current_table] >= tempGroup->size) { //if there's enough space at the table for the current group
                        //proof.state = LOADING;
                        int i = tempGroup->size;
                        while (i > 0) { //seat each individual customer in the group
                                proof.tables[current_table][curSeat].type = tempGroup->type;
                                proof.tables[current_table][curSeat].arrivalTime = arrival_time;
                                proof.clean_seats[current_table]--;
                                proof.occupied_seats[current_table]++;
                                i--;
                                curSeat++;
                                currentOccupancy++;
                                if (tempGroup->type == 0)
                                        fr++;
                                if (tempGroup->type == 1)
                                        so++;
                                if (tempGroup->type == 2)
                                        ju++;
                                if (tempGroup->type == 3)
                                        se++;
                                if (tempGroup->type == 4)
                                        pr++;
                        }


                        //remove 1 from queue size
                        proof.queue_size--;
                        proof.num_customers -= tempGroup->size;

                        //remove group from queue
                        list_del(&tempGroup->list);
                        //deallocate group since we're done using it
                        kfree(tempGroup);
                        mutex_unlock(&barMutex);
                        proof.state = LOADING;
                        ssleep(LOADING_TABLE);
                        proof.state = IDLE;
                        return 0;

                } else { //not enough space at table
                        mutex_unlock(&barMutex);
                        return 1;
                }
        } return -1; //unable to acquire lock
}

int cleanTable(void) {
        int i = 0;
        //attempt to acquire lock
        if (mutex_lock_interruptible(&barMutex) == 0) {
                //if there are no seats to clean return unsuccessful
                if (proof.clean_seats[current_table] == 8) {
                        mutex_unlock(&barMutex);
                        return 1;
                }

                //Check if people are still sitting
                while (i < 8) {
                        if (proof.tables[current_table][i].type < 5) {
                                mutex_unlock(&barMutex);
                                return 1; //if seat is not clean or dirty
                        }
                        i++;
                }

                //ACTUALLY clean the table
                i = 0;
                while (i < 8) {
                        proof.tables[current_table][i].type = 5; //set to clean
                        i++;
                }
                //proof.state = CLEANING; //set state to cleaning


                proof.clean_seats[current_table] = 8; //set number of clean seats to 8 for current table
                mutex_unlock(&barMutex); //release table lock
                proof.state = CLEANING;
                ssleep(CLEAN_TABLE); //sleep for 10 seconds
                proof.state = IDLE;
                return 0;
        } else {
                //this code is only reached if lock cannot be acquired
                printk(KERN_ALERT "failed to acquire lock for cleanTable function\n");
                return 1; //unsuccessful
        }
}

int waitress_run(void *data) {
        int table_cleaned;
        int seated;

        while (!kthread_should_stop()) {
                if (proof.queue_size == 0) { //if queue is empty
                        if (closedFlag == 1 && proof.clean_seats[0] + proof.clean_seats[1] + proof.clean_seats[2] + proof.clean_seats[3] == 32) {
                                proof.state = OFFLINE;
                                ssleep(1);
                                continue;
                        } else if (proof.clean_seats[0] + proof.clean_seats[1] + proof.clean_seats[2] + proof.clean_seats[3] == 32) { //if every seat is clean
                                proof.state = IDLE; //do nothing
                                ssleep(1);
                                continue;
                        } else {
                                table_cleaned = cleanTable();
                                ssleep(MOVING_TABLE);
                                current_table++;
                                current_table = current_table % 4;
                                continue;
                        }
                } else {
                        if (proof.clean_seats[current_table] < 8) table_cleaned = cleanTable();

                        seated = seatGroup();

                        if (seated != 0) { //if unsuccessful, move tables
                                proof.state = MOVING;
                                ssleep(MOVING_TABLE);
                                curSeat = 0;
                                current_table++;
                                current_table = current_table % 4;

                        }
                }

                printk(KERN_ALERT "finished one iteration of kthread_should_stop not false...\n");
        }
        return 0;
}

//SYSTEM CALLS

//open the BAR not the module
extern int (*STUB_initialize_bar)(void);
int initialize_bar(void) {
        printk(KERN_ALERT "initializing bar...\n");
        if (proc_entry == NULL) return -ENOMEM;
        main_thread = kthread_run(waitress_run, NULL, "She's at table %d bud", current_table);
        int i = 0;
        int j = 0;
        fr = 0;
        currentOccupancy = 0;
        customersServiced = 0;
        //acquire lock
        if (mutex_lock_interruptible(&barMutex) == 0) {
                while(i < 4) { //for every table, set them to clean and ready
                        proof.clean_seats[i] = 8; //set the clean seat count for each table
                        proof.occupied_seats[i] = 0; //set the occupied seat count for each table
                        while(j < 8) { //go through each seat
                                proof.tables[i][j].type = 5; //set all seats to clean
                                j++;
                        }
                        j = 0;
                        i++;
                }
                mutex_unlock(&barMutex);
                return 0;
        } else {
                printk(KERN_ALERT "failed to acquire lock for initialize_bar function\n");
                return 1;
        }
}

//customer arrival
extern int (*STUB_customer_arrival)(int,int);
int customer_arrival(int number_of_customers, int type) {
        //customers arrive in groups
        if ((type < 0 || type > 4) || (number_of_customers < 0 || number_of_customers > 8)) return 2;

        //acquire lock
        if (mutex_lock_interruptible(&barMutex) == 0) {
                if (closedFlag == 1) {
                        mutex_unlock(&barMutex);
                        return 1;
                } else {
                        //add groups to queue
                        //create temporary group based on function call arguments
                        struct group* tempGroup;
                        tempGroup = kmalloc(sizeof(Group) * 1, __GFP_RECLAIM);
                        tempGroup->size = number_of_customers;
                        tempGroup->type = type;
                        tempGroup->id = groupIdCounter; //use groupIdCounter to create unique group id
                        groupIdCounter++; //increment id counter to keep id's unique
                        ktime_get_real_ts64(&tempGroup->arrivalTime); //set arrival time to when they arrived

                        list_add_tail(&tempGroup->list, &proof.queue); //add group to list*/
                        proof.queue_size++;
                        proof.num_customers += number_of_customers;

                        mutex_unlock(&barMutex);
                        return 0;
                }
        } else {
                printk(KERN_ALERT "failed to acquire lock for customer_arrival function\n");
                return 1;
        }
}

//close the BAR not the module
extern int (*STUB_close_bar)(void);
int close_bar(void) {
        if (mutex_lock_interruptible(&barMutex) == 0) {
                if (closedFlag == 1) {
                        mutex_unlock(&barMutex);
                        return 1;
                }
                closedFlag = 1;
                mutex_unlock(&barMutex);
                return 0;
        } return 1;
}

// Initializes the MODULE
static int bar_init(void) {
        proc_entry = proc_create("majorsbar", 0666, NULL, &procfile_fops);
        if(proc_entry == NULL) return -ENOMEM;
        STUB_initialize_bar = initialize_bar;
        STUB_customer_arrival = customer_arrival;
        STUB_close_bar = close_bar;
        proof.state=IDLE;
        mutex_init(&barMutex);
        INIT_LIST_HEAD(&proof.queue);
        return 0;
}
// exits the MODULE
static void bar_exit(void) {
        kthread_stop(main_thread);
        proc_remove(proc_entry);
        STUB_initialize_bar = NULL;
        STUB_customer_arrival = NULL;
        STUB_close_bar = NULL;
        proof.state = OFFLINE;
        mutex_destroy(&barMutex);
        return;
}
module_init(bar_init);
module_exit(bar_exit);
