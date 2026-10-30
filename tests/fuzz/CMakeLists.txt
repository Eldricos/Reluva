# Fuzz targets for the readers of untrusted data: each .cpp here but
# replay.cpp is one (fuzz.h). In the normal test run reluva_fuzz_replay runs
# them all over their corpora (corpus/<target>/: seeds made from the tests'
# builders by the seeds_* programs, and inputs the fuzzers once crashed
# on). With -DRELUVA_FUZZ=ON (clang) each is also a libFuzzer program,
# reluva_fuzz_<target>, the code built with address and undefined
# behaviour sanitizers:
#   reluva_fuzz_geo -max_total_time=300 new_inputs/ tests/fuzz/corpus/geo
set(RELUVA_FUZZ_TARGETS bink sound savestate cat_archive string_table gmf geo mdl fif texture levels arf mission_state)

if(RELUVA_BUILD_TESTS)
    set(replay_sources replay.cpp)
    foreach(t IN LISTS RELUVA_FUZZ_TARGETS)
        list(APPEND replay_sources ${t}.cpp)
    endforeach()
    add_executable(reluva_fuzz_replay ${replay_sources})
    target_compile_definitions(reluva_fuzz_replay PRIVATE RELUVA_FUZZ_REPLAY)
    target_link_libraries(reluva_fuzz_replay PRIVATE reluva_engines reluva_warnings)
    add_test(NAME fuzz_replay COMMAND reluva_fuzz_replay ${CMAKE_CURRENT_SOURCE_DIR}/corpus)
endif()

if(RELUVA_FUZZ)
    foreach(t IN LISTS RELUVA_FUZZ_TARGETS)
        add_executable(reluva_fuzz_${t} ${t}.cpp)
        target_link_libraries(reluva_fuzz_${t} PRIVATE reluva_engines reluva_warnings)
        target_link_options(reluva_fuzz_${t} PRIVATE -fsanitize=fuzzer)
    endforeach()
    # The seed corpora's makers (run by hand, into corpus/): the tests'
    # own builders, their files compiled in with their main() renamed.
    foreach(s common newworlds)
        add_executable(reluva_fuzz_seeds_${s} seeds_${s}.cpp)
        target_link_libraries(reluva_fuzz_seeds_${s} PRIVATE reluva_engines)
        target_include_directories(reluva_fuzz_seeds_${s} PRIVATE ${PROJECT_SOURCE_DIR}/tests)
        target_compile_options(reluva_fuzz_seeds_${s} PRIVATE -Wno-unused-function)
    endforeach()
endif()
