// Online C compiler to run C program online
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#define iterate_access_valid(i, rb) \
     rb->arr_valid[((i + ((rb)->tail)) % ((rb)->capacity))]
     
#define iterate_access_data(i, rb) \
     rb->arr[((i + ((rb)->tail)) % ((rb)->capacity))]

typedef struct ring_buffer{
    int capacity;
    int head;
    int tail;
    int* arr;
    short* arr_valid;
} ring_buffer;

ring_buffer* init(int size) {
    ring_buffer* rb = (ring_buffer*)malloc(sizeof(ring_buffer));
    rb->capacity = size;
    rb->arr = (int*)calloc(size, sizeof(int));
    rb->arr_valid = (short*)calloc(size, sizeof(short));
    rb->head = 0;
    rb->tail = 0;
    return rb;
}

// by default shoule be: * 2
ring_buffer* resize(ring_buffer* rb_old, int new_size) {
    int old_size = rb_old->capacity;
    ring_buffer* rb_new = init(new_size);
    
    int i;
    for (i = 0; i < old_size; i ++) {
        if (iterate_access_valid(i, rb_old)) {
            rb_new->arr_valid[i] = 1;
            rb_new->arr[i] = iterate_access_data(i, rb_old);
        } else {
            break;
        }
    }
    
    if (i == 0) {
        // do nothing, empty
        rb_new->tail = 0;
        rb_new->head = 0;
    } else if (i == old_size) {
        // old is full
        rb_new->tail = 0;
        rb_new->head = old_size;
    } else {
        rb_new->tail = 0;
        rb_new->head = i;
    }
    
    free(rb_old->arr);
    free(rb_old->arr_valid);
    free(rb_old);
    
    return rb_new;
}

short push(ring_buffer* rb, int* data) {
    if (rb->arr_valid[rb->head]) {
        return 0;
    } else {
        rb->arr[rb->head] = *data;
        rb->arr_valid[rb->head] = 1;
        rb->head += 1;
        rb->head = rb->head % (rb->capacity);
        return 1;
    }
}

short pop(ring_buffer* rb, int* data) {
    if (rb->arr_valid[rb->tail]) {
        *data = rb->arr[rb->tail];
        rb->arr_valid[rb->tail] = 0;
        rb->tail += 1;
        rb->tail = rb->tail % (rb->capacity);
        return 1;
    } else {
        return 0;
    }
}

int main() {
    ring_buffer* rb = init(5);
    for (int i = 0; i < 10; i ++) {
         push(rb, &i);
    }
    for (int i = 0; i < 3; i ++) {
        pop(rb, &i);
    }
    for (int i = 5; i < 8; i ++) {
        push(rb, &i);
    }
    rb = resize(rb, 10);
    for (int i = 8; i < 20; i ++) {
        push(rb, &i);
    }
    for (int i = 0; i < 10; i ++) {
        if (iterate_access_valid(i, rb)) {
            printf("%d", iterate_access_data(i, rb));
        }
    }

    return 0;
}