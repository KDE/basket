set(basketthumbcreator_PART_SRCS basketthumbcreator.cpp)

add_library(basketthumbcreator MODULE ${basketthumbcreator_PART_SRCS})

target_link_libraries(basketthumbcreator
  Qt5::Core Qt5::Gui
  KF5::I18n KF5::XmlGui KF5::KIOWidgets)

install(TARGETS basketthumbcreator DESTINATION ${PLUGIN_INSTALL_DIR})


########### install files ###############

find_program(XDG-MIME_EXECUTABLE xdg-mime)
find_program(XDG-DESKTOP-MENU_EXECUTABLE xdg-desktop-menu)

install(FILES  basketthumbcreator.desktop DESTINATION ${SERVICES_INSTALL_DIR})
install(CODE "
execute_process(COMMAND ${XDG-MIME_EXECUTABLE} install --novendor
  ${CMAKE_CURRENT_SOURCE_DIR}/basket.xml)
execute_process(COMMAND ${XDG-DESKTOP-MENU_EXECUTABLE} install
  --novendor ${CMAKE_SOURCE_DIR}/src/basket.desktop)
execute_process(COMMAND ${XDG-MIME_EXECUTABLE} default
  ${CMAKE_SOURCE_DIR}/src/basket.desktop application/x-basket-item)
")
