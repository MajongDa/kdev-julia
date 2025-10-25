cmake_minimum_required(VERSION 3.23)

project(kdev-julia)
set(CMAKE_CXX_STANDARD 20)
set(KDE_COMPILERSETTINGS_LEVEL 6.0)
set(QT_MIN_VERSION "6.5.0")
set(KF_MIN_VERSION "6.0.0")

find_package(ECM ${KF_MIN_VERSION} REQUIRED)


set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(ECMQtDeclareLoggingCategory)
include(FeatureSummary)
include(ECMOptionalAddSubdirectory)
include(ECMAddAppIcon)
include(ECMInstallIcons)
include(ECMDeprecationSettings)

find_package(KDevPlatform REQUIRED)
find_package(KDevelop REQUIRED)


find_package(ECM ${KF_MIN_VERSION} REQUIRED)
find_package(Qt6 ${QT_MIN_VERSION} CONFIG REQUIRED Core Widgets Test)

find_package(KF6 ${KF_MIN_VERSION} REQUIRED COMPONENTS
    Config
    CoreAddons
    I18n
    Parts
    Service
    TextEditor
    ThreadWeaver
    WidgetsAddons
    XmlGui
)

#Julia part
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}")
find_package(Julia REQUIRED)
configure_file(kdevjuliaversion.h.cmake "${CMAKE_CURRENT_BINARY_DIR}/kdevjuliaversion.h" @ONLY)

set(kdevjulialanguagesupport_PART_SRCS
    julialanguagesupport.cpp
    juliahighlighting.cpp
    projectconfig/projectconfigpage.cpp
    juliaexecutionlauncher.cpp
    juliaexecutionjob.cpp
    graphicsoutputview.cpp
    graphicstoolviewfactory.cpp
)


ecm_qt_declare_logging_category(kdevjulialanguagesupport_PART_SRCS
    HEADER juliadebug.h
    EXPORT KDEV_JULIA
    IDENTIFIER KDEV_JULIA
    #FILE kdev-julia.categories
    CATEGORY_NAME "kdevelop.plugins.julia"
    DESCRIPTION "KDevelop plugin: Julia language support"
    EXPORT KDEVJULIA
)
ki18n_wrap_ui(kdevjulialanguagesupport_PART_SRCS
    projectconfig/projectconfig.ui
)

kdevplatform_add_plugin( kdevjulialanguagesupport
    JSON kdevjuliasupport.json
    SOURCES ${kdevjulialanguagesupport_PART_SRCS}
    )

target_link_libraries(kdevjulialanguagesupport
    KDev::Language
    KDev::Interfaces
    KDev::Util
    KDev::OutputView
    KF6::Service
    KF6::ThreadWeaver
    KF6::TextEditor
)

#add_subdirectory(lspclient)
install(FILES juliastartupscript.jl DESTINATION ${KDE_INSTALL_DATADIR}/kdevjuliasupport)
install(FILES graphics/FilePlotDisplay.jl DESTINATION ${KDE_INSTALL_DATADIR}/kdevjuliasupport/graphics)
install(FILES graphics/SocketPlotDisplay.jl DESTINATION ${KDE_INSTALL_DATADIR}/kdevjuliasupport/graphics)

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
