cmake_minimum_required(VERSION 3.14)

find_package(Catch2 3 CONFIG REQUIRED)

# Test executable
add_executable(ReactiveLitepp_tests
    test_event_basic.cpp
    test_event_multithreading.cpp
    test_event_lifetime.cpp
    test_event_stress.cpp
    test_property.cpp
    test_property_observability.cpp
    test_observable_object.cpp
    test_observable_object_getproperty.cpp
    test_observable_object_makeproperty.cpp
    test_observable_collection.cpp
)

target_link_libraries(ReactiveLitepp_tests PRIVATE 
    ReactiveLitepp
    Catch2::Catch2WithMain
)

target_compile_features(ReactiveLitepp_tests PRIVATE cxx_std_20)

# Enable testing
include(CTest)
include(Catch)
catch_discover_tests(ReactiveLitepp_tests)
