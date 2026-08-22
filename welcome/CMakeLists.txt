file(GLOB BASKETS_FILES RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" Welcome_*.baskets)
foreach(FILE IN LISTS BASKETS_FILES)
    string(REGEX REPLACE "^Welcome_(.*)\.baskets$" "\\1" FILE_LANGUAGE "${FILE}")
    if (FILE_LANGUAGE STREQUAL "en" OR FILE_LANGUAGE STREQUAL "en_US")
        set(FILE_DESTINATION "${KDE_INSTALL_DATADIR}/basket/welcome")
    else ()
        set(FILE_DESTINATION "${KDE_INSTALL_DATADIR}/basket/welcome/l10n/${FILE_LANGUAGE}")
    endif ()
    install(FILES
        "${CMAKE_CURRENT_SOURCE_DIR}/${FILE}"
        DESTINATION "${FILE_DESTINATION}"
        RENAME Welcome.baskets
    )
endforeach()
