set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR})

include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/src 
                    ${CMAKE_BINARY_DIR}/src)

link_directories(${CMAKE_BINARY_DIR}/lib)


macro(basket_standalone_unit_test _testname)
    kde4_add_unit_test(${_testname} ${_testname}.cpp ${_extra_sources} ${ARGN})
    target_link_libraries(${_testname} basketcommon ${KDE4_KDEUI_LIBS} ${QT_QTTEST_LIBRARY})
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

    kde4_add_unit_test(${_testname} ${_testname}.cpp ${baskettest_SRCS})
    
    target_link_libraries(${_testname} basketcommon ${GPGME_VANILLA_LIBRARIES}
      ${NEPOMUK_LIBRARIES} ${KDE4_KDEUI_LIBS} ${KDE4_KIO_LIBS} ${KDE4_KPARTS_LIBS} ${KDE4_KFILE_LIBS} ${KDE4_PHONON_LIBS}
      ${QIMAGEBLITZ_LIBRARIES} ${QT_QTTEST_LIBRARY})
endmacro(basket_full_unit_test)


basket_standalone_unit_test(notetest)
basket_standalone_unit_test(basketviewtest)
basket_full_unit_test(toolstest)