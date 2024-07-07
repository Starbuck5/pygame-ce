#define NO_PYGAME_C_API

#include "pygame.h"

#include "module_a.h"
#include "module_b.h"

MODINIT_DEFINE(core) {
    PyObject *module;

    static struct PyModuleDef _module = {PyModuleDef_HEAD_INIT,
                                         "core",
                                         "CORE MDOULE",
                                         -1,
                                         NULL,
                                         NULL,
                                         NULL,
                                         NULL,
                                         NULL};

    printf("%i\n", func_a());
    printf("%i\n", func_b());
    printf("%i\n", func_cross_a());
    printf("%i\n", func_cross_b());

    /* create the module */
    module = PyModule_Create(&_module);
    if (module == NULL) {
        return NULL;
    }

    PyObject *module_a = construct_module_a();
    PyModule_AddObject(module, "module_a3", module_a);

    return module;
}
