#include "sys.h"

int main() {
    diag_ctrl_t ctrl = {MAGIC3 , 0, {NULL}, 0, NULL, 0};
    ctrl.blk2file_size = 400000;
    ctrl.blk2file = (uint64_t*)malloc(sizeof(uint64_t) * 400000);
    ctrl.blk2file_size = call(GET_MAP, &ctrl);
    for(int i = 0; i < ctrl.blk2file_size; i++) {
        printf("%lu ", ctrl.blk2file[i]);
    }
    return 0;
}