########### next target ###############

include(ECMMarkAsTest)
include(ECMAddTests)

find_package(Qt5 ${REQUIRED_QT_VERSION} CONFIG REQUIRED Test)

set(BASKET_TEST_SRC
    notetest.cpp
    basketviewtest.cpp
    basketscenmodeltest.cpp
    toolstest.cpp
    archivetest.cpp
)

ecm_add_tests(${BASKET_TEST_SRC} LINK_LIBRARIES LibBasket Qt5::Test)

