#ifndef _MODULE_A_H
#define _MODULE_A_H
#define NO_PYGAME_C_API

#include "pygame.h"

#include "pgcompat.h"

#include "module_b.h"

int func_a(void);

int func_cross_a(void);

PyObject* construct_module_a(void);

#endif
