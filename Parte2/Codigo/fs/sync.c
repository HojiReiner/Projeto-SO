#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include "sync.h"

/*
Initializes a new lock_sync structures using the sync
strategie defined by the string. The sync strategies
available are:
1."mutex"
2."rwlock"
3."nosync"
Strings with a different name will make the program exit
with failure
*/
Sync_Lock Lock_Init(char* strat){
    Sync_Lock new = (Sync_Lock) malloc(sizeof(struct lock_sync));

    if(strcmp(strat,"mutex") == 0){
        new->sync_strat = MSYNC;
        new->lock = malloc(sizeof(pthread_mutex_t));
        if(pthread_mutex_init((pthread_mutex_t*) new->lock, NULL) != 0){
            fprintf(stderr, "Error: problem initializing mutex\n");
            exit(EXIT_FAILURE);
        }
    }
    else if(strcmp(strat,"rwlock") == 0){
        new->sync_strat = RWSYNC;
        new->lock = malloc(sizeof(pthread_rwlock_t));
        if(pthread_rwlock_init( (pthread_rwlock_t*) new->lock, NULL) != 0){
            fprintf(stderr, "Error: problem initializing rwlock\n");
            exit(EXIT_FAILURE);
        }
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

/*Receives a Sync_Lock and destroys it*/
void Destroy_Lock(Sync_Lock x_lock){
    if(x_lock->sync_strat == MSYNC){
        if(pthread_mutex_destroy((pthread_mutex_t*)x_lock->lock) != 0){
            fprintf(stderr, "Error: problem destoying mutex\n");
            exit(EXIT_FAILURE);
        }
    }
    else if(x_lock->sync_strat == RWSYNC){
        if(pthread_rwlock_destroy((pthread_rwlock_t*)x_lock->lock) != 0){
            fprintf(stderr, "Error: problem destoying rwlock\n");
            exit(EXIT_FAILURE);
        }
    }
    free(x_lock->lock);
    free(x_lock);
}


/*Lock code using the defined sync strategy*/
void Lock(Sync_Lock x_lock, int rw){
    if(x_lock->sync_strat == MSYNC){
        if(pthread_mutex_lock((pthread_mutex_t*)x_lock->lock) != 0){
            fprintf(stderr, "Error: problem locking mutex\n");
            exit(EXIT_FAILURE);
        }
    }
    else if(x_lock->sync_strat == RWSYNC){
        if(rw == LREAD){
            if(pthread_rwlock_rdlock((pthread_rwlock_t*)x_lock->lock) != 0){
                fprintf(stderr, "Error: problem locking rdlock\n");
                exit(EXIT_FAILURE);
            }
        }
        else if(rw == LWRITE){
            if(pthread_rwlock_wrlock((pthread_rwlock_t*)x_lock->lock) != 0){
                fprintf(stderr, "Error: problem locking wrlock\n");
                exit(EXIT_FAILURE);
            }
        }
    }
}


/*Unlock code using the defined sync strategy*/
void Unlock(Sync_Lock x_lock){
    if(x_lock->sync_strat == MSYNC){
        if(pthread_mutex_unlock((pthread_mutex_t*)x_lock->lock) != 0){
            fprintf(stderr, "Error: problem unlocking mutex\n");
            exit(EXIT_FAILURE);
        }
    }
    else if(x_lock->sync_strat == RWSYNC){
        if(pthread_rwlock_unlock((pthread_rwlock_t*)x_lock->lock) != 0){
            fprintf(stderr, "Error: problem unlocking rwlock\n");
            exit(EXIT_FAILURE);
        }
    }
}