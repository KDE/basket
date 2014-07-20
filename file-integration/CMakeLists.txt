set(basketthumbcreator_PART_SRCS basketthumbcreator.cpp)

kde4_add_plugin(basketthumbcreator ${basketthumbcreator_PART_SRCS})

target_link_libraries(basketthumbcreator ${KDE4_KIO_LIBS})

install(TARGETS basketthumbcreator DESTINATION ${PLUGIN_INSTALL_DIR})


########### install files ###############

find_package(SharedMimeInfo REQUIRED)
find_program(XDG-DESKTOP-MENU_EXECUTABLE xdg-desktop-menu)

install(FILES  basketthumbcreator.desktop DESTINATION ${SERVICES_INSTALL_DIR})
install(FILES basket.xml DESTINATION ${XDG_MIME_INSTALL_DIR})
update_xdg_mimetypes(${XDG_MIME_INSTALL_DIR})
install(CODE "
execute_process(COMMAND ${XDG-DESKTOP-MENU_EXECUTABLE} install
  --novendor ${CMAKE_SOURCE_DIR}/src/basket.desktop)
")

kde4_install_icons(${ICON_INSTALL_DIR})
