########### next target ###############

include(ECMMarkAsTest)
include(ECMAddTests)

find_package(Qt6 ${REQUIRED_QT_VERSION} CONFIG REQUIRED Test)

set(BASKET_TEST_SRC
    notetest.cpp
    basketviewtest.cpp
    toolstest.cpp
    archivetest.cpp
)

ecm_add_tests(${BASKET_TEST_SRC} LINK_LIBRARIES LibBasket Qt6::Test)

