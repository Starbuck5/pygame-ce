import math

import pygame
from pygame import _audio, _sdl3_mixer
from pygame.typing import FileLike

"""
Levels of initialization

- SDL and SDL mixer initialized
- Music/mixer compat pgce side resources created
    - Tracks/Audio/structures potentially
    - Mixer opened to default audio device


compat_indicated = False


mixer.pre_init()
    compat_indicated = True

mixer.init()
    Call SDL init
    if compat_indicated:
        mixer_compat_setup()

mixer.Sound()
mixer.Channel()
mixer.music.*()
    Check SDL init
    if not mixer compat setup:
        mixer_compat_setup()
"""


class MixerInternals:
    default_frequency = 44100
    default_size = -16
    default_channels = 2
    default_chunksize = 512
    default_allowedchanges = 5

    request_frequency = default_frequency
    request_size = default_size
    request_channels = default_channels
    request_chunksize = default_chunksize
    request_allowedchanges = default_allowedchanges
    request_device = None

    allow_channels_change = 0x4

    initialized = False
    mixer: _sdl3_mixer.Mixer | None = None
    channels: list["Channel"] = []
    reserved_channels = 0
    soundfount: str | None = None


def init(
    frequency: int = 0,
    size: int = 0,
    channels: int = 0,
    buffer: int = 0,
    devicename: str | None = None,
    allowedchanges: int = -1,
) -> None:
    if MixerInternals.initialized:
        return

    if frequency == 0:
        frequency = MixerInternals.request_frequency
    if size == 0:
        size = MixerInternals.request_size
    if allowedchanges == -1:
        allowedchanges = MixerInternals.request_allowedchanges
    if channels == 0:
        channels = MixerInternals.request_channels

    if allowedchanges & MixerInternals.allow_channels_change:
        if channels <= 1:
            channels = 1
        elif channels <= 3:
            channels = 2
        elif channels <= 5:
            channels = 4
        else:
            channels = 6
    else:
        if channels not in (1, 2, 4, 6):
            raise pygame.error("'channels' must be in 1, 2, 4, or 6")

    chunk = buffer
    if chunk == 0:
        chunk = MixerInternals.request_chunksize

    if devicename is None:
        devicename = MixerInternals.request_device

    if size == 8:
        fmt = _audio.U8
    elif size == -8:
        fmt = _audio.S8
    # 16 -> U16 not available in SDL3
    elif size == -16:
        fmt = _audio.S16
    elif size == 32:
        fmt = _audio.F32
    else:
        raise ValueError(f"Unsupported size {size}")

    # Make chunk a power of 2
    chunk = max(1 << (chunk - 1).bit_length(), 256)

    # TODO: driver envs

    _sdl3_mixer.init()

    mixer_spec = _audio.AudioSpec(fmt, channels, frequency)

    mixer_device = _audio.DEFAULT_PLAYBACK_DEVICE
    if devicename is not None:
        potential_devices = [
            device
            for device in _audio.get_playback_devices()
            if device.name == devicename
        ]
        if potential_devices:
            mixer_device = potential_devices[0]

    MixerInternals.initialized = True
    MixerInternals.mixer = _sdl3_mixer.Mixer(mixer_device, mixer_spec)
    MixerInternals.channels = [Channel(i) for i in range(8)]
    MixerInternals.reserved_channels = 0

    music._init()


def pre_init(
    frequency: int = 0,
    size: int = 0,
    channels: int = 0,
    buffer: int = 0,
    devicename: str | None = None,
    allowedchanges: int = -1,
) -> None:
    if frequency != 0:
        MixerInternals.request_frequency = frequency
    else:
        MixerInternals.request_frequency = MixerInternals.default_frequency

    if size != 0:
        MixerInternals.request_size = size
    else:
        MixerInternals.request_size = MixerInternals.default_size

    if channels != 0:
        MixerInternals.request_channels = channels
    else:
        MixerInternals.request_channels = MixerInternals.default_channels

    if buffer != 0:
        MixerInternals.request_chunksize = buffer
    else:
        MixerInternals.request_chunksize = MixerInternals.default_chunksize

    MixerInternals.request_device = devicename

    if allowedchanges != -1:
        MixerInternals.request_allowedchanges = allowedchanges
    else:
        MixerInternals.request_allowedchanges = MixerInternals.default_allowedchanges


