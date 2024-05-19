set(basketthumbcreator_PART_SRCS basketthumbcreator.cpp)

add_library(basketthumbcreator MODULE ${basketthumbcreator_PART_SRCS})

target_link_libraries(basketthumbcreator
  Qt6::Core Qt6::Gui
  KF6::I18n KF6::XmlGui KF6::KIOGui KF6::KCMUtils KF6::KIOWidgets KF6::KIOCore)

install(TARGETS basketthumbcreator DESTINATION ${KDE_INSTALL_PLUGINDIR})


########### install files ###############

install(FILES basketthumbcreator.desktop DESTINATION ${KDE_INSTALL_KSERVICES5DIR})
install(FILES basket.xml DESTINATION ${KDE_INSTALL_MIMEDIR})
