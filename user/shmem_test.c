#include "kernel/types.h"
#include "user/user.h"

int main(int argc, char** argv){
    int p_pid = getpid();
    char random_string[12];
    char* str;
    int unmap_flag = argc > 1;

    if(fork() == 0){
        //child
        printf("Chhild size, pre map: %d\n", sbrk(0));
        str = map_shared_pages(p_pid, random_string, 12);
        if(str == 0){
            printf("Error mapping shared pages\n");
            return 1;
        }
        printf("Chhild size, after map: %d\n", sbrk(0));
        memcpy(str, "Hello daddy", 12);
        if(unmap_flag){
            unmap_shared_pages(str, 12);
        }
        printf("Chhild size, after unmap: %d\n", sbrk(0));
        str = malloc(0x14000);
        if(str == 0){
            printf("Error allocating memory\n");
            return 1;
        }
        printf("Chhild size, after malloc: %d\n", sbrk(0));
        free(str);
    }
    else {
        
   }
}