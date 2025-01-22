#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <pthread.h>
#include <queue>
#include <vector>

struct Job {
    void (*jobFunction)(void*);
    void* jobData;
};

class ThreadPool {
public:
    ThreadPool(int numThreads);
    void addJob(Job job);
    ~ThreadPool();

private:
    static void* workerFunction(void* arg);
    std::queue<Job> jobQueue;
    std::vector<pthread_t> workers;
    pthread_mutex_t jobQueueMutex;
    pthread_cond_t jobQueueCond;
};

#endif // THREADPOOL_H