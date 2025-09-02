from dataclasses import dataclass
from typing import Union, List

import pygame
from pygame import _mixer
from pygame import Event


@dataclass
class _MixerState:
    mixer: _mixer.Mixer
    channel_tracks: List[_mixer.Track]

    def __init__(self):
        self.mixer = _mixer.Mixer()
        self.channel_tracks = []
        for _ in range(8):
            track = _mixer.Track(self.mixer)
            track.add_tag("_sound")
            self.channel_tracks.append(track)



_mixer_state: Union[_MixerState, None] = None


def init(*args, **kwargs):
    global _mixer_state

    _mixer.init()
    _mixer_state = _MixerState()


def pre_init(*args, **kwargs):
    pass


def quit():
    global _mixer_state

    # KNOWN ISSUE: quitting SDL3_mixer causes segfaults when deallocating Mixer objects (probably others too)
    #_mixer.quit()
    _mixer_state = None


def get_init():
    return _mixer_state is not None


get_sdl_mixer_version = _mixer.get_sdl_mixer_version


class Sound:
    def __init__(self, file):
        if _mixer_state is None:
            raise pygame.error("mixer not initialized")
        self._mixer = _mixer_state.mixer
        self._audio = _mixer.Audio(file, True, self._mixer)

    def play(self):
        self._mixer.play_audio(self._audio)


class Channel:
    def __init__(self, id: int) -> None:
        if _mixer_state is None:
            raise pygame.error("mixer not initialized")
        self._track = _mixer_state.channel_tracks[id]
        self._id = id

    @property
    def id(self) -> int:
        return self._id
    
    def play(
        self,
        sound: Sound,
        loops: int = 0,
        maxtime: int = 0,
        fade_ms: int = 0,
    ) -> None:
        self._track.set_audio(sound._audio)
        self._track.play(loops=loops, max_ms=maxtime, fadein_ms=fade_ms)

    def stop(self) -> None:
        self._track.stop()

    def pause(self) -> None:
        self._track.pause()

    def unpause(self) -> None:
        self._track.resume()

    def fadeout(self, time: int, /) -> None:
        self._track.stop(self._track.ms_to_frames(time))

    def queue(self, sound: Sound, /) -> None: ...
    def set_source_location(self, angle: float, distance: float, /) -> None: ...
    #@overload
    #def set_volume(self, value: float, /) -> None: ...
    #@overload
    def set_volume(self, left: float, right: float, /) -> None: ...
    def get_volume(self) -> float: ...
    def get_busy(self) -> bool: ...
    def get_sound(self) -> Sound: ...
    def get_queue(self) -> Sound: ...
    def set_endevent(self, type: Union[int, Event] = 0, /) -> None: ...
    def get_endevent(self) -> int: ...   




class _MusicImplementation:
    def __init__(self):
        self.mixer = None
        self.track = None

    def load(self, file):
        if _mixer_state is None:
            raise pygame.error("mixer not initialized")
        self.mixer = _mixer_state.mixer
        self.track = _mixer.Track(self.mixer)
        audio = _mixer.Audio(file, False, self.mixer)
        self.track.set_audio(audio)

    def play(self, loops=0, start=0.0, fade_ms=0):
        self.track.play(loops=loops, start_ms=round(start * 1000), fadein_ms=fade_ms)

    def fadeout(self, time, /):
        pass


music = _MusicImplementation()
