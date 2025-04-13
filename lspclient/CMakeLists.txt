# ki18n_wrap_ui(lspclientplugin lspconfigwidget.ui)
# set (lspclient_SRC
#     lspclientcompletion.cpp
#     lspclientconfigpage.cpp
#     lspclienthover.cpp
#     lspclientplugin.cpp
#     lspclientpluginview.cpp
#     lspclientserver.cpp
#     lspclientservermanager.cpp
#     lspclientsymbolview.cpp
#     lspclientutils.cpp
#     lspsemantichighlighting.cpp
#     semantic_tokens_legend.cpp
#     gotosymboldialog.cpp
#     inlayhints.cpp
#
#     lspconfigwidget.ui
# )
#
# set(CMAKE_AUTOMOC ON)
# set(CMAKE_AUTORCC ON)
# set(CMAKE_AUTOUIC ON)
#
# include(ECMQtDeclareLoggingCategory)
#
# ecm_qt_declare_logging_category(
#   DEBUG_SOURCES
#   HEADER lspclient_debug.h
#   IDENTIFIER LSPCLIENT
#   CATEGORY_NAME "lspclientplugin_DEBUG"
# )
#
#


# set(CMAKE_AUTOMOC ON)
# set(CMAKE_AUTOUIC ON)
# set(CMAKE_INCLUDE_CURRENT_DIR ON)
#
# find_package(KF6 ${KF_DEP_VERSION}
#     REQUIRED COMPONENTS
#       I18n
#       CoreAddons
#       GuiAddons
#       Crash
#       IconThemes
#       TextEditor
#       DocTools
#       TextWidgets
#
# )
#
# ki18n_wrap_ui(lspclientplugin lspconfigwidget.ui)
#
#
# add_library ( lspclient ${lspclient_SRC} )
# target_compile_features(lspclient PRIVATE cxx_std_17)
#
#
# if(BUILD_TESTING)
#   add_subdirectory(tests)
# endif()
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(KF6 ${KF_DEP_VERSION}
    REQUIRED COMPONENTS
      I18n
      CoreAddons
      GuiAddons
      Crash
      IconThemes
      TextEditor
      DocTools
      TextWidgets
)
find_package(RapidJSON)



add_library(lspclient)
target_include_directories(
  lspclient
  PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
  # ${CMAKE_CURRENT_SOURCE_DIR}/lib # hostprocess.h etc
    #
    # lspclientcompletion.h
    # lspclientconfigpage.h
    # lspclienthover.h
    # lspclientplugin.h
    # lspclientpluginview.h
    # lspclientserver.h
    # lspclientservermanager.h
    # lspclientsymbolview.h
    # lspclientutils.h
    # lspsemantichighlighting.h
    # semantic_tokens_legend.h
    # gotosymboldialog.h
    # inlayhints.h
    #
    # diagnostic_types.h
)

target_link_directories(
  lspclient
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/
)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/lib/)

target_link_libraries( lspclient
    PUBLIC
      utils
)

target_link_libraries( lspclient
    PRIVATE
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
target_sources(
  lspclient
  PUBLIC
    lspclientcompletion.cpp
    lspclientconfigpage.cpp
    lspclienthover.cpp
    lspclientplugin.cpp
    lspclientpluginview.cpp
    lspclientserver.cpp
    lspclientservermanager.cpp
    lspclientsymbolview.cpp
    lspclientutils.cpp
    lspsemantichighlighting.cpp
    semantic_tokens_legend.cpp
    gotosymboldialog.cpp
    inlayhints.cpp

    ${CMAKE_CURRENT_SOURCE_DIR}/lspconfigwidget.ui

    plugin.qrc
)


include(ECMQtDeclareLoggingCategory)
ecm_qt_declare_logging_category(
  DEBUG_SOURCES
  HEADER lspclient_debug.h
  IDENTIFIER LSPCLIENT
  CATEGORY_NAME "katelspclientplugin"
)
target_sources(lspclient PRIVATE ${DEBUG_SOURCES})

ki18n_wrap_ui(lspclient lspconfigwidget.ui)

if(BUILD_TESTING)
  add_subdirectory(tests)
endif()

