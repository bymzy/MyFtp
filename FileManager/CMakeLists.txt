aux_source_directory(./ SOURCE_FILES)
add_library(FileManager ${SOURCE_FILES})
#add_dependencies(FileManager libssl.a)
