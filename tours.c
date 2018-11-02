#include <sys/types.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include "wrappers.h"

sem_t availSeats; // Semaphore for number of available seats. Indiana sets this to capacity and
    // tourists decrement in a queue to board.
sem_t seatbeltsFastened; // Semaphore for seatbelts fastened. Indiana sets this to zero and waits
    // one time for each tourist as they increment it.
sem_t tourStarted; // Semaphore informing tourists that the tour has started. Indiana sets this to
    // capacity and tourists decrement to zero.
sem_t songOver; // Semaphore informing Indiana that tourists have finished their song. Indiana
    // sets this to 0 and waits one time for each tourist as they increment it.
sem_t tourFinished; // Semaphore informing tourists that the tour has finished. Indiana sets this
    // to capacity and tourists decrement to zero.
sem_t arrived; // Semaphore informs Indiana of tourist arrivals. Tourists increment and Indiana
    // waits one time for each tourist.
sem_t busLoaded; // Post when as full as possible. Indiana responds.
sem_t busUnloaded; // Post when fully deboarded. Indiana responds.
sem_t tweetMutex; // Mutex preventing multiple tourists from tweeting at once.
sem_t countersMutex; // Mutex preventing multiple tourists from adjusting counters at once.
int tickets;
int onBoard; // Number of people on board the bus at any time
int shopping; // Number of tourists in town and shopping (not on the bus)
int tripsPerTourist; // Number  of times tourists do a shopping-touring routine.
int numTourists;

#define BUS_CAP 3

void *tourist(void *arg);
void *Indiana(void *arg);

void main(int argc, char *argv[]) {
    if(argc != 3)
    {
        printf("Format: ./main num_tourists trips_per_tourist\n");
        _exit(-1);
    }
    numTourists = atoi(argv[1]);
    tripsPerTourist = atoi(argv[2]);

    printf("OPERATOR We have %d tourists for today. Each will make %d tours\n", numTourists,
        tripsPerTourist);

    tickets = numTourists * tripsPerTourist;
    onBoard = 0;
    shopping = 0;

    Sem_init(&arrived, 0, 0);
    Sem_init(&busLoaded, 0, 0);
    Sem_init(&busUnloaded, 0, 0);
    Sem_init(&seatbeltsFastened, 0, 0);
    Sem_init(&songOver, 0, 0);
    Sem_init(&availSeats, 0, 0);
    Sem_init(&tourStarted, 0, 0);
    Sem_init(&tourFinished, 0, 0);
    Sem_init(&tweetMutex, 0, 1);
    Sem_init(&countersMutex, 0, 1);

    pthread_t indy;
    pthread_t tourists[numTourists];
    
    Pthread_create(&indy, NULL, Indiana, NULL);

    for(long i = 0; i < numTourists; i++)
    {
        Pthread_create(&tourists[i], NULL, tourist, (void *) (i+1));
    }

    for(int i = 0; i < numTourists; i++)
    {
        Pthread_join(tourists[i], NULL);
    }
    Pthread_join(indy, NULL);

    Sem_destroy(&arrived);
    Sem_destroy(&busLoaded);
    Sem_destroy(&busUnloaded);
    Sem_destroy(&seatbeltsFastened);
    Sem_destroy(&songOver);
    Sem_destroy(&availSeats);
    Sem_destroy(&tweetMutex);
    Sem_destroy(&countersMutex);
    Sem_destroy(&tourStarted);
    Sem_destroy(&tourFinished);

    printf("\n#### Operator Terminated ####\n");
}

