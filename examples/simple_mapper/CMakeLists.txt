cmake_minimum_required( VERSION 3.15 )

project( simple_mapper )

set( SOURCES
    src/simple_mapper.cpp
)

add_executable( simple_mapper ${SOURCES} )
target_link_libraries( simple_mapper PRIVATE LibTokaMap::libtokamap )

add_library( custom_library SHARED custom_library/custom_library.cpp )
target_link_libraries( custom_library PRIVATE LibTokaMap::libtokamap )

add_library( json_data_source SHARED data_source_library/json_data_source.cpp )
target_link_libraries( json_data_source PRIVATE LibTokaMap::libtokamap )
