########### next target ###############

set(libbasket_SRCS
    aboutdata.cpp aboutdata.h
    animation.cpp animation.h
    archive.cpp archive.h
    backgroundmanager.cpp backgroundmanager.h
    backup.cpp backup.h
    basketfactory.cpp basketfactory.h
    basketlistview.cpp basketlistview.h
    basketproperties.cpp basketproperties.h
    basketscene.cpp basketscene.h
    basketstatusbar.cpp basketstatusbar.h
    basketview.cpp basketview.h
    bnpview.cpp
    colorpicker.cpp colorpicker.h
    common.cpp common.h
    debugwindow.cpp debugwindow.h
    decoratedbasket.cpp decoratedbasket.h
    file_metadata.cpp file_metadata.h
    filter.cpp filter.h
    focusedwidgets.cpp focusedwidgets.h
    formatimporter.cpp formatimporter.h
    global.cpp global.h
    gitwrapper.cpp gitwrapper.h
    htmlexporter.cpp htmlexporter.h
    history.cpp history.h
    kcolorcombo2.cpp kcolorcombo2.h
    kgpgme.cpp kgpgme.h
    linklabel.cpp linklabel.h
    newbasketdialog.cpp newbasketdialog.h
    note.cpp note.h
    notecontent.cpp notecontent.h
    notedrag.cpp notedrag.h
    noteedit.cpp noteedit.h
    notefactory.cpp notefactory.h
    noteselection.cpp noteselection.h
    password.cpp password.h
    regiongrabber.cpp regiongrabber.h
    settings.cpp settings.h
    settings_versionsync.cpp settings_versionsync.h
    softwareimporters.cpp softwareimporters.h
    tag.cpp tag.h
    tagsedit.cpp tagsedit.h
    tools.cpp tools.h
    variouswidgets.cpp variouswidgets.h
    xmlwork.cpp xmlwork.h
)

ki18n_wrap_ui(basket_FORM_HDRS passwordlayout.ui basketproperties.ui settings_versionsync.ui)

qt_add_dbus_adaptor(libbasket_SRCS org.kde.basket.BNPView.xml bnpview.h BNPView)

qt_add_resources(basket_RESOURCES ../basket.qrc)

add_library(LibBasket SHARED ${libbasket_SRCS} ${basket_FORM_HDRS} ${basket_RESOURCES})

target_link_libraries(LibBasket
    KF6::Archive
    KF6::ConfigWidgets
    KF6::CoreAddons
    KF6::Codecs
    KF6::ColorScheme
    KF6::Crash
    KF6::DBusAddons
    KF6::FileMetaData
    KF6::GlobalAccel
    KF6::GuiAddons
    KF6::I18n
    KF6::IconWidgets
    KF6::IconThemes
    KF6::JobWidgets
    KF6::KCMUtils
    KF6::KIOWidgets
    KF6::Notifications
    KF6::Parts
    KF6::TextWidgets
    KF6::WindowSystem
    KF6::XmlGui
    Qt::Core
    Qt::Multimedia
)

if(TARGET PkgConfig::gpgme)
    target_link_libraries(LibBasket PkgConfig::gpgme)
endif()

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
    mainwindow.cpp mainwindow.h
    application.cpp application.h
    basket.qrc
)

if(DEBUG_PIPE)
    list(APPEND basket_SRCS debugwindow.cpp debugwindow.h)
endif()

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


install(FILES org.kde.basket.desktop DESTINATION ${KDE_INSTALL_APPDIR})
install(FILES org.kde.basket.appdata.xml DESTINATION ${KDE_INSTALL_METAINFODIR})

