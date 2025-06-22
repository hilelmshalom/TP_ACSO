#include "thread-pool.h"
#include <iostream>
#include <vector>
#include <stdexcept>
#include <functional>

using namespace std;

// Para mejorar la legibilidad, definimos un alias de tipo para una matriz.
using Matrix = vector<vector<int>>;

/**
 * @brief Imprime una matriz en la consola de forma ordenada.
 * @param mat La matriz a imprimir.
 */
void printMatrix(const Matrix& mat) {
    for (const auto& row : mat) {
        for (int val : row) {
            cout << val << "\t";
        }
        cout << endl;
    }
}

/**
 * @brief Función de trabajo que calcula una única fila de la matriz resultante.
 * C[row_idx] = A[row_idx] * B
 * @param a Matriz A (izquierda).
 * @param b Matriz B (derecha).
 * @param c Matriz C (resultado), se modifica por referencia.
 * @param row_idx El índice de la fila de C que esta tarea debe calcular.
 */
void multiplyRow(const Matrix& a, const Matrix& b, Matrix& c, size_t row_idx) {
    // Se asume que las dimensiones ya han sido validadas.
    size_t cols_b = b[0].size();
    size_t cols_a = a[0].size(); // que es igual a b.size()

    for (size_t j = 0; j < cols_b; ++j) {
        int sum = 0;
        for (size_t k = 0; k < cols_a; ++k) {
            sum += a[row_idx][k] * b[k][j];
        }
        c[row_idx][j] = sum;
    }
}

int main() {
    // --- Configuración del Problema ---
    const int NUM_THREADS = 4; // Usaremos 4 hilos para el cálculo.
    ThreadPool pool(NUM_THREADS);

    // Definimos las matrices de ejemplo.
    Matrix A = {
        {1, 2, 3, 4},
        {5, 6, 7, 8},
        {9, 10, 11, 12},
        {13, 14, 15, 16}
    };

    Matrix B = {
        {16, 15, 14, 13},
        {12, 11, 10, 9},
        {8, 7, 6, 5},
        {4, 3, 2, 1}
    };

    // Validamos que las dimensiones sean compatibles para la multiplicación.
    if (A.empty() || A[0].size() != B.size()) {
        cerr << "Error: Las dimensiones de las matrices no son compatibles para la multiplicación." << endl;
        return 1;
    }

    // Creamos la matriz resultado C con las dimensiones correctas (filas de A x columnas de B)
    // y la inicializamos con ceros.
    size_t rows_c = A.size();
    size_t cols_c = B[0].size();
    Matrix C(rows_c, vector<int>(cols_c, 0));

    cout << "Matriz A:" << endl;
    printMatrix(A);
    cout << "\nMatriz B:" << endl;
    printMatrix(B);
    
    // --- Planificación de Tareas en el ThreadPool ---
    // Cada tarea se encargará de calcular una fila de la matriz C.
    for (size_t i = 0; i < rows_c; ++i) {
        // Usamos una lambda para capturar las matrices y el índice de la fila.
        // Capturamos por referencia (&) para evitar copias costosas de las matrices.
        pool.schedule([&A, &B, &C, i]() {
            multiplyRow(A, B, C, i);
        });
    }

    // --- Sincronización y Muestra de Resultados ---
    cout << "\nCalculando la multiplicación de matrices usando " << NUM_THREADS << " hilos..." << endl;
    
    // `wait()` bloquea el hilo principal hasta que todas las tareas (cálculo de todas las filas)
    // hayan sido completadas por los workers del pool.
    pool.wait();

    cout << "\nMultiplicación completada. Matriz Resultante C:" << endl;
    printMatrix(C);

    return 0;
}