set(kritaasccdl_SOURCES
    kis_asccdl_filter.cpp
    kis_wdg_asccdl.cpp
)

ki18n_wrap_ui(kritaasccdl_SOURCES
    wdg_asccdl.ui
    )
kis_add_library(kritaasccdl MODULE ${kritaasccdl_SOURCES})
target_link_libraries(kritaasccdl kritaui)
install(TARGETS kritaasccdl  DESTINATION ${KRITA_PLUGIN_INSTALL_DIR})
install( FILES
    asccdl.action
DESTINATION  ${KDE_INSTALL_DATADIR}/krita/actions)