def quit() -> None:
    MixerInternals.initialized = False
    MixerInternals.mixer = None
    MixerInternals.channels = []


def get_init() -> tuple[int, int, int]:
    if not MixerInternals.initialized:
        return None

    mix_spec = MixerInternals.mixer.spec
    realform = (
        -mix_spec.format.bitsize
        if mix_spec.format.is_signed
        else mix_spec.format.bitsize
    )

    return (mix_spec.frequency, realform, mix_spec.channels)


def get_driver() -> str:
    return _audio.get_current_driver()


def set_num_channels(count: int, /) -> None:
    if not MixerInternals.initialized:
        raise pygame.error("mixer not initialized")

    # TODO: lock
    channels = MixerInternals.channels

    if len(channels) > count:
        channels = channels[:count]
    else:
        channels += [Channel(i + count) for i in range(count)]


def get_num_channels() -> int:
    if not MixerInternals.initialized:
        raise pygame.error("mixer not initialized")

    return len(MixerInternals.channels)


def set_reserved(count: int, /) -> int:
    if not MixerInternals.initialized:
        raise pygame.error("mixer not initialized")

    if count < 0:
        count = 0

    num_channels = get_num_channels()
    if count > num_channels:
        count = num_channels

    MixerInternals.reserved_channels = count
    return count


def set_soundfont(paths: str | None = None, /) -> None:
    # TODO: thread this through SDL_mixer.decoder.fluidsynth.soundfont_path
    # on load
    MixerInternals.soundfount = paths


def get_soundfont() -> str | None:
    return MixerInternals.soundfount


get_sdl_mixer_version = _sdl3_mixer.get_sdl_mixer_version


class Sound:
    def __init__(self, file) -> None:
        if MixerInternals.mixer is None:
            raise pygame.error("mixer not initialized")

        self._audio = _sdl3_mixer.Audio(
            file, predecode=True, preferred_mixer=MixerInternals.mixer
        )
        self._tag = str(id(self))
        self._volume = 1.0

    def play(
        self,
        loops: int = 0,
        maxtime: int = -1,
        fade_ms: int = -1,
    ) -> "Channel":
        selected_channel: Channel | None = None

        # TODO lock
        for channel in MixerInternals.channels[MixerInternals.reserved_channels :]:
            if not channel.get_busy():
                selected_channel = channel
                break

        if selected_channel is None:
            return None

        selected_channel.play(self, loops, maxtime, fade_ms)
        return selected_channel

    def stop(self) -> None:
        MixerInternals.mixer.stop_tag(self._tag)

    def fadeout(self, time: int, /) -> None:
        MixerInternals.mixer.stop_tag(self._tag, time)

    def set_volume(self, value: float, /) -> None:
        self._volume = value
        MixerInternals.mixer.set_tag_gain(self._tag, value)

    def get_volume(self) -> float:
        return self._volume

    def get_length(self) -> float:
        return float(self._audio.duration_ms) / 1000


class Channel:
    def __init__(self, id: int) -> None:
        if not MixerInternals.initialized:
            raise pygame.error("mixer not initialized")

        self._id = id
        self._track = _sdl3_mixer.Track(MixerInternals.mixer)
        self._sound = None

    @property
    def id(self) -> int:
        return self._id

    def play(
        self,
        sound: Sound,
        loops: int = 0,
        maxtime: int = -1,
        fade_ms: int = -1,
    ) -> None:
        if self._sound is not None:
            self._track.remove_tag(self._sound._tag)

        self._sound = sound
        self._track.set_audio(sound._audio)
        self._track.add_tag(sound._tag)
        self._track.play(loops=loops, max_ms=maxtime, fadein_ms=fade_ms)

    def stop(self) -> None:
        self._track.stop()

    def pause(self) -> None:
        self._track.pause()

    def unpause(self) -> None:
        self._track.resume()

    def fadeout(self, time: int, /) -> None:
        self._track.stop(fade_out_frames=self.track.ms_to_frames(time))

    def set_source_location(self, angle: float, distance: float, /) -> None:
        x = math.sin(angle) * distance
        z = math.cos(angle) * distance
        self._track.set_3d_position((x, 0, z))

    def set_volume(self, arg1: float, arg2: float | None = -1.0, /):
        if arg2 == -1.0:
            self._track.gain = arg1
            return

        if arg2 == None:
            arg2 = arg1

        self._track.set_stereo(arg1, arg2)

    def get_volume(self) -> float:
        return self._track.gain

    def get_busy(self) -> bool:
        return self._track.playing

    def get_sound(self) -> Sound | None:
        return self._sound


