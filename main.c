/*************************************************************************************************
 * MUST BE SURE TO USE STRONG SEMAPHORES WHILE WAITING FOR AVAILABLE SEATS. HAVE NOT ACCOUNTED FOR
 * THIS YET
 *************************************************************************************************/

#include <sys/types.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "wrappers.h"

/**************************************************************************************
 * These descriptions comply with my best understanding of what/how each is supposed to
 * do/function, but may not be 100% accurate.
 **************************************************************************************/
sem_t *availSeats; // Semaphore for number of available seats. Indiana sets this to capacity and
    // tourists decrement in a queue to board.

sem_t *seatbeltsFastened; // Semaphore for seatbelts fastened. Indiana sets this to zero and waits
    // one time for each tourist as they increment it.

sem_t *tourStarted; // Semaphore informing tourists that the tour has started. Indiana sets this to
    // capacity and tourists decrement to zero.

sem_t *songOver; // Semaphore informing Indiana that tourists have finished their song. Indiana
    // sets this to 0 and waits one time for each tourist as they increment it.

sem_t *tourFinished; // Semaphore informing tourists that the tour has finished. Indiana sets this
    // to capacity and tourists decrement to zero.

sem_t *arrived; // Semaphore informs Indiana of tourist arrivals. Tourists increment and Indiana
    // waits one time for each tourist.

sem_t *busLoaded; // Post when as full as possible. Indiana responds.

sem_t *busUnloaded; // Post when fully deboarded. Indiana responds.

sem_t *tweetMutex; // Mutex preventing multiple tourists from tweeting at once.

sem_t *countersMutex; // Mutex preventing multiple tourists from adjusting counters at once.

int tripsPerTourist; // Number  of times tourists do a shopping-touring routine.

int onBoard; // Number of people on board the bus at any time

int shopping; // Number of tourists in town and shopping (not on the bus)

/***************************************
 * Why is this a global variable anyway?
 ***************************************/
int tickets; // Total tickets for today = numTourists x trips-per-tourist

#define BUS_CAP 3

void *tourist(void *arg);
void *Indiana(void *arg);

void main(int argc, char *argv[]) {
    if(argc != 3)
    {
        printf("Format: ./main num_tourists trips_per_tourist\n");
        exit(-1);
    }
    int numTourists = atoi(argv[1]);
    tripsPerTourist = atoi(argv[2]);

    Sem_init(arrived, 0, 0);
    Sem_init(busLoaded, 0, 0);
    Sem_init(busUnloaded, 0, 0);
    Sem_init(seatbeltsFastened, 0, 0);
    Sem_init(songOver, 0, 0);
    Sem_init(availSeats, 0, 0);
    Sem_init(tweetMutex, 0, 1);
    Sem_init(countersMutex, 0, 1);

    pthread_t indy;
    pthread_t tourists[numTourists];
    
    Pthread_create(&indy, NULL, Indiana, NULL);

    for(int i = 0; i < numTourists; i++)
    {
        Pthread_create(&tourists[i], NULL, tourist, (void *) &i);
    }

    for(int i = 0; i < numTourists; i++)
    {
        Pthread_join(tourists[i], NULL);
    }
    Pthread_join(indy, NULL);

    Sem_destroy(arrived);
    Sem_destroy(busLoaded);
    Sem_destroy(busUnloaded);
    Sem_destroy(seatbeltsFastened);
    Sem_destroy(songOver);
    Sem_destroy(availSeats);
    Sem_destroy(tweetMutex);
    Sem_destroy(countersMutex);
}

void * tourist(void *arg) {
    int j = *(int *)arg;
    long shopTime;
    srandom(time(NULL));

    // Tweet "Tourist <j>: Arrived
    W(tweetMutex);
    printf("Tourist <%d>: Arrived\n", j);
    V(tweetMutex);

    // Notify Indiana
    V(arrived);

    // Increment "shopping"
    W(countersMutex);
    shopping++;
    V(countersMutex);

    // Repeat "tripsPerTourist" times
    for (int i = 0; i < tripsPerTourist; i++) {

        // Tweet "Tourist <j>: Going to shop" 
        W(tweetMutex);
        printf("Tourist <%d>: Going to shop\n", j);
        V(tweetMutex);

        // Simulate shopping session by sleeping
        shopTime = (random() % 2001) + 500;
        usleep(shopTime);

        // Wait for an available seat on the bus.
        W(availSeats);

        // Update counters
        W(countersMutex);
        onBoard++;
        shopping--;
        V(countersMutex);

        // Tweet "Tourist <j>: I got a seat on the bus"
        W(tweetMutex);
        printf("Tourist <%d>: I got a seat on the bus\n", j);
        V(tweetMutex);

        // IF there are no more vacant seats OR NO other tourists are still shopping on the street
        if (onBoard == BUS_CAP || shopping == 0) {

            // Alert Driver that bus is "as-full-as-possible
            V(busLoaded);
        }
        V(countersMutex);

        // Fasten seatbelt and inform driver of that
        V(seatbeltsFastened);

        // Wait for the bus to actually move
        W(tourStarted);

        // Tweet "Tourist <j>: wheels on the bus
        W(tweetMutex);
        printf("Tourist <%d>: The Wheels on the Bus go Round and Round!\n", j);
        V(tweetMutex);

        // Inform Driver my song is over
        V(songOver);

        // Wait for the tour to finish
        W(tourFinished);

        // Tweet "I got off the bus"
        W(tweetMutex);
        printf("Tourist <%d>: I got off the bus\n", j);
        V(tweetMutex);

        // If I am the last tourist to get off the bus
        W(countersMutex);
        onBoard--;
        if (onBoard == 0) {

            // Tweet "The bus is now vacant"
            W(tweetMutex);
            printf("Tourist <%d>: The bus is now vacant!\n", j);

            // Inform Driver that this group of tourists got off the bus
            V(busUnloaded);
        }
        shopping++;
        V(countersMutex);
    }
    // Dercrement shopping (no longer in town)
    W(countersMutex);
    shopping--;
    V(countersMutex);

    // Tweet "leaving town"
    W(tweetMutex);
    printf("Tourist <%d>: Leaving Town\n", j);
    V(tweetMutex);
}

void *Indiana(void* arg)
{
    
}
