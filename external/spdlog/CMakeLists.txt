set(SPDLOG_INSTALL ON CACHE BOOL "" FORCE)
set(SPDLOG_FMT_EXTERNAL ON CACHE BOOL "" FORCE)
set(SPDLOG_WCHAR_FILENAMES ON CACHE BOOL "" FORCE)

opendaq_dependency(
    NAME                spdlog
    REQUIRED_VERSION    1.17.0
    GIT_REPOSITORY      https://github.com/gabime/spdlog.git
    GIT_REF             v1.17.0
    EXPECT_TARGET       spdlog::spdlog
    PATCH_FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/patches/001-periodic_worker_init_func.patch
)

# Disable spdlog's Mapped Diagnostic Context (MDC). openDAQ never uses MDC, and in async
# mode spdlog's `full_formatter`/`mdc_formatter` call `mdc::get_context()` on the logger
# worker thread, instantiating a `thread_local std::map` there. On mingw-w64 that thread-local
# is destroyed (via `run_dtor_list`) against already-freed heap during process/thread shutdown,
# crashing on thread exit (observable when a debugger forces orderly per-thread TLS teardown).
# `SPDLOG_NO_TLS` compiles out the thread-local entirely. Set PUBLIC so spdlog and every
# consumer that inlines spdlog headers agree, avoiding an ODR mismatch.
if (TARGET spdlog)
    target_compile_definitions(spdlog PUBLIC SPDLOG_NO_TLS)
endif()
if (TARGET spdlog_header_only)
    target_compile_definitions(spdlog_header_only INTERFACE SPDLOG_NO_TLS)
endif()
