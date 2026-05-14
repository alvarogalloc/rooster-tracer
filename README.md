# Rooster Raytracer
#### v0.1
This is a raytracer that renders spheres, planes and triangles with phong shading and hard shadows.
**design decisions:**
- i used a functional approach to not use naked loops everywhere
- make the code decouple from the actual data from the scene (make a super simple text parser to get the objects)
**how to use it:**
- run `java src/App.java sample.rscn output.png` from the project's root
- open output.png to see the result
- change sample.rscn to put new things to the scene
- you can load triangle meshes from OBJ files with `obj path/to/model.obj` in the scene file
- phong shading uses all scene lights, define at least one with `dir_light dx dy dz r g b intensity` or `point_light px py pz r g b intensity`
- you can define materials with `mat r g b` and reference one from OBJ as `obj model.obj ox oy oz materialId`
- spheres/triangles create inline phong materials from their RGB values, and planes use `plane px py pz nx ny nz materialId`
- java render tests are organized as scenes under `test-scenes/` and expected outputs under `test-imgs/`
- regenerate all java test outputs with `java src/RenderTestScenes.java`
