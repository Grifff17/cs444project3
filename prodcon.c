#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include "eventbuf.c"

int num_producers;
int num_consumers;
int num_events;
int queue_size;
int done;

struct eventbuf *head;
sem_t *items;
sem_t *mutex;
sem_t *spaces;


sem_t *sem_open_temp(const char *name, int value) {
    sem_t *sem;

    // Create the semaphore
    if ((sem = sem_open(name, O_CREAT, 0600, value)) == SEM_FAILED)
        return SEM_FAILED;

    // Unlink it so it will go away after this process exits
    if (sem_unlink(name) == -1) {
        sem_close(sem);
        return SEM_FAILED;
    }

    return sem;
}

void *producer(void *arg) {
    int *id = arg;
    int num_prods = 0;
    while (num_prods < num_events) {
        int event_val = *id * 100 + num_prods;
        sem_wait(spaces);
        sem_wait(mutex);
        eventbuf_add(head, event_val);
        printf("P%i: adding event %i\n", *id, event_val);
        sem_post(mutex);
        sem_post(items);
        num_prods++;
    }
    printf("P%i: exiting\n", *id);
    return NULL;
}

void *consumer(void *arg) {
    int *id = arg;
    while (done == 0 || !eventbuf_empty(head)) {
        sem_wait(items);
        if (eventbuf_empty(head)) {
            break;
        }
        sem_wait(mutex);
        int event_val = eventbuf_get(head);
        printf("C%i: got event %i\n", *id, event_val);
        sem_post(mutex);
        sem_post(spaces);
    }
    printf("C%i: exiting\n", *id);
    sem_post(items);
    return NULL;
}


int main(int argc, char *argv[]) {
    (void)argc;

    num_producers = atoi(argv[1]);
    num_consumers = atoi(argv[2]);
    num_events = atoi(argv[3]);
    queue_size = atoi(argv[4]);

    head = eventbuf_create();
    done = 0;

    items = sem_open_temp("items", 0);
    mutex = sem_open_temp("mutex", 1);
    spaces = sem_open_temp("spaces", queue_size);

    pthread_t *cons_thread = calloc(num_consumers, sizeof *cons_thread);
    int *cons_thread_id = calloc(num_consumers, sizeof *cons_thread_id);

    pthread_t *prod_thread = calloc(num_producers, sizeof *prod_thread);
    int *prod_thread_id = calloc(num_producers, sizeof *prod_thread_id);

    srand(time(NULL) + getpid());

    for (int i = 0; i < num_consumers; i++) {
        cons_thread_id[i] = i;
        pthread_create(cons_thread + i, NULL, consumer, cons_thread_id + i);
    }
    for (int i = 0; i < num_producers; i++) {
        prod_thread_id[i] = i;
        pthread_create(prod_thread + i, NULL, producer, prod_thread_id + i);
    }

    for (int j = 0; j < num_producers; j++) {
        pthread_join(prod_thread[j], NULL);
    }
    done = 1;
    sem_post(items);
    for (int j = 0; j < num_consumers; j++) {
        pthread_join(cons_thread[j], NULL);
    }
    eventbuf_free(head);
}


