#ifndef SYNC_H
#define SYNC_H

/*sync_strat constants*/
#define MSYNC 0
#define RWSYNC 1
#define NOSYNC 2

/*rw constants*/
#define NA 0
#define LREAD 1
#define LWRITE 2

typedef struct lock_sync{
	int sync_strat;
	void* lock;
} *Sync_Lock;


Sync_Lock Lock_Init(char* strat);
void Destroy_Lock(Sync_Lock x_lock);
void Lock(Sync_Lock x_lock, int rw);
void Unlock(Sync_Lock x_lock);

#endif