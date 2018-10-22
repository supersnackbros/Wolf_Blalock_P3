/*************************************************************************************************
 * MUST BE SURE TO USE STRONG SEMAPHORES WHILE WAITING FOR AVAILABLE SEATS. HAVE NOT ACCOUNTED FOR
 * THIS YET
 *************************************************************************************************/

#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>

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

int tripsPerTourist; // Number of times tourists do a shopping-touring routine.

int totalSeats; // Number of seats on the bus

int onBoard; // Number of people on board the bus at any time

int shopping; // Number of tourists in town and shopping (not on the bus)

/***************************************
 * Why is this a global variable anyway?
 ***************************************/
int tickets; // Total tickets for today = numTourists x trips-per-tourist

void main(int argc, char *argv[]) {
    int numTourists;
    
}

void * tourist(void *arg) {
    int j = (int)arg;
    long shopTime;
    srandom(time(NULL));

    // Tweet "Tourist <j>: Arrived
    sem_wait(tweetMutex);
    printf("Tourist <%d>: Arrived\n", j);
    sem_post(tweetMutex);

    // Notify Indiana
    sem_post(arrived);

    // Increment "shopping"
    sem_wait(countersMutex);
    shopping++;
    sem_post(countersMutex);

    // Repeat "tripsPerTourist" times
    for (int i = 0; i < tripsPerTourist; i++) {

        // Tweet "Tourist <j>: Going to shop" 
        sem_wait(tweetMutex);
        printf("Tourist <%d>: Going to shop\n", j);
        sem_post(tweetMutex);

        // Simulate shopping session by sleeping
        shopTime = (random() % 2001) + 500;
        usleep(shopTime);

        // Wait for an available seat on the bus.
        sem_wait(availSeats);

        // Update counters
        sem_wait(countersMutex);
        onBoard++;
        shopping--;

        // Tweet "Tourist <j>: I got a seat on the bus"
        sem_wait(tweetMutex);
        printf("Tourist <%d>: I got a seat on the bus\n", j);
        sem_post(tweetMutex);

        // IF there are no more vacant seats OR NO other tourists are still shopping on the street
        if (onBoard == totalSeats || shopping == 0) {

            // Alert Driver that bus is "as-full-as-possible
            sem_post(busLoaded);
        }
        sem_post(countersMutex);

        // Fasten seatbelt and inform driver of that
        sem_post(seatbeltsFastened);

        // Wait for the bus to actually move
        sem_wait(tourStarted);

        // Tweet "Tourist <j>: wheels on the bus
        sem_wait(tweetMutex);
        printf("Tourist <%d>: The Wheels on the Bus go Round and Round!\n", j);
        sem_post(tweetMutex);

        // Inform Driver my song is over
        sem_post(songOver);

        // Wait for the tour to finish
        sem_wait(tourFinished);

        // Tweet "I got off the bus"
        sem_wait(tweetMutex);
        printf("Tourist <%d>: I got off the bus\n", j);
        sem_post(tweetMutex);

        // If I am the last tourist to get off the bus
        sem_wait(countersMutex);
        onBoard--;
        if (onBoard == 0) {

            // Tweet "The bus is now vacant"
            sem_wait(tweetMutex);
            printf("Tourist <%d>: The bus is now vacant!\n", j);

            // Inform Driver that this group of tourists got off the bus
            sem_post(busUnloaded);
        }
        shopping++;
        sem_post(countersMutex);
    }
    // Dercrement shopping (no longer in town)
    sem_wait(countersMutex);
    shopping--;
    sem_post(countersMutex);

    // Tweet "leaving town"
    sem_wait(tweetMutex);
    printf("Tourist <%d>: Leaving Town\n", j);
}
