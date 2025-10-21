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
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/FindJulia.cmake")
set(Julia_PREFIX "$HOME/.juliaup/bin/julia-1.11.3")

set(kdevjulialanguagesupport_PART_SRCS
    julialanguagesupport.cpp
    juliahighlighting.cpp
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
kdevplatform_add_plugin( kdevjulialanguagesupport
    JSON kdevjuliasupport.json
    SOURCES ${kdevjulialanguagesupport_PART_SRCS}
    )

target_link_libraries(kdevjulialanguagesupport
    KDev::Language
    KDev::Interfaces
    KDev::Util
    KF6::Service
    KF6::ThreadWeaver
    KF6::TextEditor
)

#add_subdirectory(lspclient)

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
