#include "ThreadPool.h"
#include <iostream>
#include <pthread.h>

ThreadPool::ThreadPool(int numThreads) {
    pthread_mutex_init(&jobQueueMutex, nullptr);
    pthread_cond_init(&jobQueueCond, nullptr);

    for (int i = 0; i < numThreads; i++) {
        pthread_t worker;
        pthread_create(&worker, NULL, &ThreadPool::workerFunction, this);
        workers.push_back(worker);
    }
}

void ThreadPool::addJob(Job job) {
    pthread_mutex_lock(&jobQueueMutex);
    jobQueue.push(job);
    pthread_cond_signal(&jobQueueCond);
    pthread_mutex_unlock(&jobQueueMutex);
}

void* ThreadPool::workerFunction(void* arg) {
    ThreadPool* pool = (ThreadPool*)arg;
    
    while (true) {
        pthread_mutex_lock(&pool->jobQueueMutex);

        while (pool->jobQueue.empty()) {
            pthread_cond_wait(&pool->jobQueueCond, &pool->jobQueueMutex);
        }

        Job job = pool->jobQueue.front();
        pool->jobQueue.pop();
        pthread_mutex_unlock(&pool->jobQueueMutex);

        job.jobFunction(job.jobData);
    }

    return NULL;
}

ThreadPool::~ThreadPool() {
    pthread_mutex_destroy(&jobQueueMutex);
    pthread_cond_destroy(&jobQueueCond);
}