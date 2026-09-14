cmake_minimum_required(VERSION 3.14)
project(OpenterfaceCore LANGUAGES C CXX VERSION 0.1.0)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# ── output directories ──────────────────────────────────────────────────
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# ── platform: internal-only backend layer ────────────────────────────────
# Kept separate: platform-specific, not part of the public API
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
    PRIVATE src/platform
)
target_compile_options(openterface_platform_internal PRIVATE
    $<$<C_COMPILER_ID:MSVC>:/W4>
    $<$<NOT:$<C_COMPILER_ID:MSVC>>:-Wall -Wextra>
)
if(WIN32)
    target_link_libraries(openterface_platform_internal PRIVATE hid setupapi)
endif()

# ── unified Core SDK ─────────────────────────────────────────────────────
# Single library merging all core functionality:
#   - Common layer: version, capability, device, profile, transport
#   - Input layer: HID lookup, packet building, macro parsing
#   - Protocol layer: CH9329 serial protocol
#   - Native layer: chip detection, HID session, video status, USB mode (optional)
#   - Watchdog: connection health monitoring (optional)

set(OPENTERFACE_CORE_SOURCES
    # Common layer: data structures and abstract interfaces
    src/core/version.c
    src/capability/capability.c
    src/device/device_info.c
    src/profile/profile.c
    src/transport/transport.c

    # Input layer: HID event mapping and packet building
    src/input/input_event.c
    src/input/input_code_map.c
    src/input/input_packet.c
    src/input/input_parser.c

    # Protocol layer: CH9329 serial protocol
    src/protocol/ch9329/protocol_ch9329.c
)

# Native layer: platform-dependent device interaction (optional)
option(BUILD_NATIVE_CORE "Build native platform layer (chip/HID/video/USB/watchdog)" ON)
if(BUILD_NATIVE_CORE)
    list(APPEND OPENTERFACE_CORE_SOURCES
        # Core entry: platform bridge for serial transport
        src/core/native_entry.c

        # Chip detection and video chip control
        src/chip/chip_detector.c
        src/chip/video_chip_controller.c

        # HID device session management
        src/hid/hid_device_session.c
        src/hid/hid_transaction_guard.c

        # Video status polling
        src/video_status/video_status_poller.c

        # USB mode switching
        src/usb_mode/usb_mode_controller.c
        src/usb_mode/usb_mode_ms21xx.c
        src/usb_mode/usb_mode_serial.c

        # Connection watchdog
        src/watchdog/connection_watchdog.c
    )
endif()

add_library(openterface_core STATIC ${OPENTERFACE_CORE_SOURCES})
target_include_directories(openterface_core
    PUBLIC include
)
target_compile_options(openterface_core PRIVATE
    $<$<C_COMPILER_ID:MSVC>:/W4>
    $<$<NOT:$<C_COMPILER_ID:MSVC>>:-Wall -Wextra>
)

if(BUILD_NATIVE_CORE)
    # Native code needs access to platform internals and private headers
    target_include_directories(openterface_core
        PRIVATE
            src/platform
            src/hid
            src/chip
    )
    target_link_libraries(openterface_core
        PRIVATE
            openterface_platform_internal
    )
endif()

# ── CLI demo ─────────────────────────────────────────────────────────────
option(BUILD_CLI "Build the CLI demo tool" ON)
if(BUILD_CLI)
    add_executable(input_cli cli/input_cli.c)
    target_link_libraries(input_cli PRIVATE openterface_core)
    target_compile_options(input_cli PRIVATE
        $<$<C_COMPILER_ID:MSVC>:/W4>
        $<$<NOT:$<C_COMPILER_ID:MSVC>>:-Wall -Wextra>
    )
endif()

# ── Tests ────────────────────────────────────────────────────────────────
option(BUILD_TESTS "Build the unit tests" ON)
if(BUILD_TESTS)
    enable_testing()

    # Input module tests (pure logic, no platform dependency)
    add_executable(input_test tests/input_test.c)
    target_link_libraries(input_test PRIVATE openterface_core)
    add_test(NAME input_test COMMAND input_test)

    add_executable(key_test_plan tests/key_test_plan.c)
    target_link_libraries(key_test_plan PRIVATE openterface_core)
    add_test(NAME key_test_plan COMMAND key_test_plan)

    # Transport module tests
    add_executable(transport_test tests/transport_test.c)
    target_link_libraries(transport_test PRIVATE openterface_core)
    add_test(NAME transport_test COMMAND transport_test)

    # Device module tests
    add_executable(device_test tests/device_test.c)
    target_link_libraries(device_test PRIVATE openterface_core)
    add_test(NAME device_test COMMAND device_test)

    # Capability module tests
    add_executable(capability_test tests/capability_test.c)
    target_link_libraries(capability_test PRIVATE openterface_core)
    add_test(NAME capability_test COMMAND capability_test)

    # Profile module tests
    add_executable(profile_test tests/profile_test.c)
    target_link_libraries(profile_test PRIVATE openterface_core)
    add_test(NAME profile_test COMMAND profile_test)

    # Protocol ch9329 module tests
    add_executable(protocol_ch9329_test tests/protocol_ch9329_test.c)
    target_link_libraries(protocol_ch9329_test PRIVATE openterface_core)
    add_test(NAME protocol_ch9329_test COMMAND protocol_ch9329_test)

    # Native core tests (require BUILD_NATIVE_CORE)
    if(BUILD_NATIVE_CORE)
        add_executable(openterface_native_core_test tests/native_core_test.c)
        target_link_libraries(openterface_native_core_test PRIVATE openterface_core)
        add_test(NAME openterface_native_core_test COMMAND openterface_native_core_test)

        add_executable(watchdog_test tests/watchdog_test.c)
        target_link_libraries(watchdog_test PRIVATE openterface_core)
        add_test(NAME watchdog_test COMMAND watchdog_test)
    endif()

    # Skeleton test: exercises common + input + protocol APIs
    add_executable(openterface_core_skeleton_test tests/core_skeleton_test.c)
    target_link_libraries(openterface_core_skeleton_test PRIVATE openterface_core)
    add_test(NAME openterface_core_skeleton_test COMMAND openterface_core_skeleton_test)
endif()

# ── install rules ────────────────────────────────────────────────────────
install(DIRECTORY include/ DESTINATION include)
install(TARGETS openterface_core
    ARCHIVE DESTINATION lib
)
