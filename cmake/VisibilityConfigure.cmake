function(visibility_setup)
    if (${CMAKE_BUILD_TYPE} MATCHES "Release" OR ${CMAKE_BUILD_TYPE} MATCHES "RelWithDebInfo")
      set(CMAKE_C_VISIBILITY_PRESET hidden PARENT_SCOPE)
      set(CMAKE_CXX_VISIBILITY_PRESET hidden PARENT_SCOPE)
      set(CMAKE_VISIBILITY_INLINES_HIDDEN ON PARENT_SCOPE)
    endif()
endfunction()
