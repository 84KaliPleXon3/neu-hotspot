add_executable(
    cpp-parallel
    main.cpp
)
target_link_libraries(
    cpp-parallel Threads::Threads
)
