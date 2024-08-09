#ifndef _COLOR_MODULE_H
#define _COLOR_MODULE_H

#ifdef PYGAMEAPI_COLOR_INTERNAL

static PyTypeObject pgColor_Type;
static PyObject* pgColor_New(Uint8 rgba[]);
static int
pg_RGBAFromObjEx(PyObject *color, Uint8 rgba[],
                 pgColorHandleFlags handle_flags);
static PyObject *pgColor_NewLength(Uint8 rgba[], Uint8 length);
static int
pg_MappedColorFromObj(PyObject *val, SDL_PixelFormat *format, Uint32 *color,
                      pgColorHandleFlags handle_flags);

#else /* NOT PYGAMEAPI_COLOR_INTERNAL */

#define pgColor_Type (*(PyObject *)PYGAMEAPI_GET_SLOT(color, 0))
#define pgColor_CheckExact(x) ((x)->ob_type == &pgColor_Type)
#define pgColor_New (*(PyObject * (*)(Uint8 *)) PYGAMEAPI_GET_SLOT(color, 1))
#define pg_RGBAFromObjEx                                                    \
    (*(int (*)(PyObject *, Uint8 *, pgColorHandleFlags))PYGAMEAPI_GET_SLOT( \
        color, 2))
#define pgColor_NewLength \
    (*(PyObject * (*)(Uint8 *, Uint8)) PYGAMEAPI_GET_SLOT(color, 3))
#define pg_MappedColorFromObj                           \
    (*(int (*)(PyObject *, SDL_PixelFormat *, Uint32 *, \
               pgColorHandleFlags))PYGAMEAPI_GET_SLOT(color, 4))
#define pgColor_AsArray(x) (((pgColorObject *)x)->data)
#define pgColor_NumComponents(x) (((pgColorObject *)x)->len)

#define import_pygame_color() IMPORT_PYGAME_MODULE(color)

#endif

PyMODINIT_FUNC PgCreateColorModule(void);

#endif
