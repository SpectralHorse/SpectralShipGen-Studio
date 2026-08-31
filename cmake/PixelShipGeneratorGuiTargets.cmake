# GUI/Application-owned targets. The Library targets must already exist, either
# from the combined development tree or from an explicitly supplied checkout.
if(NOT TARGET PixelShipGeneratorCore OR NOT TARGET PixelShipGeneratorDiagnosticsCore)
    message(FATAL_ERROR "PixelShipGenerator GUI requires PixelShipGeneratorCore and PixelShipGeneratorDiagnosticsCore targets from the Library checkout.")
endif()

find_package(Threads REQUIRED)

add_library(PixelShipGeneratorDiagnosticsAppSupport STATIC DiagnosticsApp/DiagnosticsAppController.cpp)
target_include_directories(PixelShipGeneratorDiagnosticsAppSupport PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/DiagnosticsApp)
target_link_libraries(PixelShipGeneratorDiagnosticsAppSupport PUBLIC PixelShipGeneratorDiagnosticsCore Threads::Threads)
target_compile_features(PixelShipGeneratorDiagnosticsAppSupport PUBLIC cxx_std_17)

add_library(PixelShipGeneratorApplicationCommon STATIC
    Application/SFMLPixelText.cpp
    Application/SFMLCharts.cpp
    Application/SFMLImageAdapter.cpp
)
target_include_directories(PixelShipGeneratorApplicationCommon PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/Application)
target_link_libraries(PixelShipGeneratorApplicationCommon PUBLIC PixelShipGeneratorCore sfml-graphics)
target_compile_features(PixelShipGeneratorApplicationCommon PUBLIC cxx_std_17)

add_executable(PixelShipGenerator
    Preview/GenerationCalibration.cpp
    Preview/GenerationCalibrationSerializer.cpp
    Preview/AttributeRerollStudio.cpp
    Preview/PreviewAnimationSession.cpp
    Preview/ConfigurationEditorControls.cpp
    Preview/ConfigurationBundle.cpp
    Preview/PreviewConfigurationEditor.cpp
    Preview/ShipGenerationProfileEditorBindings.cpp
    Preview/ShipFactionProfileEditorBindings.cpp
    Preview/ShipPaletteConfigurationEditorBindings.cpp
    Preview/RuntimeCustomPresetWorkspace.cpp
    Preview/UserPresetPersistence.cpp
    Preview/StructuralProfileSelection.cpp
    Preview/FactionProfileSelection.cpp
    Preview/PaletteProfileSelection.cpp
    Preview/PreviewCollectionSession.cpp
    Preview/PreviewCommand.cpp
    Preview/PreviewInspection.cpp
    Preview/PreviewWorkspace.cpp
    Preview/PreviewWorkspaceNavigation.cpp
    Preview/PreviewCommandPanel.cpp
    Preview/PreviewFavoritesPersistence.cpp
    Preview/PreviewPreferences.cpp
    Preview/PreviewRenderer.cpp
    Preview/ShipGeneratorPreviewApp.cpp
    Preview/ShipGenerator_main.cpp
)
target_include_directories(PixelShipGenerator PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Preview)
target_link_libraries(PixelShipGenerator PRIVATE
    PixelShipGeneratorCore PixelShipGeneratorDiagnosticsCore PixelShipGeneratorApplicationCommon sfml-graphics
)
target_compile_features(PixelShipGenerator PRIVATE cxx_std_17)

add_executable(PixelShipGeneratorDiagnostics DiagnosticsApp/DiagnosticsApp.cpp DiagnosticsApp/DiagnosticsApp_main.cpp)
target_include_directories(PixelShipGeneratorDiagnostics PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/DiagnosticsApp)
target_link_libraries(PixelShipGeneratorDiagnostics PRIVATE
    PixelShipGeneratorDiagnosticsAppSupport PixelShipGeneratorApplicationCommon sfml-graphics
)
target_compile_features(PixelShipGeneratorDiagnostics PRIVATE cxx_std_17)
target_compile_definitions(PixelShipGeneratorDiagnostics PRIVATE PIXEL_SHIP_GENERATOR_BUILD_CONFIGURATION="${CMAKE_BUILD_TYPE}")

include(CheckIncludeFileCXX)
check_include_file_cxx("SFML/Graphics.hpp" PIXEL_SHIP_GENERATOR_HAS_SFML_GRAPHICS_HEADER)
if(TARGET sfml-graphics OR PIXEL_SHIP_GENERATOR_HAS_SFML_GRAPHICS_HEADER)
    set(PIXEL_SHIP_GENERATOR_PREVIEW_REGRESSION_HAS_SFML ON)
else()
    set(PIXEL_SHIP_GENERATOR_PREVIEW_REGRESSION_HAS_SFML OFF)
endif()

