import pygame.core

for name, value in vars(pygame.core.color).items():
    globals()[name] = value
