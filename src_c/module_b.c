#include "module_b.h"

int func_b(void) {
    return 42;
}

int func_cross_b(void) {
    return func_a();
}
