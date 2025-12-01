from typing_extensions import Buffer
import pygame.base
import pygame._base_audio as _base_audio


class AudioFormat:
    # AudioFormat details pulled from SDL_audio.h header files
    # These details are stable for the lifetime of SDL3, as programs built
    # on one release will be able to run on newer releases.
    _MASK_BITSIZE = 0xFF
    _MASK_FLOAT = 1 << 8
    _MASK_BIG_ENDIAN = 1 << 12
    _MASK_SIGNED = 1 << 15

    def __init__(self, name: str, value: int) -> None:
        self._name = f"pygame.audio.{name}"
        self._value = value

    @property
    def bitsize(self) -> int:
        return self._value & AudioFormat._MASK_BITSIZE

    @property
    def bytesize(self) -> int:
        return self.bitsize // 8

    @property
    def is_float(self) -> bool:
        return bool(self._value & AudioFormat._MASK_FLOAT)

    @property
    def is_int(self) -> bool:
        return not self.is_float

    @property
    def is_big_endian(self) -> bool:
        return bool(self._value & AudioFormat._MASK_BIG_ENDIAN)

    @property
    def is_little_endian(self) -> bool:
        return not self.is_big_endian

    @property
    def is_signed(self) -> bool:
        return bool(self._value & AudioFormat._MASK_SIGNED)

    @property
    def is_unsigned(self) -> bool:
        return not self.is_signed

    @property
    def silence_value(self) -> int:
        return _base_audio.get_silence_value_for_format(self._value)

    def __index__(self) -> int:
        """Returns the actual constant value needed for calls to SDL"""
        return self._value

    def __repr__(self) -> str:
        return self._name


UNKNOWN = AudioFormat("UNKNOWN", 0x0000)
U8 = AudioFormat("U8", 0x0008)
S8 = AudioFormat("S8", 0x8008)
S16LE = AudioFormat("S16LE", 0x8010)
S16BE = AudioFormat("S16BE", 0x9010)
S32LE = AudioFormat("S32LE", 0x8020)
S32BE = AudioFormat("S32BE", 0x9020)
F32LE = AudioFormat("F32LE", 0x8120)
F32BE = AudioFormat("F32BE", 0x9120)

if pygame.base.get_sdl_byteorder() == 1234:
    S16 = S16LE
    S32 = S32LE
    F32 = F32LE
else:
    S16 = S16BE
    S32 = S32BE
    F32 = F32BE


class AudioSpec:
    def __init__(self, format: AudioFormat, channels: int, frequency: int) -> None:
        if not isinstance(format, AudioFormat):
            raise TypeError(
                f"AudioSpec format must be an AudioFormat, received {type(format)}"
            )

        if channels < 1 or channels > 8:
            raise ValueError("Invalid channel count, should be between 1 and 8.")

        # AudioSpecs are immutable so that they can be owned by other things
        # like AudioStreams without worrying about what happens if someone
        # changes the spec externally.
        self._format = format
        self._channels = channels
        self._frequency = frequency

    @property
    def format(self) -> AudioFormat:
        return self._format

    @property
    def channels(self) -> int:
        return self._channels

    @property
    def frequency(self) -> int:
        return self._frequency

    @property
    def framesize(self) -> int:
        return self._format.bytesize * self.channels

    def __repr__(self) -> str:
        return (
            self.__class__.__name__
            + f"({self._format}, {self._channels}, {self._frequency})"
        )


