#include <SDL3_mixer/SDL_mixer.h>
#include "pygame.h"
#include "pgcompat.h"

// ***************************************************************************
// OVERALL DEFINITIONS
// ***************************************************************************

typedef struct {
    bool mixer_initialized;
} _mixer_state;

#define GET_STATE(x) (_mixer_state *)PyModule_GetState(x)

typedef struct {
    PyObject_HEAD MIX_Mixer *mixer;
} PGMixerObject;

typedef struct {
    PyObject_HEAD MIX_Audio *audio;
} PGAudioObject;

// ***************************************************************************
// MIXER.MIXER CLASS
// ***************************************************************************

static PyObject *
pg_mixer_obj_play(PGMixerObject *self, PyObject *arg)
{
    if (!PyObject_IsInstance(
            arg, PyObject_GetAttrString((PyObject *)self, "_audio_type"))) {
        return RAISE(PyExc_TypeError, "audio must be an Audio");
    }

    PGAudioObject *audio = (PGAudioObject *)arg;
    if (!MIX_PlayAudio(self->mixer, audio->audio)) {
        return RAISE(pgExc_SDLError, SDL_GetError());
    }
    Py_RETURN_NONE;
}

static PyObject *
pg_mixer_obj_stop_tag(PGMixerObject *self, PyObject *args, PyObject *kwargs)
{
    char *tag;
    int64_t fade_out_ms = 0;
    char *keywords[] = {"tag", "fade_out_ms", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s|L", keywords, &tag,
                                     &fade_out_ms)) {
        return NULL;
    }

    if (!MIX_StopTag(self->mixer, tag, fade_out_ms)) {
        return RAISE(pgExc_SDLError, SDL_GetError());
    }

    Py_RETURN_NONE;
}

static PyObject *
pg_mixer_obj_pause_tag(PGMixerObject *self, PyObject *args, PyObject *kwargs)
{
    char *tag;
    char *keywords[] = {"tag", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s", keywords, &tag)) {
        return NULL;
    }

    if (!MIX_PauseTag(self->mixer, tag)) {
        return RAISE(pgExc_SDLError, SDL_GetError());
    }

    Py_RETURN_NONE;
}

static PyObject *
pg_mixer_obj_resume_tag(PGMixerObject *self, PyObject *args, PyObject *kwargs)
{
    char *tag;
    char *keywords[] = {"tag", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s", keywords, &tag)) {
        return NULL;
    }

    if (!MIX_ResumeTag(self->mixer, tag)) {
        return RAISE(pgExc_SDLError, SDL_GetError());
    }

    Py_RETURN_NONE;
}

static PyObject *
pg_mixer_obj_set_tag_gain(PGMixerObject *self, PyObject *args,
                          PyObject *kwargs)
{
    char *tag;
    float gain;
    char *keywords[] = {"tag", "gain", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|L", keywords, &tag,
                                     &gain)) {
        return NULL;
    }

    if (!MIX_SetTagGain(self->mixer, tag, gain)) {
        return RAISE(pgExc_SDLError, SDL_GetError());
    }

    Py_RETURN_NONE;
}

static PyObject *
pg_mixer_obj_stop_all_tracks(PGMixerObject *self, PyObject *args,
                             PyObject *kwargs)
{
    int64_t fade_out_ms = 0;
    char *keywords[] = {"fade_out_ms", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s", keywords,
                                     &fade_out_ms)) {
        return NULL;
    }

    if (!MIX_StopAllTracks(self->mixer, fade_out_ms)) {
        return RAISE(pgExc_SDLError, SDL_GetError());
    }

    Py_RETURN_NONE;
}

static PyObject *
pg_mixer_obj_pause_all_tracks(PGMixerObject *self, PyObject *_null)
{
    if (!MIX_PauseAllTracks(self->mixer)) {
        return RAISE(pgExc_SDLError, SDL_GetError());
    }
    Py_RETURN_NONE;
}

