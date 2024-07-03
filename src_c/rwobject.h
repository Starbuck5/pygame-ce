/*
 *  SDL_RWops support for python objects
 */
#define NO_PYGAME_C_API
#define PYGAMEAPI_RWOBJECT_INTERNAL
#include "pygame.h"

#include "pgcompat.h"

#include "doc/pygame_doc.h"

SDL_RWops *
pgRWops_FromObject(PyObject *obj, char **extptr);

PyObject *
pg_EncodeString(PyObject *obj, const char *encoding, const char *errors,
                PyObject *eclass);

PyObject *
pg_EncodeFilePath(PyObject *obj, PyObject *eclass);

int
pgRWops_IsFileObject(SDL_RWops *rw);

SDL_RWops *
pgRWops_FromFileObject(PyObject *obj);

PyObject *
pg_encode_file_path(PyObject *self, PyObject *args, PyObject *keywds);

PyObject *
pg_encode_string(PyObject *self, PyObject *args, PyObject *keywds);
