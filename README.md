# Voxel Raytracer
- Features fully destructable world and realistic light and shadows.
- Supports both local and global light sources.
- Renders voxels stored in a SSBO via fragment shader.
- Each voxel is stored in just 4 bytes with 24-bit color and 7 unused bits (for future expansion).
- Supports collision detection and player/entity gravity.
- **This project is primarily developed and tested on AMD GPUs, some Nvidia specific issues have been observed and fixed as best as possible.**

## Controls

- `W` `A` `S` `D` to move
- `Space` to jump
- `Left Click` to enable camera movement
- `Right Click` to destroy blocks
- `Shift` to toggle depth field view
- `T` to place local light (limit 16)

### Known Issue(s)

- When digging to the bottom of the map, jumping may not work. This is due to no solid blocks being beneath the players feet (the player is essentially hovering on the bottom map boundry), to get around this you must walk onto solid ground.

- *Shader compiling with large SSBO's takes awhile on Nvidia GPUs. If you have an Nvidia GPU it may take awhile to start the first time, potentially stating "not responding" please do not end the task.*
