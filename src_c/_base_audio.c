#include "pygame.h"
#include "pgcompat.h"

// ***************************************************************************
// OVERALL DEFINITIONS
// ***************************************************************************

typedef struct {
    bool audio_initialized;
    PyObject *audio_device_type;
} audio_state;

#define GET_STATE(x) (audio_state *)PyModule_GetState(x)

typedef struct {
    PyObject_HEAD SDL_AudioDeviceID devid;
} PGAudioDeviceObject;

typedef struct {
    PyObject_HEAD
} PGLogicalAudioDeviceObject;

typedef struct {
    PyObject_HEAD
} PGAudioStreamObject;

#define AUDIO_INIT_CHECK(module)                               \
    if (!(GET_STATE(module))->audio_initialized) {             \
        return RAISE(pgExc_SDLError, "audio not initialized"); \
    }

// ***************************************************************************
// AUDIO.AUDIODEVICE CLASS
// ***************************************************************************

static PyObject *
pg_adevice_obj_get_channel_map(PGAudioDeviceObject *self, PyObject *_null)
{
    int count;

    int *channel_map = SDL_GetAudioDeviceChannelMap(self->devid, &count);
    if (channel_map == NULL) {
        Py_RETURN_NONE;
    }

    PyObject *channel_map_list = PyList_New(count);
    PyObject *item;
    for (int i = 0; i < count; i++) {
        item = PyLong_FromLong(channel_map[i]);
        if (item == NULL) {
            SDL_free(channel_map);
            Py_DECREF(channel_map_list);
            return NULL;
        }
        PyList_SetItem(channel_map_list, i, item);
    }

    SDL_free(channel_map);
    return channel_map_list;
}

static PyObject *
pg_adevice_obj_get_name(PGAudioDeviceObject *self, void *_null)
{
    const char *name = SDL_GetAudioDeviceName(self->devid);
    if (name == NULL) {
        return RAISE(pgExc_SDLError, SDL_GetError());
    }
    return PyUnicode_FromString(name);
}

static PyGetSetDef adevice_obj_getsets[] = {
    {"name", (getter)pg_adevice_obj_get_name, NULL, "TODO", NULL},
    {NULL, NULL, NULL, NULL, NULL}};

static PyMethodDef adevice_obj_methods[] = {
    {"get_channel_map", (PyCFunction)pg_adevice_obj_get_channel_map,
     METH_NOARGS, "TODO"},
    {NULL, NULL, 0, NULL}};

static PyType_Slot adevice_slots[] = {{Py_tp_methods, adevice_obj_methods},
                                      {Py_tp_getset, adevice_obj_getsets},
                                      {0, NULL}};

static PyType_Spec adevice_spec = {.name = "AudioDevice",
                                   .basicsize = sizeof(PGAudioDeviceObject),
                                   .itemsize = 0,
                                   .flags = 0,
                                   .slots = adevice_slots};

// ***************************************************************************
// MODULE METHODS
// ***************************************************************************

static PyObject *
pg_audio_init(PyObject *module, PyObject *_null)
{
    audio_state *state = GET_STATE(module);
    if (!state->audio_initialized) {
        if (!SDL_InitSubSystem(SDL_INIT_AUDIO)) {
            return RAISE(pgExc_SDLError, SDL_GetError());
        }
        state->audio_initialized = true;
    }
    Py_RETURN_NONE;
}

static PyObject *
pg_audio_quit(PyObject *module, PyObject *_null)
{
    audio_state *state = GET_STATE(module);
    if (state->audio_initialized) {
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
        state->audio_initialized = false;
    }
    Py_RETURN_NONE;
}

static PyObject *
pg_audio_get_current_driver(PyObject *module, PyObject *_null)
{
    AUDIO_INIT_CHECK(module);

    const char *driver = SDL_GetCurrentAudioDriver();
    if (driver != NULL) {
        return PyUnicode_FromString(driver);
    }
    return RAISE(pgExc_SDLError, SDL_GetError());
}

static PyObject *
pg_audio_get_drivers(PyObject *module, PyObject *_null)
{
    int num_drivers = SDL_GetNumAudioDrivers();

    PyObject *driver_list = PyList_New(num_drivers);
    PyObject *item;
    const char *driver;
    for (int i = 0; i < num_drivers; i++) {
        driver = SDL_GetAudioDriver(i);
        if (driver == NULL) {
            return RAISE(pgExc_SDLError, SDL_GetError());
            Py_DECREF(driver_list);
        }
        item = PyUnicode_FromString(driver);
        if (item == NULL) {
            Py_DECREF(driver_list);
            return NULL;
        }
        PyList_SetItem(driver_list, i, item);
    }

    return driver_list;
}

