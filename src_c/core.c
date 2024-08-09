#define NO_PYGAME_C_API

#include "pygame.h"
#include "pgcompat.h"

#define PYGAMEAPI_COLOR_INTERNAL
#include "color_api.h"

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

    /* create the module */
    module = PyModule_Create(&_module);
    if (module == NULL) {
        return NULL;
    }

    PyObject* color_module = PgCreateColorModule();
    if (color_module == NULL) {
        return NULL;
    }
    Py_INCREF(color_module);
    if (PyModule_AddObject(module, "color", color_module)) {
        Py_XDECREF(color_module);
        return NULL;
    }

    return module;
}
