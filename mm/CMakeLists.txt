add_library(ublk_mm STATIC
    aligned_allocator.hpp
    cache_line_aligned_allocator.hpp
    generic_allocators.hpp
    mem.hpp
    mem_allocator.hpp
    mem_cached_allocator.cpp
    mem_cached_allocator.hpp
    mem_chunk_pool.hpp
    mem_pool_allocator.hpp
    mem_types.hpp
    page_aligned_allocator.hpp
    pool_allocator.hpp
    pool_allocators.hpp
)

target_include_directories(ublk_mm PRIVATE ${CMAKE_CURRENT_LIST_DIR}/..)
target_link_libraries(ublk_mm PRIVATE
    ublk::utils
)
set_property(TARGET ublk_mm PROPERTY POSITION_INDEPENDENT_CODE ON)

add_library(ublk::mm ALIAS ublk_mm)
