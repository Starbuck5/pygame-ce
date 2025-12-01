#include "pygame.h"
#include "pgcompat.h"

// ***************************************************************************
// OVERALL DEFINITIONS
// ***************************************************************************

typedef struct {
    bool audio_initialized;
    PyObject *audio_device_type;
    PyObject *audio_device_state_type;
    PyObject *audio_stream_state_type;
} audio_state;

#define GET_STATE(x) (audio_state *)PyModule_GetState(x)

typedef struct {
    PyObject_HEAD SDL_AudioDeviceID devid;
} PGAudioDeviceObject;

typedef struct {
    PyObject_HEAD SDL_AudioDeviceID devid;
} PGAudioDeviceStateObject;

typedef struct {
    PyObject_HEAD SDL_AudioStream *stream;
} PGAudioStreamStateObject;

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

static PyType_Spec adevice_spec = {
    .name = "BaseAudioDevice",
    .basicsize = sizeof(PGAudioDeviceObject),
    .itemsize = 0,
    // todo apparently needs to support GC
    // https://docs.python.org/3/c-api/typeobj.html#c.Py_TPFLAGS_HEAPTYPE
    .flags = Py_TPFLAGS_BASETYPE,
    .slots = adevice_slots};

// ***************************************************************************
// AUDIO.AUDIODEVICESTATE CLASS
// ***************************************************************************

static PyType_Slot adevice_state_slots[] = {{0, NULL}};

static PyType_Spec adevice_state_spec = {
    .name = "AudioDeviceState",
    .basicsize = sizeof(PGAudioDeviceStateObject),
    .itemsize = 0,
    // todo apparently needs to support GC
    // https://docs.python.org/3/c-api/typeobj.html#c.Py_TPFLAGS_HEAPTYPE
    .flags = 0,
    .slots = adevice_state_slots};

// ***************************************************************************
// AUDIO.AUDIOSTREAMSTATE CLASS
// ***************************************************************************

static PyType_Slot astream_state_slots[] = {{0, NULL}};

