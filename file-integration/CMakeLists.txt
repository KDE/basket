set(basketthumbcreator_PART_SRCS basketthumbcreator.cpp basketthumbcreator.h)

add_library(basketthumbcreator MODULE ${basketthumbcreator_PART_SRCS})

target_link_libraries(basketthumbcreator
  Qt::Core Qt::Gui
  KF6::KIOGui)

install(TARGETS basketthumbcreator DESTINATION ${KDE_INSTALL_PLUGINDIR})


########### install files ###############

install(FILES basket.xml DESTINATION ${KDE_INSTALL_MIMEDIR})
