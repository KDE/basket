set(basketthumbcreator_PART_SRCS basketthumbcreator.cpp)

add_library(basketthumbcreator MODULE ${basketthumbcreator_PART_SRCS})

target_link_libraries(basketthumbcreator
  Qt5::Core Qt5::Gui
  KF5::I18n KF5::XmlGui KF5::KIOWidgets)

install(TARGETS basketthumbcreator DESTINATION ${PLUGIN_INSTALL_DIR})


########### install files ###############

install(FILES basketthumbcreator.desktop DESTINATION ${SERVICES_INSTALL_DIR})
install(FILES basket.xml DESTINATION ${XDG_MIME_INSTALL_DIR})