set(PIXEL_SHIP_GENERATOR_PREVIEW_REGRESSION_SOURCES
    Tests/PixelShipGeneratorPreviewRegression_main.cpp
    Tests/RegressionRunner.cpp
    Tests/PreviewRegressionSuites.cpp
    Tests/GenerationCalibrationRegression.cpp
    Tests/PreviewPreferencesRegression.cpp
    Tests/PreviewFavoritesPersistenceRegression.cpp
    Tests/PreviewFavoritesBrowserRegression.cpp
    Tests/PreviewUserPresetPersistenceRegression.cpp
    Tests/PreviewConfigurationBundleRegression.cpp
    Tests/PreviewSessionRegression.cpp
    Tests/PreviewWorkspaceRegression.cpp
    Tests/PreviewInspectionRegression.cpp
    Tests/PreviewAnimationLabRegression.cpp
    Tests/PreviewUiQolRegression.cpp
    Tests/DiagnosticsAppRegression.cpp
    Tests/PreviewConfigurationEditorRegression.cpp
    Tests/PreviewFactionProfileEditorRegression.cpp
    Tests/PreviewPaletteEditorRegression.cpp

    Preview/GenerationCalibration.cpp
    Preview/GenerationCalibrationSerializer.cpp
    Preview/ConfigurationEditorControls.cpp
    Preview/ConfigurationBundle.cpp
    Preview/PreviewConfigurationEditor.cpp
    Preview/ShipGenerationProfileEditorBindings.cpp
    Preview/ShipFactionProfileEditorBindings.cpp
    Preview/ShipPaletteConfigurationEditorBindings.cpp
    Preview/RuntimeCustomPresetWorkspace.cpp
    Preview/UserPresetPersistence.cpp
    Preview/StructuralProfileSelection.cpp
    Preview/FactionProfileSelection.cpp
    Preview/PaletteProfileSelection.cpp
    Preview/PreviewCommand.cpp
    Preview/PreviewInspection.cpp
    Preview/PreviewWorkspace.cpp
    Preview/PreviewAnimationSession.cpp
    Preview/PreviewCollectionSession.cpp
    Preview/PreviewFavoritesPersistence.cpp
    Preview/PreviewPreferences.cpp
)
if(PIXEL_SHIP_GENERATOR_PREVIEW_REGRESSION_HAS_SFML)
    list(APPEND PIXEL_SHIP_GENERATOR_PREVIEW_REGRESSION_SOURCES
        Tests/AttributeRerollStudioRegression.cpp
        Preview/AttributeRerollStudio.cpp
        Preview/PreviewCommandPanel.cpp
    )
endif()

add_executable(PixelShipGeneratorPreviewRegression ${PIXEL_SHIP_GENERATOR_PREVIEW_REGRESSION_SOURCES})
target_include_directories(PixelShipGeneratorPreviewRegression PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/Tests
    ${CMAKE_CURRENT_SOURCE_DIR}/Preview
)
target_link_libraries(PixelShipGeneratorPreviewRegression PRIVATE
    PixelShipGeneratorCore PixelShipGeneratorDiagnosticsCore PixelShipGeneratorDiagnosticsAppSupport
)
target_compile_features(PixelShipGeneratorPreviewRegression PRIVATE cxx_std_17)
if(PIXEL_SHIP_GENERATOR_PREVIEW_REGRESSION_HAS_SFML)
    target_link_libraries(PixelShipGeneratorPreviewRegression PRIVATE sfml-graphics)
    target_compile_definitions(PixelShipGeneratorPreviewRegression PRIVATE PIXEL_SHIP_GENERATOR_PREVIEW_HAS_SFML=1)
else()
    target_compile_definitions(PixelShipGeneratorPreviewRegression PRIVATE PIXEL_SHIP_GENERATOR_PREVIEW_HAS_SFML=0)
    message(STATUS "SFML graphics headers unavailable: Attribute Reroll Studio Preview regression is not registered in this build.")
endif()

if(BUILD_TESTING)
    set(PIXEL_SHIP_GENERATOR_PREVIEW_NORMAL_SUITES
        preview-preferences preview-favorites favorites-browser user-presets configuration-bundles
        preview-session preview-workspaces preview-inspection animation-lab preview-ui-qol diagnostics-app
        configuration-editor faction-profile-editor palette-editor
    )
    if(PIXEL_SHIP_GENERATOR_PREVIEW_REGRESSION_HAS_SFML)
        list(APPEND PIXEL_SHIP_GENERATOR_PREVIEW_NORMAL_SUITES attribute-reroll-studio)
    endif()
    set(PIXEL_SHIP_GENERATOR_PREVIEW_LONG_SUITES calibration)
    foreach(SUITE_NAME IN LISTS PIXEL_SHIP_GENERATOR_PREVIEW_NORMAL_SUITES)
        add_test(NAME preview.${SUITE_NAME} COMMAND PixelShipGeneratorPreviewRegression --suite ${SUITE_NAME})
        set_tests_properties(preview.${SUITE_NAME} PROPERTIES LABELS "preview;normal")
    endforeach()
    foreach(SUITE_NAME IN LISTS PIXEL_SHIP_GENERATOR_PREVIEW_LONG_SUITES)
        add_test(NAME preview.${SUITE_NAME} COMMAND PixelShipGeneratorPreviewRegression --suite ${SUITE_NAME})
        set_tests_properties(preview.${SUITE_NAME} PROPERTIES LABELS "preview;long")
    endforeach()
endif()
