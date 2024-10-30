# Voxel Raytracer
- Features fully destructable world and realistic light and shadows.
- Renders voxels stored in a SSBO via fragment shader.
- Each voxel is stored in just 4 bytes with 24-bit color and 7 unused bits (for future expansion).
- Supports collision detection and player/entity gravity.
- **This project is primarily developed and tested on AMD GPUs, some Nvidia specific issues have been observed and fixed as best as possible.**

### Known Issue(s)
- *Shader compiling with large SSBO's takes awhile on Nvidia GPUs. If you have an Nvidia GPU it may take awhile to start the first time, potentially stating "not responding" please do not end the task.*
