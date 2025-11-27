import pygame.base
import pygame._base_audio as _base_audio


init = _base_audio.init
quit = _base_audio.quit
get_current_driver = _base_audio.get_current_driver
get_drivers = _base_audio.get_drivers
get_playback_devices = _base_audio.get_playback_devices
get_recording_devices = _base_audio.get_recording_devices


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
        raise NotImplementedError("SDL_GetSilenceValueForFormat")

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

        self.format = format
        self.channels = channels
        self.frequency = frequency

    @property
    def framesize(self) -> int:
        return self.format.bytesize * self.channels

    def __repr__(self) -> str:
        return (
            f"{self.__class__.__module__}.{self.__class__.__name__}"
            f"({self.format}. {self.channels}, {self.frequency})"
        )
