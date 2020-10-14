#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include "sync.h"

Sync_Lock Lock_Init(char* strat){
    Sync_Lock new = (Sync_Lock) malloc(sizeof(struct lock_sync));

    if(strcmp(strat,"mutex") == 0){
        new->sync_strat = MSYNC;
        new->lock = malloc(sizeof(pthread_mutex_t));
        pthread_mutex_init((pthread_mutex_t*) new->lock, NULL);
    }
    else if(strcmp(strat,"rwlock") == 0){
        new->sync_strat = RWSYNC;
        new->lock = malloc(sizeof(pthread_rwlock_t));
        pthread_rwlock_init( (pthread_rwlock_t*) new->lock, NULL);
    }
    else if(strcmp(strat,"nosync") == 0){
        new->sync_strat = NOSYNC;
        new->lock = NULL;
    }
    else{
        fprintf(stderr, "Error: %s is not an available sync strategy\n",strat);
        exit(EXIT_FAILURE);
    }
    return new;
}


void Destroy_Lock(Sync_Lock x_lock){
    free(x_lock->lock);
    free(x_lock);
}


/*Lock code using the defined sync strategy*/
void Lock(Sync_Lock x_lock, int rw){
    if(x_lock->sync_strat == MSYNC){
        pthread_mutex_lock((pthread_mutex_t*)x_lock->lock);
    }
    else if(x_lock->sync_strat == RWSYNC){
        if(rw == LREAD){
            pthread_rwlock_rdlock((pthread_rwlock_t*)x_lock->lock);
        }
        else if(rw == LWRITE){
            pthread_rwlock_wrlock((pthread_rwlock_t*)x_lock->lock);
        }
    }
}


/*Unlock code using the defined sync strategy*/
void Unlock(Sync_Lock x_lock){
    if(x_lock->sync_strat == MSYNC){
        pthread_mutex_unlock((pthread_mutex_t*)x_lock->lock);
    }
    else if(x_lock->sync_strat == RWSYNC){
        pthread_rwlock_unlock((pthread_rwlock_t*)x_lock->lock);
    }
}