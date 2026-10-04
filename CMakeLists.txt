cmake_minimum_required(VERSION 3.15)
project(Jarvis VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

include(FetchContent)

# Fetch nlohmann/json
FetchContent_Declare(
    json
    URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
)
FetchContent_MakeAvailable(json)

# Fetch libcurl
FetchContent_Declare(
    curl
    GIT_REPOSITORY https://github.com/curl/curl.git
    GIT_TAG curl-8_7_1
)
set(BUILD_CURL_EXE OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(HTTP_ONLY ON CACHE BOOL "" FORCE)
set(CURL_USE_OPENSSL OFF CACHE BOOL "" FORCE)
set(CURL_USE_SCHANNEL ON CACHE BOOL "" FORCE)
set(CURL_DISABLE_TESTS ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(curl)

# Fetch imgui
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.89.9
)
FetchContent_MakeAvailable(imgui)

# Create an imgui library object
add_library(imgui_lib STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_win32.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_dx11.cpp
)
target_include_directories(imgui_lib PUBLIC ${imgui_SOURCE_DIR} ${imgui_SOURCE_DIR}/backends)

# Notice we use WIN32 in add_executable so it hides the console window
add_executable(Jarvis WIN32
    src/main.cpp
    src/AudioControl.cpp
    src/LLMController.cpp
    src/SystemActions.cpp
    src/ConfigManager.cpp
)

target_include_directories(Jarvis PRIVATE include)

target_link_libraries(Jarvis PRIVATE
    nlohmann_json::nlohmann_json
    libcurl
    imgui_lib
    sapi.lib
    ole32.lib
    winmm.lib
    d3d11.lib
    d3dcompiler.lib
    shlwapi.lib
    shell32.lib
    uuid.lib
    advapi32.lib dwmapi.lib
)

target_compile_definitions(Jarvis PRIVATE CURL_STATICLIB)
