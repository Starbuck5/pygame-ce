from dataclasses import dataclass
from typing import Optional

from pygame.base import (
    _system_get_cpu_instruction_sets as get_cpu_instruction_sets,
    _system_get_total_ram as get_total_ram,
    _system_get_pref_path as get_pref_path,
    _system_get_pref_locales as get_pref_locales,
    _system_get_power_state
)

@dataclass(frozen=True)
class PowerState:
    battery_percent: Optional[int]
    battery_seconds: Optional[int]
    on_battery: bool
    no_battery: bool
    charging: bool
    charged: bool
    plugged_in: bool
    has_battery: bool

def get_power_state() -> PowerState:
    return PowerState(**_system_get_power_state())
