include_directories(${GPGME_INCLUDES})

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

qt5_add_dbus_adaptor(libbasket_SRCS org.kde.basket.BNPView.xml bnpview.h BNPView)

qt5_add_resources(basket_RESOURCES ../basket.qrc)

add_library(LibBasket SHARED ${libbasket_SRCS} ${basket_FORM_HDRS} ${basket_RESOURCES})

target_link_libraries(LibBasket
    ${PHONON_LIBRARY}
    ${GPGME_VANILLA_LIBRARIES}
    KF5::Archive
    KF5::ConfigWidgets
    KF5::CoreAddons
    KF5::Crash
    KF5::DBusAddons
    KF5::FileMetaData
    KF5::GlobalAccel
    KF5::GuiAddons
    KF5::I18n
    KF5::IconThemes
    KF5::KCMUtils
    KF5::KIOWidgets
    KF5::Notifications
    KF5::Parts
    KF5::TextWidgets
    KF5::WindowSystem
    KF5::XmlGui
)

set_target_properties(LibBasket PROPERTIES
    VERSION
        ${Qt5Core_VERSION}
    SOVERSION
        ${Qt5Core_VERSION_MAJOR}
)

install(TARGETS LibBasket DESTINATION ${KDE_INSTALL_LIBDIR})

# Add unit tests after all variables have been set.
# If we save target_link_libraries to a variable, we can reuse it too

if (BUILD_TESTING)
    add_subdirectory(tests)
endif ()

########### next target ###############

set(basket_SRCS main.cpp mainwindow.cpp application.cpp)

add_executable(basket ${basket_SRCS})
target_link_libraries(basket LibBasket)

if (LIBGIT2_FOUND)
    target_link_libraries(LibBasket ${LIBGIT2_LIBRARIES})
    target_link_libraries(basket ${LIBGIT2_LIBRARIES})
endif()

install(TARGETS basket DESTINATION ${KDE_INSTALL_BINDIR})

add_library(basket_config_general MODULE kcm_basket/basket_config_general.cpp)
kcoreaddons_desktop_to_json(LibBasket "kcm_basket/basket_config_general.desktop" SERVICE_TYPES kcmodule.desktop)
target_link_libraries(basket_config_general LibBasket)
install(TARGETS basket_config_general DESTINATION ${KDE_INSTALL_PLUGINDIR}/pim/kcms/basket/)

add_library(basket_config_baskets MODULE kcm_basket/basket_config_baskets.cpp)
kcoreaddons_desktop_to_json(LibBasket "kcm_basket/basket_config_baskets.desktop" SERVICE_TYPES kcmodule.desktop)
target_link_libraries(basket_config_baskets LibBasket)
install(TARGETS basket_config_baskets DESTINATION ${KDE_INSTALL_PLUGINDIR}/pim/kcms/basket/)

add_library(basket_config_new_notes MODULE kcm_basket/basket_config_new_notes.cpp)
kcoreaddons_desktop_to_json(LibBasket "kcm_basket/basket_config_new_notes.desktop" SERVICE_TYPES kcmodule.desktop)
target_link_libraries(basket_config_new_notes LibBasket)
install(TARGETS basket_config_new_notes DESTINATION ${KDE_INSTALL_PLUGINDIR}/pim/kcms/basket/)

add_library(basket_config_notes_appearance MODULE kcm_basket/basket_config_notes_appearance.cpp)
kcoreaddons_desktop_to_json(LibBasket "kcm_basket/basket_config_notes_appearance.desktop" SERVICE_TYPES kcmodule.desktop)
target_link_libraries(basket_config_notes_appearance LibBasket)
install(TARGETS basket_config_notes_appearance DESTINATION ${KDE_INSTALL_PLUGINDIR}/pim/kcms/basket/)

add_library(basket_config_apps MODULE kcm_basket/basket_config_apps.cpp)
kcoreaddons_desktop_to_json(LibBasket "kcm_basket/basket_config_apps.desktop" SERVICE_TYPES kcmodule.desktop)
target_link_libraries(basket_config_apps LibBasket)
install(TARGETS basket_config_apps DESTINATION ${KDE_INSTALL_PLUGINDIR}/pim/kcms/basket/)

add_library(basket_config_version_sync MODULE kcm_basket/basket_config_version_sync.cpp)
kcoreaddons_desktop_to_json(LibBasket "kcm_basket/basket_config_version_sync.desktop" SERVICE_TYPES kcmodule.desktop)
target_link_libraries(basket_config_version_sync LibBasket)
install(TARGETS basket_config_version_sync DESTINATION ${KDE_INSTALL_PLUGINDIR}/pim/kcms/basket/)

set(DESKTOP_FILES
    kcm_basket/basket_config_general.desktop
    kcm_basket/basket_config_baskets.desktop
    kcm_basket/basket_config_new_notes.desktop
    kcm_basket/basket_config_notes_appearance.desktop
    kcm_basket/basket_config_apps.desktop
    kcm_basket/basket_config_version_sync.desktop
)
install(FILES ${DESKTOP_FILES} DESTINATION ${KDE_INSTALL_KSERVICES5DIR})


install(FILES org.kde.basket.desktop DESTINATION ${KDE_INSTALL_APPDIR})
install(FILES org.kde.basket.appdata.xml DESTINATION ${KDE_INSTALL_METAINFODIR})
install(FILES basketui.rc DESTINATION ${KDE_INSTALL_KXMLGUI5DIR}/basket)

