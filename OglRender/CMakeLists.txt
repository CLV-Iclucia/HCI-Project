project(OglRender LANGUAGES CXX)
file(GLOB_RECURSE srcs CONFIGURE_DEPENDS src/*.cpp include/*.h src/*.cc)

add_library(ogl-render STATIC ${srcs})
target_link_libraries(ogl-render PUBLIC GLAD glfw)
target_include_directories(ogl-render PUBLIC include ${GLM_PATH})