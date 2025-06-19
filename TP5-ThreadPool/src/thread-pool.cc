/**
 * File: thread-pool.cc
 * --------------------
 * Presents the implementation of the ThreadPool class.
 */

#include "thread-pool.h"
#include <iostream>

using namespace std;

ThreadPool::ThreadPool(size_t numThreads) :
    dt(),                        // 1ro: dispatcher thread handle
    wts(numThreads),             // 2do: worker thread handles
    done(false),                 // 3ro: bandera done
    taskQueue(),                 // 4to: cola de tareas
    taskQueueMutex(),            // 5to: mutex de la cola de tareas
    tasksAvailable(),            // 6to: semáforo de tareas disponibles
    workersAvailable(numThreads),// 7mo: semáforo de workers disponibles
    availableWorkerIds(),        // 8vo: cola de IDs de workers disponibles
    workerManagementMutex(),     // 9no: mutex de management de workers
    tasksInProgress(0),          // 10mo: contador de tareas en progreso
    waitMutex(),                 // 11vo: mutex para wait
    waitCondition()              // 12vo: condición para wait
{
    // Inicia un número específico de hilos trabajadores. 
    for (size_t i = 0; i < numThreads; ++i) {
        availableWorkerIds.push(i); // Todos los IDs de workers están disponibles al inicio.
        wts[i].ts = thread([this, i] { worker(i); });
    }

    // Inicia el único hilo dispatcher. 
    dt = thread([this] { dispatcher(); });
}


void ThreadPool::schedule(const function<void(void)>& thunk) {
    // Se comprueba si la función es nula.
    if (!thunk) {
        throw invalid_argument("La función a programar no puede ser nula.");
    }
    // Si el pool está destruido, no se pueden agregar más tareas.
    if (done) {
        throw runtime_error("ThreadPool está siendo destruido, no se programan más tareas.");
    }
    // Bloquea la cola para agregar la nueva tarea de forma segura. 
    unique_lock<mutex> lock(taskQueueMutex);
    taskQueue.push(thunk);
    tasksInProgress++;
    
    // Notifica al hilo despachador que una nueva tarea ha sido añadida. 
    tasksAvailable.signal();    
}

void ThreadPool::worker(int id) {
    // Cada worker ejecuta en un bucle. 
    while (true) {
        // cout << "Trabajador " << id << ": me duermo" << endl;
        // El worker se bloquea hasta que el dispatcher le asigne una tarea. 
        wts[id].taskReadySema.wait();

        // cout << "Trabajador " << id << ": me despierto" << endl;

        // Condición de salida: si el pool se está destruyendo.
        if (done) break;

        // Invoca la función asignada. 
        // cout << "Worker " << id << ": laburo" << endl;
        wts[id].thunk();
        
        // Decrementa el contador de tareas en progreso.
        tasksInProgress--;

        // Si no hay más tareas, notifica a los hilos que esperan en wait().
        if (tasksInProgress == 0) {
            unique_lock<mutex> lock(waitMutex);
            waitCondition.notify_all();
        }

        // El worker se marca a sí mismo como disponible nuevamente para el dispatcher. 
        {
            unique_lock<mutex> lock(workerManagementMutex);
            availableWorkerIds.push(id);
        }
        workersAvailable.signal();
    }
}

void ThreadPool::dispatcher() {
    // El dispatcher ejecuta en un bucle. 
    while (true) {
        // Duerme hasta que schedule le indique que se ha añadido algo a la cola. 
        tasksAvailable.wait();

        // Condición de salida: si el pool se está destruyendo y no hay más tareas.
        if (done && tasksInProgress == 0) break;
        
        // Espera a que un worker esté disponible. 
        workersAvailable.wait();
        
        // Extrae una tarea de la cola y un worker disponible. 
        function<void(void)> task;
        {
            unique_lock<mutex> lock(taskQueueMutex);
            task = taskQueue.front();
            taskQueue.pop();
        }

        int workerId;
        {
            unique_lock<mutex> lock(workerManagementMutex);
            workerId = availableWorkerIds.front();
            availableWorkerIds.pop();
        }
        
        // Asigna la tarea al worker y le notifica para que la ejecute. 
        wts[workerId].thunk = task;
        wts[workerId].taskReadySema.signal();
    }
}

void ThreadPool::wait() {
    // Bloquea y espera hasta que todas las tareas programadas se hayan ejecutado. 
    unique_lock<mutex> lock(waitMutex);
    waitCondition.wait(lock, [this] { return tasksInProgress == 0; });    
}

ThreadPool::~ThreadPool() {
    // Espera a que todas las funciones se hayan ejecutado. 
    wait();

    // Establece la bandera para indicar a los hilos que deben terminar.
    done = true;

    // Despierta al dispatcher para que pueda verificar la bandera 'done' y terminar.
    tasksAvailable.signal();
    dt.join(); // Espera a que el dispatcher termine.

    // Despierta a todos los workers que puedan estar esperando.
    for (size_t i = 0; i < wts.size(); ++i) {
        wts[i].taskReadySema.signal();
        wts[i].ts.join(); // Espera a que cada worker termine.
    }    
}
