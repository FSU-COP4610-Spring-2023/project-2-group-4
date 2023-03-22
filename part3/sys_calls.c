#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/syscalls.h>
#include <linux/linkage.h>
MODULE_LICENSE("GPL");

int (*STUB_initialize_bar)(void) = NULL;
int (*STUB_customer_arrival)(int,int) = NULL;
int (*STUB_close_bar)(void) = NULL;

EXPORT_SYMBOL(STUB_initialize_bar);
EXPORT_SYMBOL(STUB_customer_arrival);
EXPORT_SYMBOL(STUB_close_bar);

SYSCALL_DEFINE0(initialize_bar) {
        
        if (STUB_initialize_bar != NULL) 
                return STUB_initialize_bar();
        else 
                return -ENOSYS;
        
}

SYSCALL_DEFINE2(customer_arrival, int, number_of_customers, int, type) {
        if(STUB_customer_arrival != NULL)
                return STUB_customer_arrival(number_of_customers, type);
        else
                return -ENOSYS;
}

SYSCALL_DEFINE0(close_bar) {
        if (STUB_close_bar != NULL) 
                return STUB_close_bar();
        else 
                return -ENOSYS;

}
