
include_directories(${QT_INCLUDES} )


########### install files ###############


kdoctools_create_handbook(index.docbook INSTALL_DESTINATION ${HTML_INSTALL_DIR}/en SUBDIR basket)



#original Makefile.am contents follow:

#KDE_DOCS = basket
#KDE_LANG = en
