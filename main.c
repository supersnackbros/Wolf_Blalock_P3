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
#include <fcntl.h>
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

int numTourists;

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
        _exit(-1);
    }
    numTourists = atoi(argv[1]);
    tripsPerTourist = atoi(argv[2]);

    Sem_init(arrived, 0, 0);
    Sem_init(busLoaded, 0, 0);
    Sem_init(busUnloaded, 0, 0);
    Sem_init(seatbeltsFastened, 0, 0);
    Sem_init(songOver, 0, 0);
    Sem_init(availSeats, 0, 0);
    Sem_init(tourStarted, 0, 0);
    Sem_init(tourFinished, 0, 0);
    Sem_init(tweetMutex, 0, 1);
    Sem_init(countersMutex, 0, 1);

    pthread_t indy;
    pthread_t tourists[numTourists];
    
    Pthread_create(&indy, NULL, Indiana, NULL);

    for(long i = 0; i < numTourists; i++)
    {
        Pthread_create(&tourists[i], NULL, tourist, (void *) i);
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
    long j = (long) arg;
    long shopTime;
    srandom(time(NULL));

    // Tweet "Tourist <j>: Arrived
    W(tweetMutex);
    printf("Tourist <%ld>: Arrived\n", j);
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
        printf("Tourist <%ld>: Going to shop\n", j);
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
        printf("Tourist <%ld>: I got a seat on the bus\n", j);
        V(tweetMutex);

        // IF there are no more vacant seats OR NO other tourists are still shopping on the street
        W(countersMutex);
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
        printf("Tourist <%ld>: The Wheels on the Bus go Round and Round!\n", j);
        V(tweetMutex);

        // Inform Driver my song is over
        V(songOver);

        // Wait for the tour to finish
        W(tourFinished);

        // Tweet "I got off the bus"
        W(tweetMutex);
        printf("Tourist <%ld>: I got off the bus\n", j);
        V(tweetMutex);

        // If I am the last tourist to get off the bus
        W(countersMutex);
        onBoard--;
        if (onBoard == 0) {

            // Tweet "The bus is now vacant"
            W(tweetMutex);
            printf("Tourist <%ld>: The bus is now vacant!\n", j);

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
    printf("Tourist <%ld>: Leaving Town\n", j);
    V(tweetMutex);
}

void *Indiana(void* arg)
{
    srandom(time(NULL));
    // Tweet "Driver: Started My Day"
    W(tweetMutex);
    printf("Driver: Started My Day\n");
    V(tweetMutex);

    // Wait for all tourists to arrive to town
    for(int i = 0; i < numTourists; i++)
        W(arrived);

    // Repeat indefinitely
    while(true)
    {
        // If (all tourists left town)
        if(shopping <= 0)
            //break out of this loop
            break;

        // Declare all the seats on the bus now available
        for(int i = 0; i < BUS_CAP; i++)
            V(availSeats);

        // Take a nap until the bus is "as-full-as-possible"
        W(busLoaded);

        // Tweet "Indy: Welcome on Board Everyone <count> !"
        W(tweetMutex);
        printf("Indy: Welcome on Board Everyone %d!\n", onBoard);
        W(tweetMutex);

        // Wait for all tourists on board to fasten their seatbelts
        for(int i = 0; i < onBoard; i++)
            W(seatbeltsFastened);

        // Tweet "Indy: Thank you"
        W(tweetMutex);
        printf("Indy: Thanks you\n");
        V(tweetMutex);

        // duration = random: 1500 to 4000 mSec
        long duration = (random() % 2501) + 1500;

        // Tweet "Indy: Tour will last <duration> msec"
        W(tweetMutex);
        printf("Indy: Tour will last %ld msec\n", duration);
        V(tweetMutex);

        // Tweet "Indy: Bus is now moving. Sing Everyone!"
        W(tweetMutex);
        printf("Indy: Bus is now moving. Sing Everyone!\n");
        V(tweetMutex);

        // Tweet "Indy: Bus! Bus! On the street! Who is the fastest driver to beat?"
        W(tweetMutex);
        printf("Indy: Bus! Bus! On the street! Who is the fastest driver to beat?\n");
        V(tweetMutex);

        // Inform all tourists on board that bus has moved
        for(int i = 0; i < onBoard; i++)
            V(tourStarted);

        // Sleep (duration)
        usleep(duration);

        // Wait for all tourists on board to finish their songs
        for(int i = 0; i < onBoard; i++)
            W(songOver);

        // Tweet "Driver: Tour is over. Thanks you for Riding Indiana-Jones Coach"
        W(tweetMutex);
        printf("Driver: Tour is over. Thanks you for Riding Indiana-Jones Coach\n");
        V(tweetMutex);

        // Inform all tourists on board that tour has finished
        for(int i = 0; i < onBoard; i++)
            V(tourFinished);

        // Wait for last tourist of this group of tourists to get off the bus
        W(busUnloaded);
    }

    // Tweet "I did <count> tours today"
    W(tweetMutex);
    printf("I did %d tours today\n", (numTourists * tripsPerTourist));
    V(tweetMutex);
}
