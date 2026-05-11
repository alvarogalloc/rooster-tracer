add_rules("mode.debug", "mode.release")
add_requires("stb", "glm")

target("cpp-raytracer")
    set_kind("binary")
    set_languages("c++latest")
    add_files("src/*.cpp")
    add_files("src/*.cppm")
    add_packages("stb", "glm")
    if is_mode("release") then
        set_symbols("hidden")
        add_cxflags(
            "-march=native",              -- full host ISA (AVX2/AVX-512 if available)
            "-funroll-loops",
            "-fvectorize",
            "-fslp-vectorize",            -- superword-level parallelism
            "-flto=thin",                 -- ThinLTO: fast link + cross-TU inlining
            "-fomit-frame-pointer",
            "-fno-rtti"
        )
        add_ldflags("-flto=thin", { force = true })
    end