class AudioDevice:
    # Will this just make it show up in the recommendations??
    # def pause(self) -> None:
    #    raise NotImplementedError("Only LogicalAudioDevices can be paused")

    # def __repr__(self) -> str:
    #    return f"AudioDevice with name {self.name}"

    def open(self, spec: AudioSpec | None = None) -> "LogicalAudioDevice":
        if spec is None:
            dev_state = _base_audio.open_audio_device(self._state)
        elif isinstance(spec, AudioSpec):
            dev_state = _base_audio.open_audio_device(
                self._state, spec.format, spec.channels, spec.frequency
            )
        else:
            raise TypeError(
                f"AudioDevice open 'spec' argument must be an AudioSpec or None, received {type(spec)}"
            )

        device = object.__new__(LogicalAudioDevice)
        device._state = dev_state
        return device

    def bind(self, *args: "AudioStream") -> None:
        for stream in args:
            if not isinstance(stream, AudioStream):
                raise TypeError(
                    f"Bind arguments must be AudioStreams, received {type(stream)}"
                )

            _base_audio.bind_audio_stream(self._state, stream._state)

    @property
    def name(self) -> str:
        return _base_audio.get_audio_device_name(self._state)


class LogicalAudioDevice(AudioDevice):
    def pause(self) -> None:
        _base_audio.pause_audio_device(self._state)

    def resume(self) -> None:
        _base_audio.resume_audio_device(self._state)


class AudioStream:
    def __init__(self, src_spec: AudioSpec, dst_spec: AudioSpec) -> None:
        if not isinstance(src_spec, AudioSpec):
            raise TypeError(
                f"AudioStream src_spec must be an AudioSpec, received {type(src_spec)}"
            )
        if not isinstance(dst_spec, AudioSpec):
            raise TypeError(
                f"AudioStream dst_spec must be an AudioSpec, received {type(dst_spec)}"
            )

        self._src_spec = src_spec
        self._dst_spec = dst_spec

        self._state = _base_audio.create_audio_stream(
            src_spec.format,
            src_spec.channels,
            src_spec.frequency,
            dst_spec.format,
            dst_spec.channels,
            dst_spec.frequency,
        )

    def clear(self) -> None:
        _base_audio.clear_audio_stream(self._state)

    def flush(self) -> None:
        _base_audio.flush_audio_stream(self._state)

    @property
    def num_available_bytes(self) -> int:
        return _base_audio.get_audio_stream_available(self._state)

    @property
    def num_queued_bytes(self) -> int:
        return _base_audio.get_audio_stream_queued(self._state)

    def put_data(self, data: Buffer) -> None:
        _base_audio.put_audio_stream_data(self._state, data)

    def get_data(self, size: int) -> bytes:
        return _base_audio.get_audio_stream_data(self._state, size)

    @property
    def src_spec(self) -> AudioSpec:
        return self._src_spec

    @property
    def dst_spec(self) -> AudioSpec:
        return self._dst_spec

    @property
    def gain(self) -> float:
        return _base_audio.get_audio_stream_gain(self._state)

    @gain.setter
    def gain(self, value: float) -> None:
        _base_audio.set_audio_stream_gain(self._state, value)

    @property
    def frequency_ratio(self) -> float:
        return _base_audio.get_audio_stream_frequency_ratio(self._state)

    @frequency_ratio.setter
    def frequency_ratio(self, value: float) -> None:
        _base_audio.set_audio_stream_frequency_ratio(self._state, value)

    def __repr__(self) -> str:
        return f"<{self.__class__.__name__}({self._src_spec}, {self._dst_spec})>"


# Dependency inject classes into the module
_base_audio.AudioDevice = LogicalAudioDevice
_base_audio.LogicalAudioDevice = LogicalAudioDevice


init = _base_audio.init
quit = _base_audio.quit
get_current_driver = _base_audio.get_current_driver
get_drivers = _base_audio.get_drivers


# UGH, AudioDevices should probably be singletons based on the device id. ?
# TODO: deal with that.


def get_playback_devices() -> list[AudioDevice]:
    output = []

    for dev_state in _base_audio.get_playback_device_states():
        device = object.__new__(AudioDevice)
        device._state = dev_state
        output.append(device)

    return output


def get_recording_devices() -> list[AudioDevice]:
    output = []

    for dev_state in _base_audio.get_recording_device_states():
        device = object.__new__(AudioDevice)
        device._state = dev_state
        output.append(device)

    return output
