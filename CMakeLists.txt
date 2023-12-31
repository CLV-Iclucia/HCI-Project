cmake_minimum_required(VERSION 3.23)
project(HCI VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 20)
set(HCI_EXTERNAL "${PROJECT_SOURCE_DIR}/external")
set(GLM_PATH ${HCI_EXTERNAL}/glm)

find_package(TBB REQUIRED)
add_subdirectory(${HCI_EXTERNAL}/glfw)
add_subdirectory(${HCI_EXTERNAL}/glad)
add_subdirectory(OglRender)
add_subdirectory(core)