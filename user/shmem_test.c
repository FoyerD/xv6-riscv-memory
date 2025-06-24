#include "kernel/types.h"
#include "user/user.h"

int main(int argc, char** argv){
    uint64 p_pid = getpid();
    uint64 c_pid;
    int ret_val;
    char daddy_string[12] = { 0 };

    uint64 parent_va = (uint64)daddy_string;
    uint64 child_va;
    int unmap_flag = 1;

    if((c_pid = fork()) == 0){
        //child
        printf("Child size, pre map: %d\n", sbrk(0));
        child_va = map_shared_pages(p_pid, parent_va, 12);
        if(child_va == 0){
            printf("Error mapping shared pages\n");
            return 1;
        }
        printf("Child size, after map: %d\n", sbrk(0));
        memmove((void*)child_va, "Hello daddy", 12);
        if(unmap_flag){
            unmap_shared_pages(child_va, 12);
        }
        printf("Child size, after unmap: %d\n", sbrk(0));
        child_va = (uint64)malloc(0x14000);
        if(child_va == 0){
            printf("Error allocating memory\n");
            return 1;
        }
        printf("Child size, after malloc: %d\n", sbrk(0));
        free((void*)child_va);
        return 0;
    }
    else {
        //parent
        wait(&ret_val);
        if(ret_val != 0){
            printf("Child exited with error code %d\n", ret_val);
            return 1;
        }
        printf("You got mail: %s\n", daddy_string);
        return 0;
    }
}