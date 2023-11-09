enable_asan_if_debug()

foreach(dir kext chsy-format chsy-parse chsy-strip chsy-link chsy-host-rewrite chsy-c-back chsy-lower chsy-get cscc)
    add_subdirectory("${dir}")
endforeach()
