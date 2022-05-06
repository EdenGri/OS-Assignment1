#include "kernel/param.h"
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"
#include "kernel/fcntl.h"
#include "kernel/syscall.h"
#include "kernel/memlayout.h"
#include "kernel/riscv.h"

void env(int size, int interval, char* env_name) {
    int result = 1;
    int loop_size = 10e6;
    int n_forks = 2;
    for (int i = 0; i < n_forks; i++) {
        fork();
    }
    for (int i = 0; i < loop_size; i++) {
        if (i % (int)(loop_size / 10e0) == 0) {

        }
        if (i % interval == 0) {
            result = result * size;
            
        }
    }
}

void env_large() {
    env(10e6, 10e6, "env_large");
}

void env_freq() {
    env(10e1, 10e1, "env_freq");
}

int main(int argc, char** argv){
    env_large();
    env_freq();
    exit(0);
}