static PyObject *
pg_audio_get_playback_devices(PyObject *module, PyObject *_null)
{
    audio_state *state = GET_STATE(module);
    PyTypeObject *adevice_type = (PyTypeObject *)state->audio_device_type;

    int num_devices;
    SDL_AudioDeviceID *devices = SDL_GetAudioPlaybackDevices(&num_devices);

    if (devices == NULL) {
        return RAISE(pgExc_SDLError, SDL_GetError());
    }

    PyObject *device_list = PyList_New(num_devices);
    if (device_list == NULL) {
        SDL_free(devices);
        return NULL;
    }
    PGAudioDeviceObject *device;
    for (int i = 0; i < num_devices; i++) {
        device =
            (PGAudioDeviceObject *)adevice_type->tp_alloc(adevice_type, 0);
        if (device == NULL) {
            SDL_free(devices);
            Py_DECREF(device_list);
            return NULL;
        }
        device->devid = devices[i];
        if (PyList_SetItem(device_list, i, (PyObject *)device) < 0) {
            SDL_free(devices);
            Py_DECREF(device);
            Py_DECREF(device_list);
        }
    }

    return device_list;
}

static PyObject *
pg_audio_get_recording_devices(PyObject *module, PyObject *_null)
{
    audio_state *state = GET_STATE(module);
    PyTypeObject *adevice_type = (PyTypeObject *)state->audio_device_type;

    int num_devices;
    SDL_AudioDeviceID *devices = SDL_GetAudioRecordingDevices(&num_devices);

    if (devices == NULL) {
        return RAISE(pgExc_SDLError, SDL_GetError());
    }

    PyObject *device_list = PyList_New(num_devices);
    if (device_list == NULL) {
        SDL_free(devices);
        return NULL;
    }
    PGAudioDeviceObject *device;
    for (int i = 0; i < num_devices; i++) {
        device =
            (PGAudioDeviceObject *)adevice_type->tp_alloc(adevice_type, 0);
        if (device == NULL) {
            SDL_free(devices);
            Py_DECREF(device_list);
            return NULL;
        }
        device->devid = devices[i];
        if (PyList_SetItem(device_list, i, (PyObject *)device) < 0) {
            SDL_free(devices);
            Py_DECREF(device);
            Py_DECREF(device_list);
        }
    }

    return device_list;
}

static PyMethodDef audio_methods[] = {
    {"init", (PyCFunction)pg_audio_init, METH_NOARGS, "TODO"},
    {"quit", (PyCFunction)pg_audio_quit, METH_NOARGS, "TODO"},
    {"get_current_driver", (PyCFunction)pg_audio_get_current_driver,
     METH_NOARGS, "TODO"},
    {"get_drivers", (PyCFunction)pg_audio_get_drivers, METH_NOARGS, "TODO"},
    {"get_playback_devices", (PyCFunction)pg_audio_get_playback_devices,
     METH_NOARGS, "TODO"},
    {"get_recording_devices", (PyCFunction)pg_audio_get_recording_devices,
     METH_NOARGS, "TODO"},
    {NULL, NULL, 0, NULL}};

// ***************************************************************************
// MODULE SETUP
// ***************************************************************************

int
exec_audio(PyObject *module)
{
    /*imported needed apis*/
    import_pygame_base();
    if (PyErr_Occurred()) {
        return -1;
    }
    import_pygame_rwobject();
    if (PyErr_Occurred()) {
        return -1;
    }

    PyObject *audio_device_type =
        PyType_FromModuleAndSpec(module, &adevice_spec, NULL);
    if (PyModule_AddObjectRef(module, "AudioDevice", audio_device_type) < 0) {
        return -1;
    }

    audio_state *state = GET_STATE(module);
    state->audio_initialized = false;
    state->audio_device_type = audio_device_type;

    return 0;
}

MODINIT_DEFINE(_base_audio)
{
    static PyModuleDef_Slot audio_slots[] = {
        {Py_mod_exec, &exec_audio},
#if PY_VERSION_HEX >= 0x030c0000
        {Py_mod_multiple_interpreters,
         Py_MOD_MULTIPLE_INTERPRETERS_NOT_SUPPORTED},  // TODO: see if this can
                                                       // be supported later
#endif
#if PY_VERSION_HEX >= 0x030d0000
        {Py_mod_gil, Py_MOD_GIL_USED},  // TODO: support this later
#endif
        {0, NULL}};
    static struct PyModuleDef _module = {PyModuleDef_HEAD_INIT,
                                         "_base_audio",
                                         "DOC TODO",
                                         sizeof(audio_state),
                                         audio_methods,
                                         audio_slots,
                                         NULL,
                                         NULL,
                                         NULL};

    return PyModuleDef_Init(&_module);
}
