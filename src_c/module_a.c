#include "module_a.h"

int func_a(void) {
    return 37;
}

int func_cross_a(void) {
    return func_b();
}