static PyObject *
pg_mixer_obj_resume_all_tracks(PGMixerObject *self, PyObject *_null)
{
    if (!MIX_ResumeAllTracks(self->mixer)) {
        return RAISE(pgExc_SDLError, SDL_GetError());
    }
    Py_RETURN_NONE;
}

static int
pg_mixer_obj_init(PGMixerObject *self, PyObject *args, PyObject *kwargs)
{
    printf("self_mp=%p\n", self->mixer);

    self->mixer =
        MIX_CreateMixerDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, NULL);
    if (self->mixer == NULL) {
        PyErr_SetString(pgExc_SDLError, SDL_GetError());
        return -1;
    }

    return 0;
}

static PyMethodDef mixer_methods[] = {
    {"play_audio", (PyCFunction)pg_mixer_obj_play, METH_O, "TODO"},
    {"stop_tag", (PyCFunction)pg_mixer_obj_stop_tag,
     METH_VARARGS | METH_KEYWORDS, "TODO"},
    {"pause_tag", (PyCFunction)pg_mixer_obj_pause_tag,
     METH_VARARGS | METH_KEYWORDS, "TODO"},
    {"resume_tag", (PyCFunction)pg_mixer_obj_resume_tag,
     METH_VARARGS | METH_KEYWORDS, "TODO"},
    {"set_tag_gain", (PyCFunction)pg_mixer_obj_set_tag_gain,
     METH_VARARGS | METH_KEYWORDS, "TODO"},

    {"stop_all_tracks", (PyCFunction)pg_mixer_obj_stop_all_tracks,
     METH_VARARGS | METH_KEYWORDS, "TODO"},
    {"pause_all_tracks", (PyCFunction)pg_mixer_obj_pause_all_tracks,
     METH_NOARGS, "TODO"},
    {"resume_all_tracks", (PyCFunction)pg_mixer_obj_resume_all_tracks,
     METH_NOARGS, "TODO"},
    {NULL, NULL, 0, NULL}};

static PyObject *
pg_mixer_obj_get_gain(PGMixerObject *self, void *_null)
{
    return PyFloat_FromDouble(MIX_GetMasterGain(self->mixer));
}

static int
pg_mixer_obj_set_gain(PGMixerObject *self, PyObject *value, void *_null)
{
    double gain = PyFloat_AsDouble(value);
    if (gain == -1.0 && PyErr_Occurred()) {
        return -1;
    }
    if (!MIX_SetMasterGain(self->mixer, (float)gain)) {
        PyErr_SetString(pgExc_SDLError, SDL_GetError());
        return -1;
    }
    return 0;
}

static PyGetSetDef mixer_obj_getsets[] = {
    {"gain", (getter)pg_mixer_obj_get_gain, (setter)pg_mixer_obj_set_gain,
     "TODO", NULL},
    {NULL, NULL, NULL, NULL, NULL}};

static PyType_Slot mixer_slots[] = {{Py_tp_methods, mixer_methods},
                                    {Py_tp_init, pg_mixer_obj_init},
                                    {Py_tp_getset, mixer_obj_getsets},
                                    {0, NULL}};

static PyType_Spec mixer_spec = {.name = "Mixer",
                                 .basicsize = sizeof(PGMixerObject),
                                 .itemsize = 0,
                                 .flags = 0,
                                 .slots = mixer_slots};

// ***************************************************************************
// MIXER.AUDIO CLASS
// ***************************************************************************

static int
pg_audio_obj_init(PGAudioObject *self, PyObject *args, PyObject *kwargs)
{
    int predecode = 0;
    PyObject *file = NULL;

    char *keywords[] = {"file", "predecode", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|pO!", keywords, &file,
                                     &predecode)) {
        return -1;
    }

    SDL_IOStream *io = pgRWops_FromObject(file, NULL);
    if (io == NULL) {
        return -1;
    }

    self->audio = MIX_LoadAudio_IO(NULL, io, predecode, true);
    if (self->audio == NULL) {
        PyErr_SetString(pgExc_SDLError, SDL_GetError());
        return -1;
    }

    printf("file=%p, predecode=%i\n", file, predecode);

    return 0;
}

