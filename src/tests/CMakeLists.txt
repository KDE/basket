include(ECMMarkAsTest)

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR})

find_package(Qt5Test REQUIRED)

include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/src 
                    ${CMAKE_BINARY_DIR}/src)

link_directories(${CMAKE_BINARY_DIR}/lib)


macro(basket_standalone_unit_test _testname)
    add_executable(${_testname} ${_testname}.cpp ${_extra_sources} ${ARGN})
    ecm_mark_as_test(${_testname})
    target_link_libraries(${_testname} basketcommon Qt5::Test)
endmacro(basket_standalone_unit_test)


macro(basket_full_unit_test _testname)
    include_directories(${KDE4_INCLUDES} ${KDE4_INCLUDE_DIR} ${SOPRANO_INCLUDE_DIR})
      
    #set(baskettest_SRCS " ")
    foreach (cpp ${basketcommon_LIB_SRCS}) 
      set(IS_GENERATED (${cpp} MATCHES "(^/)")) # Generated files contain full path (start with '/')
      if(${IS_GENERATED})
        list(APPEND baskettest_SRCS ${cpp})
      else(${VGENERATED})
        list(APPEND baskettest_SRCS "${CMAKE_SOURCE_DIR}/src/${cpp}")
      endif(${IS_GENERATED})
    endforeach ()

    add_executable(${_testname} ${_testname}.cpp ${baskettest_SRCS})
    ecm_mark_as_test(${_testname})
    
    target_link_libraries(${_testname} basketcommon ${GPGME_VANILLA_LIBRARIES} ${NEPOMUK_LIBRARIES} Qt5::Test)
endmacro(basket_full_unit_test)


basket_standalone_unit_test(notetest)
basket_standalone_unit_test(basketviewtest)
basket_full_unit_test(toolstest)