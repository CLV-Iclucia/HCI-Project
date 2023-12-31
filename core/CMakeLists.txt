file(GLOB_RECURSE srcs CONFIGURE_DEPENDS src/*.cc src/*.cpp include/*.h)

add_executable(game apps/game.cc)
target_include_directories(game PUBLIC ${HCI_EXTERNAL}/glm)
target_link_libraries(game PUBLIC glfw ogl-render TBB::tbb)