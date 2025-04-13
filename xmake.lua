add_rules("mode.debug", "mode.release")

-- Add dependencies globally
add_requires("imgui", {configs = {glfw = true, opengl3 = true}})
add_requires("glfw", {configs = {shared = false}})
add_requires("opengl")
add_requires("glad")

-- Define the target
target("ImGuiExample")
    set_kind("binary")
    set_languages("cxx17")

    -- Add source files
    add_files("main.cpp")

    -- Add packages
    add_packages("imgui", "glfw", "opengl", "glad")

    -- Link libraries
    add_links("opengl32")