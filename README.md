# Rooster Raytracer
#### v0.1
This is a raytracer that right not only renders spheres without any shadow (they appear as circles)
**design decisions:**
- i used a functional approach to not use naked loops everywhere
- make the code decouple from the actual data from the scene (make a super simple text parser to get the objects)
**how to use it:**
- run `java src/App.java sample.rscn output.png` from the project's root
- open output.png to see the result
- change sample.rscn to put new things to the scene
- you can load triangle meshes from OBJ files with `obj path/to/model.obj` in the scene file
- flat shading uses scene lights, define at least one with `dir_light dx dy dz r g b intensity`
- you can define palette materials with `mat r g b` and reference one from OBJ as `obj model.obj ox oy oz materialId`
