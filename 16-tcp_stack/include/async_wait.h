#ifndef __ASYNC_WAIT_H__
#define __ASYNC_WAIT_H__

#include <pthread.h>
#include <stdlib.h>

struct async_wait {
	pthread_mutex_t lock;
	pthread_cond_t cond;
	int notified;
	int dead;
	int sleep;
};

static inline void wait_init(struct async_wait *wait)
{
	pthread_cond_init(&wait->cond, NULL);
	pthread_mutex_init(&wait->lock, NULL);
	wait->dead = 0;
	wait->notified = 0;
	wait->sleep = 0;
}

static inline void wait_exit(struct async_wait *wait)
{
	pthread_mutex_lock(&wait->lock);
	if (wait->dead)
		goto unlock;
	wait->dead = 1;
	if (wait->sleep)
		pthread_cond_broadcast(&wait->cond);

unlock:
	pthread_mutex_unlock(&wait->lock);
}

static inline int sleep_on(struct async_wait *wait)
{
	pthread_mutex_lock(&wait->lock);
	if (wait->dead)
		goto unlock;
	wait->sleep = 1;
	if (!wait->notified)
		pthread_cond_wait(&wait->cond, &wait->lock);
	wait->notified = 0;
	wait->sleep = 0;
unlock:
	pthread_mutex_unlock(&wait->lock);

	return -(wait->dead);
}

static inline int wake_up(struct async_wait *wait)
{
	pthread_mutex_lock(&wait->lock);
	if (wait->dead)
		goto unlock;

	if (!wait->notified) {
		wait->notified = 1;
		if (wait->sleep)
			pthread_cond_signal(&wait->cond);
	}

unlock:
	pthread_mutex_unlock(&wait->lock);
	return -(wait->dead);
}

static inline struct async_wait *alloc_wait_struct()
{
	struct async_wait *wait = malloc(sizeof(struct async_wait));
	wait_init(wait);

	return wait;
}

static inline void free_wait_struct(struct async_wait *wait)
{
	free(wait);
}

#endif
