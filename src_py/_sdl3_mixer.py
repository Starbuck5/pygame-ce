import dataclasses

from pygame import _sdl3_mixer_c

init = _sdl3_mixer_c.init
quit = _sdl3_mixer_c.quit
get_sdl_mixer_version = _sdl3_mixer_c.get_sdl_mixer_version


# Pure Python version of MIX_MSToFrames, since it's so straightforward.
def ms_to_frames(sample_rate: int, ms: int) -> int:
    if sample_rate <= 0:
        raise ValueError("Sample rate must be greater than zero.")
    if ms < 0:
        raise ValueError("MS must be positive.")

    return ms / 1000 * sample_rate


# Pure Python version of MIX_FramesToMS, since it's so straightforward.
def frames_to_ms(sample_rate: int, frames: int) -> int:
    if sample_rate <= 0:
        raise ValueError("Sample rate must be greater than zero.")
    if frames < 0:
        raise ValueError("Frames must be positive.")

    return frames / sample_rate * 1000


get_decoders = _sdl3_mixer_c.get_decoders


class Mixer(_sdl3_mixer_c.Mixer):
    pass


@dataclasses.dataclass(frozen=True)
class AudioMetadata:
    title: str | None
    artist: str | None
    album: str | None
    copyright: str | None
    track: int | None
    total_tracks: int | None


class Audio(_sdl3_mixer_c.Audio):
    def get_metadata(self) -> AudioMetadata:
        metadata = _sdl3_mixer_c.Audio.get_metadata(self)
        return AudioMetadata(
            title=metadata["title"],
            artist=metadata["artist"],
            album=metadata["album"],
            copyright=metadata["copyright"],
            track=metadata["track"],
            total_tracks=metadata["total_tracks"],
        )


class Track(_sdl3_mixer_c.Track):
    pass
