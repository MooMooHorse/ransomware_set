#include "sys.h"

int call(int opt, diag_ctrl_t* ctrl){
    return syscall(428, opt, ctrl);
}