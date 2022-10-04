########### next target ###############

set(basketweaver_SRCS
    main.cpp
    weaver.h
    weaver.cpp
    )

add_executable(basketweaver ${basketweaver_SRCS})

target_link_libraries(basketweaver LibBasket Qt5::Core)

install(TARGETS basketweaver DESTINATION ${KDE_INSTALL_BINDIR})