static PyType_Spec astream_state_spec = {
    .name = "AudioStreamState",
    .basicsize = sizeof(PGAudioStreamStateObject),
    .itemsize = 0,
    // todo apparently needs to support GC
    // https://docs.python.org/3/c-api/typeobj.html#c.Py_TPFLAGS_HEAPTYPE
    .flags = 0,
    .slots = astream_state_slots};

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
    // audio_state *state = GET_STATE(module);
    // PyTypeObject *adevice_type = (PyTypeObject *)state->audio_device_type;

    PyTypeObject *adevice_type =
        (PyTypeObject *)PyObject_GetAttrString(module, "AudioDevice");
    if (adevice_type == NULL) {
        return NULL;
    }

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
pg_audio_get_playback_device_states(PyObject *module, PyObject *_null)
{
    audio_state *state = GET_STATE(module);
    PyTypeObject *adevice_state_type =
        (PyTypeObject *)state->audio_device_state_type;

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
    PGAudioDeviceStateObject *device;
    for (int i = 0; i < num_devices; i++) {
        device = (PGAudioDeviceStateObject *)adevice_state_type->tp_alloc(
            adevice_state_type, 0);
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
pg_audio_get_audio_device_name(PyObject *module, PyObject *const *args,
                               Py_ssize_t nargs)
{
    // assert nargs == 1
    // assert type(args[0]) == AudioDeviceState
    SDL_AudioDeviceID devid = ((PGAudioDeviceStateObject *)args[0])->devid;
    const char *name = SDL_GetAudioDeviceName(devid);
    if (name == NULL) {
        return RAISE(pgExc_SDLError, SDL_GetError());
    }
    return PyUnicode_FromString(name);
}

static PyObject *
pg_audio_create_audio_stream(PyObject *module, PyObject *const *args,
                             Py_ssize_t nargs)
{
    // SDL_CreateAudioStream
    //  src_format: int, src_channels: int, src_frequency: int,
    //  dst_format: int, dst_channels: int, dst_frequency: int

    audio_state *state = GET_STATE(module);
    PyTypeObject *astream_state_type =
        (PyTypeObject *)state->audio_stream_state_type;

    SDL_AudioSpec src, dst;

    src.format = PyLong_AsInt(args[0]);
    src.channels = PyLong_AsInt(args[1]);
    src.freq = PyLong_AsInt(args[2]);
    dst.format = PyLong_AsInt(args[3]);
    dst.channels = PyLong_AsInt(args[4]);
    dst.freq = PyLong_AsInt(args[5]);

    // Check that they all succeeded
    if (src.format == -1 || src.channels == -1 || src.freq == -1 ||
        dst.format == -1 || dst.channels == -1 || dst.freq == -1) {
        if (PyErr_Occurred()) {
            return NULL;
        }
    }

    SDL_AudioStream *stream = SDL_CreateAudioStream(&src, &dst);
    if (stream == NULL) {
        return RAISE(pgExc_SDLError, SDL_GetError());
    }

    PGAudioStreamStateObject *stream_state =
        (PGAudioStreamStateObject *)astream_state_type->tp_alloc(
            astream_state_type, 0);
    stream_state->stream = stream;

    return (PyObject *)stream_state;
}

static PyObject *
pg_audio_put_audio_stream_data(PyObject *module, PyObject *const *args,
                               Py_ssize_t nargs)
{
    // SDL_PutAudioStreamData
    // stream_state: PGAudioStreamState, data: Buffer

    SDL_AudioStream *stream = ((PGAudioStreamStateObject *)args[0])->stream;

    PyObject *bytes = PyBytes_FromObject(args[1]);
    if (bytes == NULL) {
        return NULL;
    }

    void *buf;
    int len;

    if (PyBytes_AsStringAndSize(bytes, (char **)&buf, (Py_ssize_t *)&len) !=
        0) {
        Py_DECREF(bytes);
        return NULL;
    }

    if (!SDL_PutAudioStreamData(stream, buf, len)) {
        Py_DECREF(bytes);
        return RAISE(pgExc_SDLError, SDL_GetError());
    }

    Py_RETURN_NONE;
}

static PyObject *
pg_audio_get_audio_stream_data(PyObject *module, PyObject *const *args,
                               Py_ssize_t nargs)
{
    // SDL_GetAudioStreamData
    // stream_state: PGAudioStreamState, size: int

    SDL_AudioStream *stream = ((PGAudioStreamStateObject *)args[0])->stream;

    int size = PyLong_AsInt(args[1]);
    if (size == -1 && PyErr_Occurred()) {
        return NULL;
    }

    void *buf = malloc(size);
    if (buf == NULL) {
        return PyErr_NoMemory();
    }

#if 0
    PyObject *bytes = PyBytes_FromStringAndSize(NULL, size);
    if (bytes == NULL) {
        return NULL;
    }

    void *buf = PyBytes_AsString(bytes);
    if (buf == NULL) {
        Py_DECREF(bytes);
        return NULL;
    }
#endif

    int bytes_read = SDL_GetAudioStreamData(stream, buf, size);

    if (bytes_read == -1) {
        free(buf);
        //Py_DECREF(bytes);
        return RAISE(pgExc_SDLError, SDL_GetError());        
    }

    PyObject *bytes = PyBytes_FromStringAndSize(buf, bytes_read);
    free(buf);
    if (bytes == NULL) {
        return NULL;
    }

    return bytes;
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

static PyObject *
pg_audio_get_silence_value_for_format(PyObject *module, PyObject *const *args,
                                      Py_ssize_t nargs)
{
    // SDL_GetSilenceValueForFormat
    // format: int

    int format_num = PyLong_AsInt(args[0]);
    if (format_num == -1 && PyErr_Occurred()) {
        return NULL;
    }

    int silence_value =
        SDL_GetSilenceValueForFormat((SDL_AudioFormat)format_num);

    return PyBytes_FromFormat("%c", silence_value);
}

static PyMethodDef audio_methods[] = {
    {"init", (PyCFunction)pg_audio_init, METH_NOARGS, "TODO"},
    {"quit", (PyCFunction)pg_audio_quit, METH_NOARGS, "TODO"},
    {"get_current_driver", (PyCFunction)pg_audio_get_current_driver,
     METH_NOARGS, "TODO"},
    {"get_drivers", (PyCFunction)pg_audio_get_drivers, METH_NOARGS, "TODO"},
    {"get_playback_devices", (PyCFunction)pg_audio_get_playback_devices,
     METH_NOARGS, "TODO"},
    {"get_playback_device_states",
     (PyCFunction)pg_audio_get_playback_device_states, METH_NOARGS, "TODO"},
    {"get_audio_device_name", (PyCFunction)pg_audio_get_audio_device_name,
     METH_FASTCALL, "TODO"},
    {"get_recording_devices", (PyCFunction)pg_audio_get_recording_devices,
     METH_NOARGS, "TODO"},

    // format utility (the one)
    {"get_silence_value_for_format",
     (PyCFunction)pg_audio_get_silence_value_for_format, METH_FASTCALL, NULL},

    // AudioStream utilities
    {"create_audio_stream", (PyCFunction)pg_audio_create_audio_stream,
     METH_FASTCALL, "TODO"},
    {"put_audio_stream_data", (PyCFunction)pg_audio_put_audio_stream_data,
     METH_FASTCALL, "TODO"},
    {"get_audio_stream_data", (PyCFunction)pg_audio_get_audio_stream_data,
     METH_FASTCALL, "TODO"},

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
    if (PyModule_AddObjectRef(module, "BaseAudioDevice", audio_device_type) <
        0) {
        return -1;
    }

    audio_state *state = GET_STATE(module);
    state->audio_initialized = false;
    state->audio_device_type = audio_device_type;

    PyObject *audio_device_state_type =
        PyType_FromModuleAndSpec(module, &adevice_state_spec, NULL);
    if (PyModule_AddObjectRef(module, "AudioDeviceState", audio_device_type) <
        0) {
        return -1;
    }

    state->audio_device_state_type = audio_device_state_type;

    PyObject *audio_stream_state_type =
        PyType_FromModuleAndSpec(module, &astream_state_spec, NULL);
    if (PyModule_AddObjectRef(module, "AudioStreamState",
                              audio_stream_state_type) < 0) {
        return -1;
    }

    state->audio_stream_state_type = audio_stream_state_type;

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
