/**
 * File: thread-pool.h
 * -------------------
 * This class defines the ThreadPool class, which accepts a collection
 * of thunks (which are zero-argument functions that don't return a value)
 * and schedules them in a FIFO manner to be executed by a constant number
 * of child threads that exist solely to invoke previously scheduled thunks.
 */

#ifndef _thread_pool_
#define _thread_pool_

#include <cstddef>     // for size_t
#include <functional>  // for the function template used in the schedule signature
#include <thread>      // for thread
#include <vector>      // for vector
#include <queue>       // for queue
#include <mutex>       // for mutex
#include <condition_variable> // for condition_variable
#include <atomic>      // for atomic
#include "Semaphore.h" // for Semaphore

using namespace std;


/**
 * @brief Represents a worker in the thread pool.
 * 
 * The `worker_t` struct contains information about a worker 
 * thread in the thread pool. Should be includes the thread object, 
 * availability status, the task to be executed, and a semaphore 
 * (or condition variable) to signal when work is ready for the 
 * worker to process.
 */
typedef struct worker {
    thread ts;
    function<void(void)> thunk;
    /**
     * @brief Semaphore specific to this worker. The dispatcher signals it
     * when a new task is assigned to this worker.
     */
    Semaphore taskReadySema;
} worker_t;

class ThreadPool {
  public:

  /**
  * Constructs a ThreadPool configured to spawn up to the specified
  * number of threads.
  */
    ThreadPool(size_t numThreads);

  /**
  * Schedules the provided thunk (which is something that can
  * be invoked as a zero-argument function without a return value)
  * to be executed by one of the ThreadPool's threads as soon as
  * all previously scheduled thunks have been handled.
  */
    void schedule(const function<void(void)>& thunk);

  /**
  * Blocks and waits until all previously scheduled thunks
  * have been executed in full.
  */
    void wait();

  /**
  * Waits for all previously scheduled thunks to execute, and then
  * properly brings down the ThreadPool and any resources tapped
  * over the course of its lifetime.
  */
    ~ThreadPool();
    
  private:
    // Métodos privados para la lógica interna de los hilos.
    void worker(int id);
    void dispatcher();

    thread dt;                              // dispatcher thread handle
    vector<worker_t> wts;                   // worker thread handles. you may want to change/remove this
    
    // Bandera para indicar a los hilos que deben terminar.
    atomic<bool> done;

    // Cola de tareas (thunks) y su mutex para acceso seguro.
    queue<function<void(void)>> taskQueue;
    mutex taskQueueMutex;

    // Semáforo para notificar al dispatcher que hay tareas disponibles.
    Semaphore tasksAvailable;

    // Semáforo para que el dispatcher sepa cuántos workers están libres.
    Semaphore workersAvailable;

    // Cola para los IDs de los workers disponibles y su mutex.
    queue<int> availableWorkerIds;
    mutex workerManagementMutex;

    // Variables para la implementación de wait().
    atomic<size_t> tasksInProgress;
    mutex waitMutex;
    condition_variable waitCondition;

    // Previene la copia y asignación del ThreadPool. 
    ThreadPool(const ThreadPool& original) = delete;
    ThreadPool& operator=(const ThreadPool& rhs) = delete;
};
#endif
