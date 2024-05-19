include_directories(${GPGME_INCLUDES})

set(CMAKE_AUTOMOC ON)

find_package(KF6FileMetaData REQUIRED)
find_package(KF6I18n REQUIRED)
find_package(KF6Service REQUIRED)
find_package(KF6IconThemes REQUIRED)
find_package(KF6Codecs REQUIRED)
find_package(KF6WindowSystem)

########### next target ###############

set(libbasket_SRCS
    aboutdata.cpp
    archive.cpp
    backgroundmanager.cpp
    backup.cpp
    basketfactory.cpp
    basketlistview.cpp
    basketproperties.cpp
    basketscene.cpp
    basketstatusbar.cpp
    basketview.cpp
    bnpview.cpp
    colorpicker.cpp
    common.cpp
    debugwindow.cpp
    decoratedbasket.cpp
    file_metadata.cpp
    filter.cpp
    focusedwidgets.cpp
    formatimporter.cpp
    global.cpp
    gitwrapper.cpp
    htmlexporter.cpp
    history.cpp
    kcolorcombo2.cpp
    kgpgme.cpp
    linklabel.cpp
    newbasketdialog.cpp
    note.cpp
    notecontent.cpp
    notedrag.cpp
    noteedit.cpp
    notefactory.cpp
    noteselection.cpp
    password.cpp
    regiongrabber.cpp
    settings.cpp
    settings_versionsync.cpp
    softwareimporters.cpp
    tag.cpp
    tagsedit.cpp
    tools.cpp
    variouswidgets.cpp
    xmlwork.cpp
)

set(PIMO_CPP "") # One of the generated files

ki18n_wrap_ui(basket_FORM_HDRS passwordlayout.ui basketproperties.ui settings_versionsync.ui)

qt6_add_dbus_adaptor(libbasket_SRCS org.kde.basket.BNPView.xml bnpview.h BNPView)

qt6_add_resources(basket_RESOURCES ../basket.qrc)

add_library(LibBasket SHARED ${libbasket_SRCS} ${basket_FORM_HDRS} ${basket_RESOURCES})

target_link_libraries(LibBasket
    ${PHONON_LIBRARY}
    ${GPGME_VANILLA_LIBRARIES}
    KF6::Archive
    KF6::ConfigWidgets
    KF6::CoreAddons
    KF6::Codecs
    KF6::Crash
    KF6::DBusAddons
    KF6::FileMetaData
    KF6::GlobalAccel
    KF6::GuiAddons
    KF6::I18n
    KF6::IconWidgets
    KF6::IconThemes
    KF6::KCMUtils
    KF6::KIOWidgets
    KF6::Notifications
    KF6::Parts
    KF6::TextWidgets
    KF6::WindowSystem
    KF6::XmlGui
)

set_target_properties(LibBasket PROPERTIES
    VERSION
        ${Qt6Core_VERSION}
    SOVERSION
        ${Qt6Core_VERSION_MAJOR}
)

install(TARGETS LibBasket DESTINATION ${KDE_INSTALL_LIBDIR})

# Add unit tests after all variables have been set.
# If we save target_link_libraries to a variable, we can reuse it too

if (BUILD_TESTING)
    add_subdirectory(tests)
endif ()

########### next target ###############

set(basket_SRCS
    main.cpp
    mainwindow.cpp
    application.cpp
    basket.qrc
)

add_executable(basket ${basket_SRCS})
target_link_libraries(basket LibBasket)

if (LIBGIT2_FOUND)
    target_link_libraries(LibBasket ${LIBGIT2_LIBRARIES})
    target_link_libraries(basket ${LIBGIT2_LIBRARIES})
endif()

install(TARGETS basket DESTINATION ${KDE_INSTALL_BINDIR})

add_library(basket_config_general MODULE kcm_basket/basket_config_general.cpp)
target_link_libraries(basket_config_general LibBasket)
install(TARGETS basket_config_general DESTINATION ${KDE_INSTALL_PLUGINDIR}/pim/kcms/basket/)

add_library(basket_config_baskets MODULE kcm_basket/basket_config_baskets.cpp)
target_link_libraries(basket_config_baskets LibBasket)
install(TARGETS basket_config_baskets DESTINATION ${KDE_INSTALL_PLUGINDIR}/pim/kcms/basket/)

add_library(basket_config_new_notes MODULE kcm_basket/basket_config_new_notes.cpp)
target_link_libraries(basket_config_new_notes LibBasket)
install(TARGETS basket_config_new_notes DESTINATION ${KDE_INSTALL_PLUGINDIR}/pim/kcms/basket/)

add_library(basket_config_notes_appearance MODULE kcm_basket/basket_config_notes_appearance.cpp)
target_link_libraries(basket_config_notes_appearance LibBasket)
install(TARGETS basket_config_notes_appearance DESTINATION ${KDE_INSTALL_PLUGINDIR}/pim/kcms/basket/)

add_library(basket_config_apps MODULE kcm_basket/basket_config_apps.cpp)
target_link_libraries(basket_config_apps LibBasket)
install(TARGETS basket_config_apps DESTINATION ${KDE_INSTALL_PLUGINDIR}/pim/kcms/basket/)

add_library(basket_config_version_sync MODULE kcm_basket/basket_config_version_sync.cpp)
target_link_libraries(basket_config_version_sync LibBasket)
install(TARGETS basket_config_version_sync DESTINATION ${KDE_INSTALL_PLUGINDIR}/pim/kcms/basket/)

set(DESKTOP_FILES
    kcm_basket/basket_config_general.json
    kcm_basket/basket_config_baskets.json
    kcm_basket/basket_config_new_notes.json
    kcm_basket/basket_config_notes_appearance.json
    kcm_basket/basket_config_apps.json
    kcm_basket/basket_config_version_sync.json
)
install(FILES ${DESKTOP_FILES} DESTINATION ${SERVICES_INSTALL_DIR})


install(FILES org.kde.basket.desktop DESTINATION ${KDE_INSTALL_APPDIR})
install(FILES org.kde.basket.appdata.xml DESTINATION ${KDE_INSTALL_METAINFODIR})

