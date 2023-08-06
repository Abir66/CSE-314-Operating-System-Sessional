#ifndef HELPER_H
#define HELPER_H

#include<iostream>
#include<pthread.h>
#include<semaphore.h>
#include<unistd.h>
#include<queue>
#include<vector>
#include<random>

using namespace std;

// generate random number using poisson distribution
// https://scicomp.stackexchange.com/questions/27330/how-to-generate-poisson-distributed-random-numbers-quickly-and-accurately
int get_random_number(double mean){
    random_device rd;
    mt19937 gen(rd());
    poisson_distribution<int> poisson(mean);

    while(true){
        int num = poisson(gen);
        if(num != 0) return num;
    }
}


class Semaphore{

    sem_t sem;
    int t;

public:

    Semaphore(){
        sem_init(&sem, 0, 1);
    }

    Semaphore(int init_value){
        sem_init(&sem, 0, init_value);
    }

    void down(){
        sem_wait(&sem);
    }

    void up(){
        sem_post(&sem);
    }

    void destroy(){
        sem_destroy(&sem);
    }

    int get_value(){
        sem_getvalue(&sem, &t);
        return t;
    }
};



Semaphore synchronized_print_mutex(1);

#define SYNCHRONIZED_PRINT(...) \
    synchronized_print_mutex.down(); \
    std::cout<<"Time - "<<get_time()<<"s: "; \
    std::cout << __VA_ARGS__ << std::endl; \
    synchronized_print_mutex.up();


time_t begin_time;

int get_time(){
    time_t now = time(0);
    return now - begin_time;
}

#endif


