cmake_minimum_required(VERSION 3.14)
project(OpenterfaceCore LANGUAGES C CXX VERSION 0.1.0)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# ── output directories ──────────────────────────────────────────────────
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# ── unified Core SDK pure/common layers ──────────────────────────────────
set(OPENTERFACE_CORE_COMMON_SOURCES
    src/core/version.c
    src/capability/capability.c
    src/device/device_info.c
    src/profile/profile.c
    src/transport/transport.c
)

add_library(openterface_core_common STATIC ${OPENTERFACE_CORE_COMMON_SOURCES})
target_include_directories(openterface_core_common PUBLIC include)
target_compile_options(openterface_core_common PRIVATE
    $<$<C_COMPILER_ID:MSVC>:/W4>
    $<$<NOT:$<C_COMPILER_ID:MSVC>>:-Wall -Wextra>
)

# ── input module: HID lookup, packet building, macro parsing ─────────────
set(OPENTERFACE_CORE_INPUT_SOURCES
    src/input/input_event.c
    src/input/input_hid.c
    src/input/input_packet.c
    src/input/input_parser.c
)

add_library(openterface_core_input STATIC ${OPENTERFACE_CORE_INPUT_SOURCES})
target_include_directories(openterface_core_input PUBLIC include)
target_compile_options(openterface_core_input PRIVATE
    $<$<C_COMPILER_ID:MSVC>:/W4>
    $<$<NOT:$<C_COMPILER_ID:MSVC>>:-Wall -Wextra>
)

set(OPENTERFACE_PROTOCOL_CH9329_SOURCES
    src/protocol/ch9329/protocol_ch9329.c
)

add_library(openterface_protocol_ch9329 STATIC ${OPENTERFACE_PROTOCOL_CH9329_SOURCES})
target_include_directories(openterface_protocol_ch9329 PUBLIC include)
target_compile_options(openterface_protocol_ch9329 PRIVATE
    $<$<C_COMPILER_ID:MSVC>:/W4>
    $<$<NOT:$<C_COMPILER_ID:MSVC>>:-Wall -Wextra>
)
target_link_libraries(openterface_protocol_ch9329 PUBLIC openterface_core_input)

# ── platform: internal-only backend layer ────────────────────────────────
set(OPENTERFACE_PLATFORM_SOURCES
    src/platform/platform_backend.c
    src/platform/platform_stub.c
)

if(WIN32)
    list(APPEND OPENTERFACE_PLATFORM_SOURCES src/platform/platform_windows.c)
elseif(UNIX AND NOT APPLE)
    list(APPEND OPENTERFACE_PLATFORM_SOURCES src/platform/platform_linux.c)
endif()

add_library(openterface_platform_internal STATIC ${OPENTERFACE_PLATFORM_SOURCES})
target_include_directories(openterface_platform_internal
    PUBLIC include
    PRIVATE
        src/platform
)
target_compile_options(openterface_platform_internal PRIVATE
    $<$<C_COMPILER_ID:MSVC>:/W4>
    $<$<NOT:$<C_COMPILER_ID:MSVC>>:-Wall -Wextra>
)
if(WIN32)
    target_link_libraries(openterface_platform_internal PRIVATE hid setupapi)
endif()

# ── native core: device/hid/chip/video-status/usb-mode ───────────────────
option(BUILD_NATIVE_CORE "Build the Native Core skeleton" ON)
if(BUILD_NATIVE_CORE)
    set(OPENTERFACE_NATIVE_CORE_SOURCES
        src/core_native.c
        src/chip/chip_detector.c
        src/hid/hid_device_session.c
        src/hid/hid_transaction_guard.c
        src/chip/video_chip_controller.c
        src/video_status/video_status_poller.c
        src/usb_mode/usb_mode_controller.c
        src/usb_mode/usb_mode_ms21xx.c
        src/usb_mode/usb_mode_serial.c
    )

    add_library(openterface_core_native STATIC ${OPENTERFACE_NATIVE_CORE_SOURCES})
    target_include_directories(openterface_core_native
        PUBLIC include
        PRIVATE
            src/platform
            src/hid
            src/chip
    )
    target_link_libraries(openterface_core_native
        PUBLIC
            openterface_core_common
            openterface_protocol_ch9329
        PRIVATE
            openterface_platform_internal
    )
    target_compile_options(openterface_core_native PRIVATE
        $<$<C_COMPILER_ID:MSVC>:/W4>
        $<$<NOT:$<C_COMPILER_ID:MSVC>>:-Wall -Wextra>
    )

    # ── connection watchdog ────────────────────────────────────────
    set(OPENTERFACE_WATCHDOG_SOURCES
        src/watchdog/connection_watchdog.c
    )

    add_library(openterface_watchdog STATIC ${OPENTERFACE_WATCHDOG_SOURCES})
    target_include_directories(openterface_watchdog PUBLIC include)
    target_link_libraries(openterface_watchdog PUBLIC openterface_core_common)
    target_compile_options(openterface_watchdog PRIVATE
        $<$<C_COMPILER_ID:MSVC>:/W4>
        $<$<NOT:$<C_COMPILER_ID:MSVC>>:-Wall -Wextra>
    )
endif()

# ── CLI demo ─────────────────────────────────────────────────────────────
option(BUILD_CLI "Build the CLI demo tool" ON)
if(BUILD_CLI)
    add_executable(input_cli cli/input_cli.c)
    target_link_libraries(input_cli PRIVATE openterface_core_input)
    target_compile_options(input_cli PRIVATE
        $<$<C_COMPILER_ID:MSVC>:/W4>
        $<$<NOT:$<C_COMPILER_ID:MSVC>>:-Wall -Wextra>
    )
endif()

# ── Tests ────────────────────────────────────────────────────────────────
option(BUILD_TESTS "Build the unit tests" ON)
if(BUILD_TESTS)
    enable_testing()

    add_executable(input_test tests/input_test.c)
    target_link_libraries(input_test PRIVATE openterface_core_input)
    add_test(NAME input_test COMMAND input_test)

    if(BUILD_NATIVE_CORE)
        add_executable(openterface_native_core_test tests/native_core_test.c)
        target_link_libraries(openterface_native_core_test PRIVATE openterface_core_native)
        add_test(NAME openterface_native_core_test COMMAND openterface_native_core_test)

        add_executable(watchdog_test tests/watchdog_test.c)
        target_link_libraries(watchdog_test PRIVATE openterface_watchdog openterface_core_common)
        add_test(NAME watchdog_test COMMAND watchdog_test)
    endif()

    add_executable(openterface_core_skeleton_test tests/core_skeleton_test.c)
    target_link_libraries(openterface_core_skeleton_test
        PRIVATE
            openterface_core_common
            openterface_core_input
            openterface_protocol_ch9329
    )
    add_test(NAME openterface_core_skeleton_test COMMAND openterface_core_skeleton_test)
endif()

# ── install rules ────────────────────────────────────────────────────────
install(DIRECTORY include/ DESTINATION include)
install(TARGETS openterface_core_common openterface_core_input openterface_protocol_ch9329 openterface_platform_internal
    ARCHIVE DESTINATION lib
)
if(BUILD_NATIVE_CORE)
    install(TARGETS openterface_core_native openterface_watchdog
        ARCHIVE DESTINATION lib
    )
endif()
