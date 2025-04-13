set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_library(
  utils SHARED ""
  )
# install(TARGETS utils
#     LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

target_link_libraries( utils
  PUBLIC
    Qt6::Core
    Qt6::Widgets
    KF6::I18n
    KF6::CoreAddons
    KF6::Crash
    KF6::IconThemes
    KF6::TextEditor
    KF6::WindowSystem
    KF6::KIOGui
    KF6::KIOWidgets
    )

target_sources( utils
  PUBLIC
  ktexteditor_utils.cpp
  hostprocess.cpp
  quickdialog.cpp
  diagnostics/diagnosticview.cpp
  texthint/KateTextHintManager.cpp
  texthint/tooltip.cpp
  texthint/hintview.cpp
  texthint/hintstate.cpp
  # PUBLIC FILE_SET HEADERS
  # FILES
  # hostprocess.h
  # quickdialog.h
  # drawing_utils.h
)

target_include_directories(
  utils
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    # hostprocess.h
    # quickdialog.h
    # drawing_utils.h
    ${CMAKE_CURRENT_SOURCE_DIR}/texthint
    ${CMAKE_CURRENT_SOURCE_DIR}/diagnostics
    # texthint.h
)

