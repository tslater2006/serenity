serenity_component(
    HexEditor
    RECOMMENDED
    TARGETS HexEditor
)

compile_gml(HexEditorWindow.gml HexEditorWindowGML.h hex_editor_window_gml)
compile_gml(GoToOffsetDialog.gml GoToOffsetDialogGML.h go_to_offset_dialog_gml)
compile_gml(FindDialog.gml FindDialogGML.h find_dialog_gml)

set(SOURCES
    FindDialog.cpp
    GoToOffsetDialog.cpp
    HexDocument.cpp
    HexEditor.cpp
    HexEditorWidget.cpp
    main.cpp
)

set(GENERATED_SOURCES
    FindDialogGML.h
    GoToOffsetDialogGML.h
    HexEditorWindowGML.h
)

serenity_app(HexEditor ICON app-hex-editor)
target_link_libraries(HexEditor PRIVATE LibCore LibGfx LibGUI LibConfig LibDesktop LibFileSystemAccessClient LibMain LibTextCodec)
