# GUI/Application-owned targets. The Library targets must already exist, either
# from the combined development tree or from an explicitly supplied checkout.
if(NOT TARGET SpectralShipGen::Core OR NOT TARGET SpectralShipGen::Diagnostics)
    message(FATAL_ERROR "SpectralShipGen Studio requires SpectralShipGen::Core and SpectralShipGen::Diagnostics targets from the Library checkout.")
endif()

find_package(Threads REQUIRED)

add_library(SpectralShipGenStudioDiagnosticsAppSupport STATIC DiagnosticsApp/DiagnosticsAppController.cpp)
target_include_directories(SpectralShipGenStudioDiagnosticsAppSupport PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/DiagnosticsApp)
target_link_libraries(SpectralShipGenStudioDiagnosticsAppSupport PUBLIC SpectralShipGen::Diagnostics Threads::Threads)
target_compile_features(SpectralShipGenStudioDiagnosticsAppSupport PUBLIC cxx_std_17)

add_library(SpectralShipGenStudioApplicationCommon STATIC
    Application/SFMLPixelText.cpp
    Application/SFMLCharts.cpp
    Application/SFMLImageAdapter.cpp
)
target_include_directories(SpectralShipGenStudioApplicationCommon PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/Application)
target_link_libraries(SpectralShipGenStudioApplicationCommon PUBLIC SpectralShipGen::Core sfml-graphics)
target_compile_features(SpectralShipGenStudioApplicationCommon PUBLIC cxx_std_17)

add_executable(SpectralShipGenStudio
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
target_include_directories(SpectralShipGenStudio PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Preview)
target_link_libraries(SpectralShipGenStudio PRIVATE
    SpectralShipGen::Core SpectralShipGen::Diagnostics SpectralShipGenStudioApplicationCommon sfml-graphics
)
target_compile_features(SpectralShipGenStudio PRIVATE cxx_std_17)

add_executable(SpectralShipGenStudioDiagnostics DiagnosticsApp/DiagnosticsApp.cpp DiagnosticsApp/DiagnosticsApp_main.cpp)
target_include_directories(SpectralShipGenStudioDiagnostics PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/DiagnosticsApp)
target_link_libraries(SpectralShipGenStudioDiagnostics PRIVATE
    SpectralShipGenStudioDiagnosticsAppSupport SpectralShipGenStudioApplicationCommon sfml-graphics
)
target_compile_features(SpectralShipGenStudioDiagnostics PRIVATE cxx_std_17)
target_compile_definitions(SpectralShipGenStudioDiagnostics PRIVATE SPECTRAL_SHIP_GEN_BUILD_CONFIGURATION="${CMAKE_BUILD_TYPE}")

include(CheckIncludeFileCXX)
check_include_file_cxx("SFML/Graphics.hpp" SPECTRAL_SHIP_GEN_HAS_SFML_GRAPHICS_HEADER)
if(TARGET sfml-graphics OR SPECTRAL_SHIP_GEN_HAS_SFML_GRAPHICS_HEADER)
    set(SPECTRAL_SHIP_GEN_PREVIEW_REGRESSION_HAS_SFML ON)
else()
    set(SPECTRAL_SHIP_GEN_PREVIEW_REGRESSION_HAS_SFML OFF)
endif()

set(SPECTRAL_SHIP_GEN_PREVIEW_REGRESSION_SOURCES
    Tests/SpectralShipGenStudioPreviewRegression_main.cpp
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
if(SPECTRAL_SHIP_GEN_PREVIEW_REGRESSION_HAS_SFML)
    list(APPEND SPECTRAL_SHIP_GEN_PREVIEW_REGRESSION_SOURCES
        Tests/AttributeRerollStudioRegression.cpp
        Preview/AttributeRerollStudio.cpp
        Preview/PreviewCommandPanel.cpp
    )
endif()

add_executable(SpectralShipGenStudioPreviewRegression ${SPECTRAL_SHIP_GEN_PREVIEW_REGRESSION_SOURCES})
target_include_directories(SpectralShipGenStudioPreviewRegression PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/Tests
    ${CMAKE_CURRENT_SOURCE_DIR}/Preview
)
target_link_libraries(SpectralShipGenStudioPreviewRegression PRIVATE
    SpectralShipGen::Core SpectralShipGen::Diagnostics SpectralShipGenStudioDiagnosticsAppSupport
)
target_compile_features(SpectralShipGenStudioPreviewRegression PRIVATE cxx_std_17)
if(SPECTRAL_SHIP_GEN_PREVIEW_REGRESSION_HAS_SFML)
    target_link_libraries(SpectralShipGenStudioPreviewRegression PRIVATE sfml-graphics)
    target_compile_definitions(SpectralShipGenStudioPreviewRegression PRIVATE SPECTRAL_SHIP_GEN_PREVIEW_HAS_SFML=1)
else()
    target_compile_definitions(SpectralShipGenStudioPreviewRegression PRIVATE SPECTRAL_SHIP_GEN_PREVIEW_HAS_SFML=0)
    message(STATUS "SFML graphics headers unavailable: Attribute Reroll Studio Preview regression is not registered in this build.")
endif()

if(BUILD_TESTING)
    set(SPECTRAL_SHIP_GEN_PREVIEW_NORMAL_SUITES
        preview-preferences preview-favorites favorites-browser user-presets configuration-bundles
        preview-session preview-workspaces preview-inspection animation-lab preview-ui-qol diagnostics-app
        configuration-editor faction-profile-editor palette-editor
    )
    if(SPECTRAL_SHIP_GEN_PREVIEW_REGRESSION_HAS_SFML)
        list(APPEND SPECTRAL_SHIP_GEN_PREVIEW_NORMAL_SUITES attribute-reroll-studio)
    endif()
    set(SPECTRAL_SHIP_GEN_PREVIEW_LONG_SUITES calibration)
    foreach(SUITE_NAME IN LISTS SPECTRAL_SHIP_GEN_PREVIEW_NORMAL_SUITES)
        add_test(NAME preview.${SUITE_NAME} COMMAND SpectralShipGenStudioPreviewRegression --suite ${SUITE_NAME})
        set_tests_properties(preview.${SUITE_NAME} PROPERTIES LABELS "preview;normal")
    endforeach()
    foreach(SUITE_NAME IN LISTS SPECTRAL_SHIP_GEN_PREVIEW_LONG_SUITES)
        add_test(NAME preview.${SUITE_NAME} COMMAND SpectralShipGenStudioPreviewRegression --suite ${SUITE_NAME})
        set_tests_properties(preview.${SUITE_NAME} PROPERTIES LABELS "preview;long")
    endforeach()
endif()
