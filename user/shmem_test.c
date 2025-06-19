#include "kernel/types.h"
#include "user/user.h"

int main(int argc, char** argv){
    int p_pid = getpid();
    char* random_string = "Hello from shared memory!";

    if(fork() == 0){
        
    }
    else {
    
    }
}