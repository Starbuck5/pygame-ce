from dataclasses import dataclass

from pygame import _mixer


@dataclass
class _MixerState:
    init: bool = False
    mixer = None

    def acquire_mixer(self) -> _mixer.Mixer:
        if self.mixer is None:
            self.mixer = _mixer.Mixer()
        return self.mixer


_mixer_state = _MixerState()


def init(*args, **kwargs):
    _mixer.init()
    _mixer_state.init = True


def pre_init(*args, **kwargs):
    pass


def get_init():
    return _mixer_state


class Sound:
    def __init__(self, file):
        self.mixer = _mixer_state.acquire_mixer()
        self.audio = _mixer.Audio(file, True, self.mixer)

    def play(self):
        self.mixer.play_audio(self.audio)


class Channel:
    pass


class _MusicImplementation:
    def __init__(self):
        self.mixer = None
        self.track = None

    def load(self, file):
        self.mixer = _mixer_state.acquire_mixer()
        self.track = _mixer.Track(self.mixer)
        audio = _mixer.Audio(file, False, self.mixer)
        self.track.set_audio(audio)

    def play(self, loops=0, start=0.0, fade_ms=0):
        self.track.play(loops=loops, start_ms=round(start * 1000), fadein_ms=fade_ms)

    def fadeout(self, time, /):
        pass


music = _MusicImplementation()
