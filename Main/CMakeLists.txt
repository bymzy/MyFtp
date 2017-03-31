aux_source_directory(./ SOURCE_FILES)
add_executable(MyFtp ${SOURCE_FILES})
add_dependencies(MyFtp FileManager)
target_link_libraries(MyFtp pthread FileManager)
