set(kdev-julia_SRCS
    kdev-julia.cpp
)

ecm_qt_declare_logging_category(kdev-julia_SRCS
    HEADER debug.h
    IDENTIFIER PLUGIN_KDEV_JULIA
    CATEGORY_NAME "kdevelop.plugins.kdev-julia"
    DESCRIPTION "KDevelop plugin: kdev-julia"
    EXPORT KDEV_JULIA
)

kdevplatform_add_plugin(kdev-julia
    SOURCES ${kdev-julia_SRCS}
)

target_link_libraries(kdev-julia
    KDev::Interfaces
)
