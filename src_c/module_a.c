#include "module_a.h"

int func_a(void) {
    return 37;
}

int func_cross_a(void) {
    return func_b();
}

PyObject* construct_module_a(void)
{
    PyObject *module;

    static struct PyModuleDef _module = {PyModuleDef_HEAD_INIT,
                                         "module_a1",
                                         "module_a2",
                                         -1,
                                         NULL,
                                         NULL,
                                         NULL,
                                         NULL,
                                         NULL};

    /* create the module */
    module = PyModule_Create(&_module);
    if (module == NULL) {
        return NULL;
    }

    return module;
}

