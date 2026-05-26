---@diagnostic disable: undefined-global
add_rules("mode.debug", "mode.release")
add_requires("stb", "glm")

target("cpp-raytracer")
    -- set_policy("build.optimization.lto", true)
    set_kind("binary")
    set_languages("c++latest")
    
    add_files("src/*.cpp")
    add_files("src/*.cppm")
    add_packages("stb", "glm")
    
    if is_mode("release") then
        set_symbols("hidden")
        add_cxflags(
            "-march=native", -- full host ISA (AVX2/AVX-512 if available)
            "-fno-rtti"
        )
    end

    if is_plat("windows", "mingw") then
        add_ldflags("-Wl,--allow-multiple-definition", "-lstdc++exp", {force = true})
    end
