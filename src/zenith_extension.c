#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <stdint.h>
#include <stddef.h>

/* Declarations of raw x86-64 assembly routines */
extern int64_t zenith_asm_fib(int64_t n);
extern int64_t zenith_asm_loop_sum(int64_t count);
extern double zenith_asm_vector_dot_avx2(const double* a, const double* b, size_t n);

static PyObject* py_zenith_asm_fib(PyObject* self, PyObject* args) {
    (void)self;
    long long n;
    if (!PyArg_ParseTuple(args, "L", &n)) {
        return NULL;
    }
    int64_t res = zenith_asm_fib((int64_t)n);
    return PyLong_FromLongLong(res);
}

static PyObject* py_zenith_asm_loop_sum(PyObject* self, PyObject* args) {
    (void)self;
    long long count;
    if (!PyArg_ParseTuple(args, "L", &count)) {
        return NULL;
    }
    int64_t res = zenith_asm_loop_sum((int64_t)count);
    return PyLong_FromLongLong(res);
}

static PyObject* py_zenith_asm_vector_dot(PyObject* self, PyObject* args) {
    (void)self;
    PyObject *list_a, *list_b;
    if (!PyArg_ParseTuple(args, "O!O!", &PyList_Type, &list_a, &PyList_Type, &list_b)) {
        return NULL;
    }

    Py_ssize_t n_a = PyList_Size(list_a);
    Py_ssize_t n_b = PyList_Size(list_b);
    if (n_a != n_b) {
        PyErr_SetString(PyExc_ValueError, "Lists must have the same length");
        return NULL;
    }

    double* arr_a = (double*)malloc(sizeof(double) * n_a);
    double* arr_b = (double*)malloc(sizeof(double) * n_b);
    if (!arr_a || !arr_b) {
        free(arr_a);
        free(arr_b);
        return PyErr_NoMemory();
    }

    for (Py_ssize_t i = 0; i < n_a; ++i) {
        arr_a[i] = PyFloat_AsDouble(PyList_GET_ITEM(list_a, i));
        arr_b[i] = PyFloat_AsDouble(PyList_GET_ITEM(list_b, i));
    }

    double dot = zenith_asm_vector_dot_avx2(arr_a, arr_b, (size_t)n_a);

    free(arr_a);
    free(arr_b);

    return PyFloat_FromDouble(dot);
}

extern int64_t zenith_asm_prime_count(int64_t limit);
extern int64_t zenith_asm_mandel_pixel(double cr, double ci, int64_t max_iter);

static PyObject* py_zenith_asm_prime_count(PyObject* self, PyObject* args) {
    (void)self;
    long long limit;
    if (!PyArg_ParseTuple(args, "L", &limit)) {
        return NULL;
    }
    int64_t count = zenith_asm_prime_count((int64_t)limit);
    return PyLong_FromLongLong(count);
}

static PyObject* py_zenith_asm_mandel_pixel(PyObject* self, PyObject* args) {
    (void)self;
    double cr, ci;
    long long max_iter;
    if (!PyArg_ParseTuple(args, "ddL", &cr, &ci, &max_iter)) {
        return NULL;
    }
    int64_t iter = zenith_asm_mandel_pixel(cr, ci, (int64_t)max_iter);
    return PyLong_FromLongLong(iter);
}

static PyMethodDef ZenithMethods[] = {
    {"asm_fib", py_zenith_asm_fib, METH_VARARGS, "Execute recursive Fibonacci via raw x86-64 assembly."},
    {"asm_loop_sum", py_zenith_asm_loop_sum, METH_VARARGS, "Execute loop accumulation via raw x86-64 assembly pipeline."},
    {"asm_vector_dot", py_zenith_asm_vector_dot, METH_VARARGS, "Execute AVX2/FMA vectorized dot product in raw assembly."},
    {"asm_prime_count", py_zenith_asm_prime_count, METH_VARARGS, "Count primes up to limit in raw x86-64 hardware registers."},
    {"asm_mandel_pixel", py_zenith_asm_mandel_pixel, METH_VARARGS, "Execute Mandelbrot complex pixel kernel in raw assembly."},
    {NULL, NULL, 0, NULL}
};


static struct PyModuleDef zenith_module = {
    PyModuleDef_HEAD_INIT,
    "zenith_accelerator",
    "CPython Native Assembly Acceleration Extension",
    -1,
    ZenithMethods,
    NULL,
    NULL,
    NULL,
    NULL
};


PyMODINIT_FUNC PyInit_zenith_accelerator(void) {
    return PyModule_Create(&zenith_module);
}