SoundType = Sound
ChannelType = Channel


class MusicImplementation:
    def __init__(self) -> None:
        self._audio: _sdl3_mixer.Audio | None = None
        self._track: _sdl3_mixer.Track | None = None

        self._queued_audio: _sdl3_mixer.Audio | None = None
        self._queued_loops = 0

        self._end_event = 0

    def _init(self) -> None:
        self._track = _sdl3_mixer.Track(MixerInternals.mixer)
        self._track.set_stopped_callback(self._stopped_callback)

    def _stopped_callback(self, _: _sdl3_mixer.Track, __: None) -> None:
        print("Callback")
        return

        if self._end_event and pygame.display.get_init():
            pygame.event.post(pygame.Event(0, {}))

        self._audio = self._queued_audio
        self._queued_audio = None
        self._queued_loops = 0

        self._track.set_audio(self._audio)
        self._track.play(loops=self._queued_loops)

    def load(self, filename, namehint: str = "") -> None:
        if self._track is None:
            raise pygame.error("mixer not initialized")

        self._audio = _sdl3_mixer.Audio(
            filename, predecode=False, preferred_mixer=MixerInternals.mixer
        )
        self._track.set_audio(self._audio)

    def unload(self) -> None:
        if self._track is None:
            raise pygame.error("mixer not initialized")

        self._audio = None
        self._queued_audio = None
        self._queued_loops = 0
        self._track.set_audio(None)

    def play(self, loops: int = 0, start: float = 0.0, fade_ms: int = 0) -> None:
        if self._track is None:
            raise pygame.error("mixer not initialized")

        self._track.play(loops=loops, start_ms=round(start*1000), fadein_ms=round(fade_ms))

    def rewind(self) -> None:
        if self._track is None:
            raise pygame.error("mixer not initialized")

        self._track.play()

    def stop(self) -> None:
        if self._track is None:
            raise pygame.error("mixer not initialized")

        self._track.stop()

    def pause(self) -> None:
        if self._track is None:
            raise pygame.error("mixer not initialized")

        self._track.pause()

    def unpause(self) -> None:
        if self._track is None:
            raise pygame.error("mixer not initialized")

        self._track.resume()

    def fadeout(self, time: int, /) -> None:
        if self._track is None:
            raise pygame.error("mixer not initialized")

        self._track.stop(fade_out_frames=self._track.ms_to_frames(round(time)))

    def set_volume(self, volume: float, /) -> None:
        if self._track is None:
            raise pygame.error("mixer not initialized")

        self._track.gain = volume

    def get_volume(self) -> float:
        if self._track is None:
            raise pygame.error("mixer not initialized")

        return self._track.gain

    def get_busy(self) -> bool:
        if self._track is None:
            raise pygame.error("mixer not initialized")

        return self._track.playing

    def set_pos(self, pos: float, /) -> None:
        if self._track is None:
            raise pygame.error("mixer not initialized")

        self._track.set_playback_position(self._track.ms_to_frames(round(pos)))

    def get_pos(self) -> int:
        if self._track is None:
            raise pygame.error("mixer not initialized")

        if self._audio is None:
            return -1

        return self._track.frames_to_ms(self._track.get_playback_position())

    def queue(self, filename: FileLike, namehint: str = "", loops: int = 0) -> None:
        if self._track is None:
            raise pygame.error("mixer not initialized")

        self._queued_audio = _sdl3_mixer.Audio(
            filename, predecode=False, preferred_mixer=MixerInternals.mixer
        )
        self._queued_loops = loops

    def set_endevent(self, event_type: int, /) -> None:
        self._end_event = event_type
    
    def get_endevent(self) -> int:
        return self._end_event

    def get_metadata(
        self, filename: FileLike | None = None, namehint: str = ""
    ) -> dict:
        if self._track is None:
            raise pygame.error("mixer not initialized")

        if filename is None:
            audio = self._audio
            if audio is None:
                raise pygame.error("music not loaded")
        else:
            audio = _sdl3_mixer.Audio(filename)

        metadata = audio.get_metadata()

        return {
            "title": metadata.title or "",
            "album": metadata.album or "",
            "artist": metadata.artist or "",
            "copyright": metadata.copyright or "",
        }


music = MusicImplementation()
