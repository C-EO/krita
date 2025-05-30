set(kritalibkis_LIB_SRCS
    Canvas.cpp
    Channel.cpp
    DockWidget.cpp
    DockWidgetFactoryBase.cpp
    Document.cpp
    Filter.cpp
    InfoObject.cpp
    Krita.cpp
    ManagedColor.cpp
    Node.cpp
    Notifier.cpp
    PaintingResources.cpp
    PresetChooser.cpp
    Preset.cpp
    Palette.cpp
    PaletteView.cpp
    Scratchpad.cpp
    Swatch.cpp
    Resource.cpp
    Selection.cpp
    View.cpp
    Extension.cpp
    Window.cpp
    GroupLayer.cpp
    CloneLayer.cpp
    FileLayer.cpp
    FilterLayer.cpp
    FillLayer.cpp
    VectorLayer.cpp
    FilterMask.cpp
    SelectionMask.cpp
    TransparencyMask.cpp
    TransformMask.cpp
    ColorizeMask.cpp

    Shape.cpp
    GroupShape.cpp

    LibKisUtils.cpp

    AngleSelector.cpp
    IntParseSpinBox.cpp
    DoubleParseSpinBox.cpp
    SliderSpinBox.cpp

    GuidesConfig.cpp
    GridConfig.cpp

    FileDialog.cpp
)

kis_add_library(kritalibkis SHARED ${kritalibkis_LIB_SRCS})
generate_export_header(kritalibkis)

target_link_libraries(kritalibkis kritaui kritaimage kritaversion)
target_link_libraries(kritalibkis LINK_INTERFACE_LIBRARIES kritaimage kritaui)

set_target_properties(kritalibkis PROPERTIES
    VERSION ${GENERIC_KRITA_LIB_VERSION} SOVERSION ${GENERIC_KRITA_LIB_SOVERSION}
)

install(TARGETS kritalibkis  ${INSTALL_TARGETS_DEFAULT_ARGS})


add_subdirectory(tests)
