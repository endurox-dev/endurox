#include <stdio.h>
#include "mqueue.h"

#define PERCENT(n, d) (((float)(n) / ((d) ? (d) : 1)) * 100)

#define MSG_COUNT   500             

#define MQ_PATH     "/tester.mq"
#define MQ_MAXMSG   100
static mqd_t mqd = (mqd_t)-1;

void *producer(void *arg)
{
    int delay = (unsigned long)arg;
    int msg;

    printf("Producer: Delay = 0 to %d ms\n", delay);
    for (msg = 1; msg <= MSG_COUNT; msg++) {
        if (mq_send(mqd, (char *)&msg, sizeof(int), 1)) {
            perror("Unable to send message");
            return(NULL);
        }
        printf("Producer: Put message #%d in queue\n", msg);
        Sleep((delay) ? (rand() % delay) : 0);
    }
    return(NULL);
}

void *consumer(void *arg)
{
    int delay = (unsigned long)arg;
    int msg;
    int n;
    struct mq_attr mqattr;

    printf("Consumer: Delay = 0 to %d ms\n", delay);
    for (n = 0; n < MSG_COUNT; n++) {
        if (mq_receive(mqd, (char *)&msg, sizeof(int), NULL) < 0) {
            perror("Unable to recv message");
            return(NULL);
        }
        if (mq_getattr(mqd, &mqattr)) {
            perror("Unable to get message queue attributes");
            return(NULL);
        }
        printf("Consumer: Got message #%d from queue [Queue is %.1f%% full]\n", 
               msg, PERCENT(mqattr.mq_curmsgs, mqattr.mq_maxmsg));
        Sleep((delay) ? (rand() % delay) : 0);
    }
    
    return(NULL);
}

int main(int argc, char *argv[])
{
    pthread_t ptid, ctid;
    struct mq_attr mqattr = {0, MQ_MAXMSG, sizeof(int), 0};
    long pdelay = 250; /* Milliseconds */
    long cdelay = 500; /* Milliseconds */
    void *tstat;

    switch (argc) {
    case 3: cdelay = atol(argv[2]);
    case 2: pdelay = atol(argv[1]);
    default:
        break;
    }
    srand((unsigned)time(NULL));

    mq_unlink(MQ_PATH);
    if ((mqd = mq_open(MQ_PATH, O_CREAT|O_EXCL|O_RDWR, 0660, &mqattr)) ==
        (mqd_t)-1) {
        perror("Unable to open message queue");
        return(1);
    }
    if (pthread_create(&ctid, NULL, consumer, (void *)cdelay)) {
        mq_close(mqd);
        perror("Unable to create consumer thread");
        return(1);
    }

    if (pthread_create(&ptid, NULL, producer, (void *)pdelay)) {
        mq_close(mqd);
        perror("Unable to create producer thread");
        return(1);
    }

    pthread_join(ptid, &tstat);
    pthread_join(ctid, &tstat);
    mq_close(mqd);

    return(0);
}
