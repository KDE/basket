set(basketthumbcreator_PART_SRCS basketthumbcreator.cpp)

add_library(basketthumbcreator MODULE ${basketthumbcreator_PART_SRCS})

target_link_libraries(basketthumbcreator
  Qt5::Core Qt5::Gui
  KF5::I18n KF5::XmlGui KF5::KIOWidgets)

install(TARGETS basketthumbcreator DESTINATION ${KDE_INSTALL_PLUGINDIR})


########### install files ###############

install(FILES basketthumbcreator.desktop DESTINATION ${KDE_INSTALL_KSERVICES5DIR})
install(FILES basket.xml DESTINATION ${KDE_INSTALL_MIMEDIR})
