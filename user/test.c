#include "kernel/types.h"
#include "user/user.h"
#include "kernel/fcntl.h"

void example_pause_system(int interval, int pause_seconds, int loop_size) {
    int n_forks = 0;
    
    
    for (int i = 0; i < n_forks; i++) {
    	fork();
    }
    
    int n= loop_size;
    for (int i = 0; i < n; i++) {
    
        if (i % interval == 0)
            printf("pause system %d/%d completed.\n", i, n);
            
     
        int x = n/2;
        if (i == x){
            pause_system(pause_seconds);
        }
        
       
    }
    printf("\n");
    
}

void example_kill_system(int interval, int loop_size) {
    int n_forks = 2;
    
    
    for (int i = 0; i < n_forks; i++) {
    	fork();
    }
    
    int n= loop_size;
    for (int i = 0; i < n; i++) {
        if (i % interval == 0)
            printf("kill system %d/%d completed.\n", i, n);
        
            
        
        int x = n/2;
        if (i == x){
            kill_system();
        }
        
       
    }
    printf("\n");
}

int main(int argc, char** argv){
    //example_pause_system(100,5,1000);
    //example_kill_system(100, 1000);
    for(int i=1; i<1000; i++){
        printf("%d",i);
    }
    exit(0);
}