void * tourist(void *arg) {
    long j = (long) arg;
    long shopTime;
    srandom(time(NULL));

    // Increment 'shopping'
    W(&countersMutex);
    shopping++;
    V(&countersMutex);

    // Tweet "Tourist <j>: Arrived
    W(&tweetMutex);
    printf("Tourist %ld: Hey! I just arrived in Harrisonburg.\n", j);
    V(&tweetMutex);

    // Notify Indiana
    V(&arrived);

    // Repeat 'tripsPerTourist' times
    for (int i = 0; i < tripsPerTourist; i++) {

        // Simulate shopping session by sleeping
        shopTime = (random() % 2001) + 500;
        W(&tweetMutex);
        printf("Tourist %ld: Tour #%d. Going to shop for %ld milliseconds\n", j, i + 1, shopTime);
        V(&tweetMutex);
        usleep(shopTime);

        // Wait for an available seat on the bus.
        W(&tweetMutex);
        printf("Tourist %ld: Back from shopping, waiting for a seat on the bus\n", j);
        V(&tweetMutex);
        W(&availSeats);

        // Update counters (Mutex unlocked later)
        W(&countersMutex);
        onBoard++;
        shopping--;

        // Tweet "Tourist <j>: I got a seat on the bus"
        W(&tweetMutex);
        printf("Tourist %ld: I got a seat on the bus.. CLICK\n", j);
        V(&tweetMutex);

        // IF there are no more vacant seats OR NO other tourists are still shopping on the street
        if (onBoard == BUS_CAP || shopping == 0) {
            // Alert Driver that bus is "as-full-as-possible
            V(&busLoaded);
        }
        V(&countersMutex);

        // Fasten seatbelt and inform driver of that
        V(&seatbeltsFastened);

        // Wait for the bus to actually move
        W(&tourStarted);

        // Tweet "Tourist <j>: wheels on the bus
        W(&tweetMutex);
        printf("Tourist %ld: The Wheels on the Bus go Round and Round!\n", j);
        V(&tweetMutex);

        // Inform Driver my song is over
        V(&songOver);

        // Wait for the tour to finish
        W(&tourFinished);

        // Tweet "I got off the bus"
        W(&tweetMutex);
        printf("Tourist %ld: Got off the bus\n", j);
        V(&tweetMutex);

        // If I am the last tourist to get off the bus
        W(&countersMutex);
        onBoard--;
        if (onBoard == 0) {

            // Tweet "The bus is now vacant"
            W(&tweetMutex);
            printf("Tourist %ld: Last to get off. Alerting driver bus is now empty\n", j);
            V(&tweetMutex);

            // Inform Driver that this group of tourists got off the bus
            V(&busUnloaded);
        }
        shopping++;
        V(&countersMutex);
    }

    W(&countersMutex);
    shopping--;
    V(&countersMutex);

    // Tweet "leaving town"
    W(&tweetMutex);
    printf("Tourist %ld: Leaving Town\n", j);
    V(&tweetMutex);
}

void *Indiana(void* arg)
{
    int seatsToFree;
    int toursCompleted = 0;

    srandom(time(NULL));
    // Tweet "Driver: Started My Day"
    W(&tweetMutex);
    printf("\nIndy: Hey! I just started my day waiting for tourists to arrive in town\n");
    V(&tweetMutex);

    // Wait for all tourists to arrive to town 
    for(int i = 0; i < numTourists; i++) {
        W(&arrived);
    }

    // Repeat indefinitely
    while(tickets > 0)
    {
        // Declare seats on the bus as available
        W(&tweetMutex);
        printf("\nIndy: New Tour. Declaring 3 vacant seats\n");
        V(&tweetMutex);
        if (tickets >= BUS_CAP)
            seatsToFree = BUS_CAP;
        else
            seatsToFree = tickets;

        for(int i = 0; i < seatsToFree; i++) {
            V(&availSeats);
            tickets--;
        }

        // Take a nap until the bus is "as-full-as-possible"
        W(&tweetMutex);
        printf("Indy: Taking a nap until tourists get on board\n");
        V(&tweetMutex);
        W(&busLoaded);

        // Tweet "Indy: Welcome on Board Everyone <count> !"
        W(&tweetMutex);
        printf("Indy: Welcome on board dear %d passenger(s)! Please fasten your seatbelts\n", onBoard);
        V(&tweetMutex);

        // Wait for all tourists on board to fasten their seatbelts
        for(int i = 0; i < onBoard; i++)
            W(&seatbeltsFastened);

        // Tweet "Indy: Thank you"
        W(&tweetMutex);
        printf("Indy: Thank you all for fastening your seatbelts\n");
        V(&tweetMutex);

        // duration = random: 1500 to 4000 mSec
        long duration = (random() % 2501) + 1500;

        // Tweet "Indy: Tour will last <duration> msec"
        W(&tweetMutex);
        printf("Indy: Tour will last %ld milliseconds\n", duration);
        V(&tweetMutex);

        // Tweet "Indy: Bus is now moving. Sing Everyone!"
        W(&tweetMutex);
        printf("Indy: Bus will now move. We all must sing!\n");
        V(&tweetMutex);

        // Tweet "Indy: Bus! Bus! On the street! Who is the fastest driver to beat?"
        W(&tweetMutex);
        printf("Indy: Bus! Bus! On the street! Who is the fastest driver to beat?\n");
        V(&tweetMutex);

        // Inform all tourists on board that bus has moved
        for(int i = 0; i < onBoard; i++)
            V(&tourStarted);

        // Sleep (duration)
        usleep(duration);

        // Wait for all tourists on board to finish their songs
        for(int i = 0; i < onBoard; i++)
            W(&songOver);

        // Tweet "Driver: Tour is over. Thanks you for Riding Indiana-Jones Coach"
        W(&tweetMutex);
        printf("Indy: Tour is over. Thanks for your business\n");
        V(&tweetMutex);

        // Inform all tourists on board that tour has finished
        for(int i = 0; i < onBoard; i++)
            V(&tourFinished);

        // Wait for last tourist of this group of tourists to get off the bus
        W(&busUnloaded);

        // Increment 'toursCompleted'
        toursCompleted++;
    }

    // Tweet "I did <count> tours today"
    W(&tweetMutex);
    printf("\nIndy: Business is now closed. I did %d tours today\n", toursCompleted);
    V(&tweetMutex);
}
