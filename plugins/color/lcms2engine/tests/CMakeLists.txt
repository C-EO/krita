include(KritaAddBrokenUnitTest)

if(OpenEXR_FOUND)
    include_directories(SYSTEM ${OPENEXR_INCLUDE_DIRS})
endif()
include_directories( ${LCMS2_INCLUDE_DIR} )

kis_add_tests(
    TestKoLcmsColorProfile.cpp
    TestColorSpaceRegistry.cpp
    TestLcmsRGBP2020PQColorSpace.cpp
    TestProfileGeneration.cpp
    NAME_PREFIX "plugins-lcmsengine-"
    LINK_LIBRARIES kritawidgets kritapigment KF${KF_MAJOR}::I18n kritatestsdk ${LCMS2_LIBRARIES}
    )
