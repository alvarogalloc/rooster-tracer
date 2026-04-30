add_rules("mode.debug", "mode.release")
add_requires("stb", "glm")

target("cpp-raytracer")
    set_kind("binary")
    set_languages("c++latest")
    add_files("src/*.cpp")
    add_files("src/*.cppm")
    add_packages("stb", "glm")

