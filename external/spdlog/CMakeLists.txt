set(SPDLOG_INSTALL ON CACHE BOOL "" FORCE)
set(SPDLOG_FMT_EXTERNAL ON CACHE BOOL "" FORCE)
set(SPDLOG_WCHAR_FILENAMES ON CACHE BOOL "" FORCE)

opendaq_dependency(
    NAME                spdlog
    REQUIRED_VERSION    1.8.0
    GIT_REPOSITORY      https://github.com/gabime/spdlog.git
    GIT_REF             v1.8.0
    EXPECT_TARGET       spdlog::spdlog
)
