#include "diag_syscall.h"

diag_ctrl_t diag_ctrl = {NULL, 0, 0, {NULL}};

diag_ctrl_entry_t* alloc_trace() {
    diag_ctrl.trace = kmalloc(sizeof(diag_ctrl_entry_t) * MAX_NUM_TRACE, GFP_KERNEL);
    return diag_ctrl.trace;
}

diag_ctrl_entry_t* get_trace() {
    return diag_ctrl.trace;
}

uint64_t get_num_trace() {
    return diag_ctrl.num_trace;
}

/**
 * set disks to monitor.
 * @return 0 if success, -1 if fail
*/
int set_disks(uint64_t num_disks, char** disks) {
    if(num_disks > MAX_DISKS) {
        printk(KERN_ERR "too many disks\n");
        return -1;
    }
    diag_ctrl.num_disk = num_disks;
    for (int i = 0; i < num_disks; i++) {
        diag_ctrl.monitored_disk[i] = kmalloc(sizeof(char) * 256, GFP_KERNEL);
        if (diag_ctrl.monitored_disk[i] == NULL) {
            printk(KERN_ERR "kmalloc failed\n");
            return -1;
        }
        strcpy(diag_ctrl.monitored_disk[i], disks[i]);
    }
    return 0;
}


