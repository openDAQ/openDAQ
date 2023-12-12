set(USE_SYSTEM_TZ_DB ON CACHE BOOL "Use the operating system's timezone database" FORCE)

opendaq_dependency(
    NAME                date
    REQUIRED_VERSION    3.0.1
    GIT_REPOSITORY      https://github.com/HowardHinnant/date
    GIT_REF             cc4685a
    GIT_SHALLOW         OFF
    EXPECT_TARGET       date::date
    FETCH_LOG_LEVEL     WARNING
)
