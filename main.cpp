#include <iostream>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <cstdlib>
#include <ctime>

#define NUM_STUDENTS 10
#define MAX_WAITING 3

pthread_mutex_t mutex;
sem_t *sem_seats;
bool ta_sleeping = true;
int waiting_count = 0;

void *student(void *arg) {
    int id = *(int *)arg;
    while (true){
        sleep(rand() % 5 + 1); // Random delay to simulate students arriving at different times. But must be greater than 0.
        pthread_mutex_lock(&mutex);
        if (ta_sleeping) {
            // Student wakes up the TA
            std::cout << "Student " << id << " wakes up the TA." << std::endl;
            ta_sleeping = false;
            waiting_count = 1;
            pthread_mutex_unlock(&mutex);
        } else {
            if (waiting_count < MAX_WAITING) {
                // Student enters the waiting room
                waiting_count++;
                std::cout << "Student " << id << " waits for the TA. Current position:" << waiting_count << " out of " << MAX_WAITING << std::endl;
                pthread_mutex_unlock(&mutex);

                sem_wait(sem_seats); // Occupy a waiting seat
            } else {
                std::cout << "Student " << id << " sees no available seats, will come back later." << std::endl;
                pthread_mutex_unlock(&mutex);
                pthread_exit(nullptr);
            }
        }
        pthread_mutex_lock(&mutex);
        std::cout << "Student " << id << " gets help from the TA and leaves the waiting area."<< std::endl;
        waiting_count--;
        pthread_mutex_unlock(&mutex);

        // Getting help for random duration
        int help_time = rand() % 3 + 1;
        std::cout << "TA currently helping Student " << id << " for " << help_time << " seconds." << std::endl;
        sleep(help_time); // Simulate time taken for getting help

        sem_post(sem_seats); // Release the seat

        std::cout << "Student " << id << "leave TA office." << std::endl;
    }
}

void *teaching_assistant(void *arg) {
    while (true) {
        pthread_mutex_lock(&mutex);
        if (waiting_count > 0) {
            sem_post(sem_seats); // Release a waiting seat
            pthread_mutex_unlock(&mutex);
            std::cout << "TA is help student " << std::endl;
            sleep(rand() % 3); // Simulate the time taken to help a student
        } else {
            pthread_mutex_unlock(&mutex);
            std::cout << "TA goes to sleep." << std::endl;
            ta_sleeping = true;
        }
        pthread_exit(nullptr);
    }
}

int main() {
    srand(time(nullptr));

    // initialize mutex and semaphores
    sem_seats = sem_open("/seats_semaphore", O_CREAT | O_EXCL, 0644, MAX_WAITING); // Create semaphore for seats
    pthread_t ta_thread;
    pthread_t student_threads[NUM_STUDENTS];
    pthread_create(&ta_thread, nullptr, teaching_assistant, nullptr); // Create TA thread

    int student_ids[NUM_STUDENTS];
    for (int i = 0; i < NUM_STUDENTS; ++i) {
        student_ids[i] = i + 1;
        pthread_create(&student_threads[i], nullptr, student, (void *)&student_ids[i]); // Create student threads
    }


    for (auto & student_thread : student_threads) {
        pthread_join(student_thread, nullptr); // Join student threads
    }

    pthread_join(ta_thread, nullptr); // Join TA thread
    sem_close(sem_seats); // Close semaphore for seats
    sem_unlink("/seats_semaphore"); // Unlink semaphore for seats
    pthread_mutex_destroy(&mutex); // Destroy mutex

    return 0;
}