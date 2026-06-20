# Implementation systems

These files split the original monolithic OpenGL source by responsibility:

- `audio_system.inl`: MCI music, interactions, and footsteps.
- `texture_system.inl`: image lookup, embedded textures, materials, and texture parameters.
- `navigation_system.inl`: walk areas, terrain height, collision tests, and wall attachment.
- `geometry_system.inl`: reusable OpenGL primitive mesh creation.
- `model_animation_system.inl`: Assimp meshes, Blender marker nodes, bones, and animation playback.

They intentionally remain one C++ translation unit through includes at the end
of `opengl.cpp`. This preserves the existing runtime state and avoids subtle
initialization/order regressions while still keeping each system independently
discoverable and maintainable.
