#include <sys/syscall.h>
#include <unistd.h>
#include <stdio.h>

int call(int opt){
    syscall(428, opt, NULL);

}