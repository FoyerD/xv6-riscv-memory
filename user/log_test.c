#include "kernel/types.h"
#include "user/user.h"

#define NUM_CHILDREN 100
#define BUFFER_SIZE 4096 //PGSIZE

struct msg_header {
    uint16 index;
    uint16 length;
};

int min(int a, int b) {
    return (a < b) ? a : b;
}

int main(int argc, char** argv){
    // process managment
    uint64 p_pid = getpid();
    uint64 c_pids[NUM_CHILDREN];
    int ret_val;
    
    // buffer related
    char* buffer = (char*)malloc(BUFFER_SIZE);
    uint64 child_va = 0;
    uint64 parent_va = 0;
    
    // child process related
    struct msg_header child_header;
    int child_index = 0;
    int is_parent = 1;
    
    // general _helper
    struct msg_header* curr_header;
    char* child_msg;


    for(child_index = 0; child_index < NUM_CHILDREN; child_index++){
        if((c_pids[child_index] = fork()) == 0){
            // child initialization
            free(buffer); // Free the buffer in the child process

            child_header.index = child_index+1;
            child_header.length = child_header.index;
            child_msg = (char*)malloc(child_header.length + 1);
            for(int i = 0; i < child_header.length; i++){
                child_msg[i] = 'a' + (i) % 26; // fill with some data
            }
            is_parent = 0;
            parent_va = (uint64)buffer;
            child_va = map_shared_pages(p_pid, parent_va, BUFFER_SIZE);
            if(child_va == 0){
                printf("Child %d failed to map shared pages.\n", child_index);
                return 1;
            }
            break;
        }
    }
    
    if(is_parent){
        parent_va = (uint64)buffer;

        curr_header = (struct msg_header*)buffer;
        while((void*)curr_header < (void*)(buffer + BUFFER_SIZE)){
            while(curr_header->length == 0){
                sleep(1);
            }
            child_msg = (char*)malloc(curr_header->length + 1);
            memmove(child_msg, (char*)curr_header + sizeof(struct msg_header), curr_header->length);
            child_msg[curr_header->length] = '\0';
            printf("Child %d sent: %s\n", curr_header->index, child_msg);
            free(child_msg);
            curr_header = (void*)curr_header + sizeof(struct msg_header) + curr_header->length;
            curr_header = (struct msg_header*)(((uint64)curr_header + 7) & ~7);
        }
        if((void*)curr_header >= ((void*)buffer + BUFFER_SIZE)){
            printf("Reached end of buffer! in padre\n");
            return 1;
        }
        for(int i = 0; i < NUM_CHILDREN; i++){
            wait(&ret_val);
            if(ret_val != 0){
                printf("Child %d exited with error code %d\n", i, ret_val);
                return 1;
            }
        }
        printf("All children have exited successfully.\n");
        return 0;
    }

    else{
        // child logic
        curr_header = (struct msg_header*)(((uint64)child_va + 7) & ~7);
        int total_size = sizeof(struct msg_header) + child_header.length;
        
        while((void*)curr_header + sizeof(struct msg_header) < (void*)(child_va) + BUFFER_SIZE &&
            __sync_val_compare_and_swap((uint64*)curr_header, 0, *(uint64*)&child_header) != 0){
            curr_header = (void*)curr_header + total_size;
            curr_header = (struct msg_header*)(((uint64)curr_header + 7) & ~7);
            if(curr_header >= (struct msg_header*)(child_va + BUFFER_SIZE)){
                printf("Buffer overflow detected! in hijo %d\n", child_index);
                return 1;
            }
        }
        
        curr_header->index = child_header.index;
        curr_header->length = child_header.length;
        memmove((void*)curr_header + sizeof(struct msg_header),
                child_msg,
                min(child_header.length, BUFFER_SIZE - (curr_header + sizeof(struct msg_header) - (struct msg_header*)child_va)));
        free(child_msg);
        return 0;
    }
}