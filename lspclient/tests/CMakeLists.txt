add_executable(lsptestapp "")
target_include_directories(lsptestapp PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/..)
target_link_libraries(lsptestapp PRIVATE KF6::TextEditor)

target_sources(
  lsptestapp
  PRIVATE
    lsptestapp.cpp
    ../lspclientserver.cpp
    ../lspsemantichighlighting.cpp
    ../semantic_tokens_legend.cpp
    ${DEBUG_SOURCES}
)

target_link_directories(
  lsptestapp
  PRIVATE
    ${PROJECT_SOURCE_DIR}/lspclient/lib
    ${PROJECT_SOURCE_DIR}/lspclient )

# add_subdirectory(${PROJECT_SOURCE_DIR}/lspclient/lib)

target_link_libraries(
  lsptestapp
  PRIVATE
    utils
    lspclient
    Qt6::Core
    Qt6::Widgets
    KF6::CoreAddons
    KF6::Crash
    KF6::I18n
    KF6::IconThemes
    KF6::TextEditor
    KF6::WindowSystem
    KF6::KIOGui
    KF6::KIOWidgets
    KF6::WidgetsAddons
)
if(ENABLE_PCH)
    target_precompile_headers(lsptestapp REUSE_FROM katepch_tests)
endif()