static PyType_Slot audio_slots[] = {{Py_tp_init, pg_audio_obj_init},
                                    {0, NULL}};

static PyType_Spec audio_spec = {.name = "Audio",
                                 .basicsize = sizeof(PGAudioObject),
                                 .itemsize = 0,
                                 .flags = 0,
                                 .slots = audio_slots};

// ***************************************************************************
// MODULE METHODS
// ***************************************************************************

static PyObject *
pg_mixer_init(PyObject *module, PyObject *_null)
{
    _mixer_state *state = GET_STATE(module);
    if (!state->mixer_initialized) {
        if (!MIX_Init()) {
            return RAISE(pgExc_SDLError, SDL_GetError());
        }
        state->mixer_initialized = true;
    }
    Py_RETURN_NONE;
}

static PyObject *
pg_mixer_quit(PyObject *module, PyObject *_null)
{
    _mixer_state *state = GET_STATE(module);
    if (state->mixer_initialized) {
        MIX_Quit();
        state->mixer_initialized = false;
    }
    Py_RETURN_NONE;
}

static PyObject *
pg_mixer_get_decoders(PyObject *module, PyObject *_null)
{
    _mixer_state *state = GET_STATE(module);
    if (!state->mixer_initialized) {
        return RAISE(pgExc_SDLError, "mixer not initialized");
    }

    int num_decoders = MIX_GetNumAudioDecoders();
    PyObject *decoders = PyList_New(num_decoders);
    if (decoders == NULL) {
        return NULL;  // error already set
    }

    for (int i = 0; i < num_decoders; i++) {
        PyObject *decoder = PyUnicode_FromString(MIX_GetAudioDecoder(i));
        if (decoder == NULL || PyList_SetItem(decoders, i, decoder)) {
            Py_DECREF(decoders);
            return NULL;  // error already set
        }
    }

    return decoders;
}

static PyMethodDef _mixer_methods[] = {
    {"init", (PyCFunction)pg_mixer_init, METH_NOARGS, "DOC_MIXER_INIT"},
    {"quit", (PyCFunction)pg_mixer_quit, METH_NOARGS, "DOC_MIXER_QUIT"},
    {"get_decoders", (PyCFunction)pg_mixer_get_decoders, METH_NOARGS, "TODO"},
    {NULL, NULL, 0, NULL}};

// ***************************************************************************
// MODULE SETUP
// ***************************************************************************

int
exec_mixer(PyObject *module)
{
    printf("in exec mixer\n");

    /*imported needed apis*/
    import_pygame_base();
    if (PyErr_Occurred()) {
        return -1;
    }
    import_pygame_rwobject();
    if (PyErr_Occurred()) {
        return -1;
    }

    PyObject *mixer_type = PyType_FromModuleAndSpec(module, &mixer_spec, NULL);
    if (PyModule_AddObjectRef(module, "Mixer", mixer_type) < 0) {
        return -1;
    }

    PyObject *audio_type = PyType_FromModuleAndSpec(module, &audio_spec, NULL);
    if (PyModule_AddObjectRef(module, "Audio", audio_type) < 0) {
        return -1;
    }

    PyObject_SetAttrString(mixer_type, "_audio_type", audio_type);

    _mixer_state *state = GET_STATE(module);
    state->mixer_initialized = false;

    return 0;
}

MODINIT_DEFINE(_mixer)
{
    static PyModuleDef_Slot mixer_slots[] = {
        {Py_mod_exec, &exec_mixer},
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
                                         "surface",
                                         "DOC TODO",
                                         sizeof(_mixer_state),
                                         _mixer_methods,
                                         mixer_slots,
                                         NULL,
                                         NULL,
                                         NULL};

    return PyModuleDef_Init(&_module);
}
