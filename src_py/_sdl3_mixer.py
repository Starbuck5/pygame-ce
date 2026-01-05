from pygame import _sdl3_mixer_c

init = _sdl3_mixer_c.init
quit = _sdl3_mixer_c.quit
get_sdl_mixer_version = _sdl3_mixer_c.get_sdl_mixer_version

get_decoders = _sdl3_mixer_c.get_decoders

class Mixer(_sdl3_mixer_c.Mixer):
    pass

class Audio(_sdl3_mixer_c.Audio):
    pass

class Track(_sdl3_mixer_c.Track):
    pass
