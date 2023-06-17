// start tracing

#include "sys.h"

int main() {
    
    diag_ctrl_t ctrl = {MAGIC3 , 0, {NULL}, 0, NULL, 0};
    call(SET_MAGIC, &ctrl);
    call(START_TRACE, NULL);
    return 0;
}