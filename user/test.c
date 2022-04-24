#include "kernel/types.h"
#include "user/user.h"
#include "kernel/fcntl.h"

void env(int size, int interval, char* env_name) {
    int result = 1;
    int loop_size = (int)10e6;
    int n_forks = 2;
    int pid;
    
    for (int i = 0; i < n_forks; i++) {
        pid = fork();
    }
    int cond = 1;
    for (int i = 0; i < loop_size; i++) {
        if (i % (int)(loop_size / 10e0) == 0) {
        	if (pid == 0) {
        		printf("sun process: %s %d/%d completed.\n", env_name, i, loop_size);
                printf("hello\n");
                printf("hello\n");
                sleep(1);
                for(int j=0; j<10; j++){
                    printf("sun process: %s %d/%d completed.\n", env_name, i, loop_size);
                    printf("hello\n");
                    printf("hello\n");
                }
                
                
        	} else {
                printf("father proces: %s %d/%d completed.\n", env_name, i, loop_size);
                printf("hello\n");
                printf("hello\n");
                
        	}
            
        }
        if(pid!=0 && i==1000000 && cond){
            sleep(1);

            cond=0;
        }
        
        if (i % interval == 0) {
            
            for(int j= 1; j<100; j++){
                
                result = result * size;
            }
            
            result = 1;
            for(int j= 1; j<100; j++){
                result = result * size;
            }
           
            result =1;
            for(int j= 1; j<100; j++){
                result = result * size;
            }
            
        }
    }
    printf("\n");
}

void env_large() {
    env(10e6, 3, "env_large");
}

void env_freq() {
    env(10e1, 10e1, "env_freq");
}

int fib(int num){
    if(num<=1){
        return num;
    }
    return fib(num-1)+fib(num-2);
}
int sjf_test(int num)
{
    for(int i = 0; i<200; i++){
        fib(num);
    }
    sleep(1);
    for(int i = 0; i<200; i++){
        fib(num);
    }
    return 1;
}


void long_loop(){
    for(int i = 0; i<10e6; i++){
        if(i%10000==0){
            printf("in long loop\n");
        }
        

        if(i==(10e5)){
            printf("long sleep -------------------------------------------------------------------------------\n");
            sleep(1);
        }
    }
    printf("----------------------------------------------------------------------------\n");
    printf("long loop done!\n");
    printf("----------------------------------------------------------------------------\n");
}
void short_loop(){
    for(int i = 0; i<10e6; i++){
        if(i%10000==0){
            printf("in short loop\n");
        }
        

        if(i==10){
            printf("****************************************\n");
            printf("pause system ------------------------------------------------------------------------------\n");
            printf("****************************************\n");
            //sleep(1);
            pause_system(15);
        }
    }
    printf("----------------------------------------------------------------------------\n");
    printf("short loop done!\n");
    printf("----------------------------------------------------------------------------\n");
}
int main(int argc, char** argv){
    env_large();
    print_stats();
    /*int num_fork = 3;
    int pid;
    for(int i=0; i<num_fork; i++){
        pid=fork();
    }
    if(pid==0){
        short_loop();
    }else{
        
        long_loop();
    }
    print_stats();*/
    exit(0);
}