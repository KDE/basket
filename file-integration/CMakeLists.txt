set(basketthumbcreator_PART_SRCS basketthumbcreator.cpp)

kde4_add_plugin(basketthumbcreator ${basketthumbcreator_PART_SRCS})

target_link_libraries(basketthumbcreator ${KDE4_KIO_LIBS})

install(TARGETS basketthumbcreator DESTINATION ${PLUGIN_INSTALL_DIR})


########### install files ###############

find_package(SharedMimeInfo REQUIRED)

install(FILES  basketthumbcreator.desktop DESTINATION ${SERVICES_INSTALL_DIR})
install(FILES basket.xml DESTINATION ${XDG_MIME_INSTALL_DIR})
update_xdg_mimetypes(${XDG_MIME_INSTALL_DIR})

kde4_install_icons(${ICON_INSTALL_DIR})
