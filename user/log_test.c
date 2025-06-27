#include "kernel/types.h"
#include "user/user.h"

#define NUM_CHILDREN 50
#define BUFFER_SIZE 4096 // One page
#define HEADER_ALIGNMENT 8

struct msg_header {
    uint16 index;
    uint16 length;
};

int min(int a, int b) {
    return (a < b) ? a : b;
}

uint64 align(uint64 addr) {
    return (addr + HEADER_ALIGNMENT - 1) & ~(HEADER_ALIGNMENT - 1);
}

int main(int argc, char** argv) {
    uint64 p_pid = getpid();
    int ret_val;

    char* buffer = (char*)malloc(BUFFER_SIZE);
    if (!buffer) {
        printf("Parent: failed to allocate buffer\n");
        return 1;
    }

    for (int i = 0; i < BUFFER_SIZE; i++) buffer[i] = 0;

    int child_index = 0;
    uint64 buffer_va = 0;

    for (child_index = 0; child_index < NUM_CHILDREN; child_index++) {
        if (fork() == 0) {
            // child
            free(buffer);
            struct msg_header hdr;
            hdr.index = child_index + 1;
            hdr.length = 100;

            char* msg_buf = malloc(hdr.length + 1);
            for (int i = 0; i < hdr.length; i++) {
                msg_buf[i] = 'a' + (i % 26);
            }
            msg_buf[hdr.length] = '\0';

            buffer_va = map_shared_pages(p_pid, (uint64)buffer, BUFFER_SIZE);
            if (buffer_va == 0) {
                printf("Child %d: failed to map shared page\n", child_index);
                free(msg_buf);
                return 1;
            }

            int requested_total = align(sizeof(struct msg_header) + hdr.length);
            uint64 offset = __sync_fetch_and_add((uint64*)buffer_va, requested_total);

            uint64 buffer_end = BUFFER_SIZE - sizeof(uint64);
            if (offset >= buffer_end) {
                printf("Child %d: buffer already full\n", child_index);
                free(msg_buf);
                return 0;
            }

            uint64 space_left = buffer_end - offset;
            uint16 max_msg_len = 0;

            if (space_left >= sizeof(struct msg_header) + 1) {
                max_msg_len = min(hdr.length, space_left - sizeof(struct msg_header));
            } else {
                printf("Child %d: not enough space even for header + 1 byte\n", child_index);
                free(msg_buf);
                return 0;
            }

            struct msg_header* entry = (struct msg_header*)(buffer_va + sizeof(uint64) + offset);

            memmove((void*)entry + sizeof(struct msg_header), msg_buf, max_msg_len);
            __sync_synchronize();

            entry->length = max_msg_len;
            entry->index = hdr.index;

            free(msg_buf);
            return 0;
        }
    }

    // parent
    struct msg_header* curr = (struct msg_header*)(buffer + sizeof(uint64));
    int messages_read = 0;

    while ((void*)curr < (void*)(buffer + BUFFER_SIZE) && messages_read < NUM_CHILDREN) {
        if (curr->index == 0) {
            sleep(1);
            continue;
        }
        if (!((void*)curr < (void*)(buffer + BUFFER_SIZE) && messages_read < NUM_CHILDREN)){
            printf("Parent: no more messages to read or all children have sent messages\n");
            break;
        }

        wait(&ret_val);
        void* data_ptr = (void*)curr + sizeof(struct msg_header);
        void* next_header = (void*)curr + align(sizeof(struct msg_header) + curr->length);
        if ((void*)next_header > (void*)(buffer + BUFFER_SIZE)) {
            printf("Parent: message from child %d overflows buffer, stopping\n", curr->index);
            break;
        }

        char* recv_msg = malloc(curr->length + 1);
        memmove(recv_msg, data_ptr, curr->length);
        recv_msg[curr->length] = '\0';

        printf("Child %d sent: %s\n", curr->index, recv_msg);
        free(recv_msg);

        curr = (struct msg_header*)align((uint64)next_header);
        messages_read++;
    }

    printf("All children have exited successfully.\n");
    return 0;
}
