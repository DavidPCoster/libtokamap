# PackagingUtils.cmake
# Utility functions and macros for packaging LibTokaMap and related projects

# Function to configure basic CPack settings for a project
function(configure_basic_packaging)
    cmake_parse_arguments(
        ARGS
        ""
        "NAME;VERSION;DESCRIPTION;CONTACT;URL;LICENSE_FILE;README_FILE"
        "DEPENDENCIES_DEB;DEPENDENCIES_RPM"
        ${ARGN}
    )

    # Set required arguments with defaults
    if(NOT ARGS_NAME)
        set(ARGS_NAME ${PROJECT_NAME})
    endif()

    if(NOT ARGS_VERSION)
        set(ARGS_VERSION ${PROJECT_VERSION})
    endif()

    if(NOT ARGS_DESCRIPTION)
        set(ARGS_DESCRIPTION "Package for ${ARGS_NAME}")
    endif()

    if(NOT ARGS_CONTACT)
        set(ARGS_CONTACT "maintainer@example.org")
    endif()

    # Basic CPack configuration
    set(CPACK_PACKAGE_NAME "${ARGS_NAME}" PARENT_SCOPE)
    set(CPACK_PACKAGE_VERSION "${ARGS_VERSION}" PARENT_SCOPE)
    set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${ARGS_DESCRIPTION}" PARENT_SCOPE)
    set(CPACK_PACKAGE_CONTACT "${ARGS_CONTACT}" PARENT_SCOPE)

    if(ARGS_URL)
        set(CPACK_PACKAGE_HOMEPAGE_URL "${ARGS_URL}" PARENT_SCOPE)
    endif()

    # Documentation files
    if(ARGS_README_FILE AND EXISTS "${ARGS_README_FILE}")
        set(CPACK_PACKAGE_DESCRIPTION_FILE "${ARGS_README_FILE}" PARENT_SCOPE)
    endif()

    if(ARGS_LICENSE_FILE AND EXISTS "${ARGS_LICENSE_FILE}")
        set(CPACK_RESOURCE_FILE_LICENSE "${ARGS_LICENSE_FILE}" PARENT_SCOPE)
    endif()

    # Version components
    string(REPLACE "." ";" VERSION_LIST "${ARGS_VERSION}")
    list(LENGTH VERSION_LIST VERSION_COMPONENTS)

    if(VERSION_COMPONENTS GREATER 0)
        list(GET VERSION_LIST 0 VERSION_MAJOR)
        set(CPACK_PACKAGE_VERSION_MAJOR "${VERSION_MAJOR}" PARENT_SCOPE)
    endif()

    if(VERSION_COMPONENTS GREATER 1)
        list(GET VERSION_LIST 1 VERSION_MINOR)
        set(CPACK_PACKAGE_VERSION_MINOR "${VERSION_MINOR}" PARENT_SCOPE)
    endif()

    if(VERSION_COMPONENTS GREATER 2)
        list(GET VERSION_LIST 2 VERSION_PATCH)
        set(CPACK_PACKAGE_VERSION_PATCH "${VERSION_PATCH}" PARENT_SCOPE)
    endif()

    # Platform-specific configuration
    configure_platform_packaging("${ARGS_DEPENDENCIES_DEB}" "${ARGS_DEPENDENCIES_RPM}")
endfunction()

# Internal function to configure platform-specific packaging
function(configure_platform_packaging DEPENDENCIES_DEB DEPENDENCIES_RPM)
    if(APPLE)
        configure_macos_packaging()
    elseif(UNIX AND NOT APPLE)
        configure_linux_packaging("${DEPENDENCIES_DEB}" "${DEPENDENCIES_RPM}")
    elseif(WIN32)
        configure_windows_packaging()
    endif()
endfunction()

# Configure macOS packaging
function(configure_macos_packaging)
    set(CPACK_GENERATOR "TGZ;ZIP" PARENT_SCOPE)
    set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-macOS-${CMAKE_SYSTEM_PROCESSOR}" PARENT_SCOPE)
    set(CPACK_ARCHIVE_COMPONENT_INSTALL ON PARENT_SCOPE)

    # Optional productbuild configuration (requires signing certificates)
    # list(APPEND CPACK_GENERATOR "productbuild")
    # set(CPACK_PRODUCTBUILD_IDENTITY_NAME "Developer ID Installer: Your Name" PARENT_SCOPE)
endfunction()

# Configure Linux packaging
function(configure_linux_packaging DEPENDENCIES_DEB DEPENDENCIES_RPM)
    set(CPACK_GENERATOR "DEB;RPM;TGZ" PARENT_SCOPE)

    # Debian/Ubuntu package settings
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "${CPACK_PACKAGE_CONTACT}" PARENT_SCOPE)
    set(CPACK_DEBIAN_PACKAGE_SECTION "libs" PARENT_SCOPE)
    set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional" PARENT_SCOPE)
    set(CPACK_DEBIAN_FILE_NAME "DEB-DEFAULT" PARENT_SCOPE)
    set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON PARENT_SCOPE)

    if(DEPENDENCIES_DEB)
        set(CPACK_DEBIAN_PACKAGE_DEPENDS "${DEPENDENCIES_DEB}" PARENT_SCOPE)
    endif()

    # RPM package settings
    set(CPACK_RPM_PACKAGE_GROUP "System/Libraries" PARENT_SCOPE)
    set(CPACK_RPM_PACKAGE_LICENSE "MIT" PARENT_SCOPE)
    set(CPACK_RPM_FILE_NAME "RPM-DEFAULT" PARENT_SCOPE)
    set(CPACK_RPM_PACKAGE_AUTOREQPROV ON PARENT_SCOPE)

    if(DEPENDENCIES_RPM)
        set(CPACK_RPM_PACKAGE_REQUIRES "${DEPENDENCIES_RPM}" PARENT_SCOPE)
    endif()

    if(CPACK_PACKAGE_HOMEPAGE_URL)
        set(CPACK_RPM_PACKAGE_URL "${CPACK_PACKAGE_HOMEPAGE_URL}" PARENT_SCOPE)
    endif()

    # Set architecture
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "amd64" PARENT_SCOPE)
        set(CPACK_RPM_PACKAGE_ARCHITECTURE "x86_64" PARENT_SCOPE)
    else()
        set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "i386" PARENT_SCOPE)
        set(CPACK_RPM_PACKAGE_ARCHITECTURE "i386" PARENT_SCOPE)
    endif()

    set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-Linux-${CMAKE_SYSTEM_PROCESSOR}" PARENT_SCOPE)
endfunction()

# Configure Windows packaging
function(configure_windows_packaging)
    set(CPACK_GENERATOR "ZIP;NSIS" PARENT_SCOPE)
    set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-Windows-${CMAKE_SYSTEM_PROCESSOR}" PARENT_SCOPE)

    # NSIS specific settings
    set(CPACK_NSIS_DISPLAY_NAME "${CPACK_PACKAGE_NAME}" PARENT_SCOPE)
    set(CPACK_NSIS_PACKAGE_NAME "${CPACK_PACKAGE_NAME}" PARENT_SCOPE)
    set(CPACK_NSIS_MODIFY_PATH ON PARENT_SCOPE)
endfunction()

# Function to set up component-based installation
function(configure_components)
    cmake_parse_arguments(
        ARGS
        ""
        ""
        "RUNTIME_COMPONENTS;DEVELOPMENT_COMPONENTS"
        ${ARGN}
    )

    # Default components
    set(ALL_COMPONENTS "runtime;development")

    if(ARGS_RUNTIME_COMPONENTS)
        list(APPEND ALL_COMPONENTS ${ARGS_RUNTIME_COMPONENTS})
    endif()

    if(ARGS_DEVELOPMENT_COMPONENTS)
        list(APPEND ALL_COMPONENTS ${ARGS_DEVELOPMENT_COMPONENTS})
    endif()

    list(REMOVE_DUPLICATES ALL_COMPONENTS)

    set(CPACK_COMPONENTS_ALL "${ALL_COMPONENTS}" PARENT_SCOPE)

    # Component descriptions
    set(CPACK_COMPONENT_RUNTIME_DISPLAY_NAME "Runtime Libraries" PARENT_SCOPE)
    set(CPACK_COMPONENT_RUNTIME_DESCRIPTION "Runtime libraries and executables" PARENT_SCOPE)

    set(CPACK_COMPONENT_DEVELOPMENT_DISPLAY_NAME "Development Files" PARENT_SCOPE)
    set(CPACK_COMPONENT_DEVELOPMENT_DESCRIPTION "Headers, CMake files, and development tools" PARENT_SCOPE)
    set(CPACK_COMPONENT_DEVELOPMENT_DEPENDS "runtime" PARENT_SCOPE)
endfunction()

# Function to configure source packaging
function(configure_source_packaging)
    cmake_parse_arguments(
        ARGS
        ""
        "NAME;VERSION"
        "IGNORE_PATTERNS"
        ${ARGN}
    )

    if(NOT ARGS_NAME)
        set(ARGS_NAME ${PROJECT_NAME})
    endif()

    if(NOT ARGS_VERSION)
        set(ARGS_VERSION ${PROJECT_VERSION})
    endif()

    set(CPACK_SOURCE_GENERATOR "TGZ;ZIP" PARENT_SCOPE)
    set(CPACK_SOURCE_PACKAGE_FILE_NAME "${ARGS_NAME}-${ARGS_VERSION}-source" PARENT_SCOPE)

    # Default ignore patterns
    set(DEFAULT_IGNORE_PATTERNS
        "/build/"
        "/\\.git/"
        "/\\.github/"
        "/\\.vscode/"
        "/\\.idea/"
        "\\.DS_Store"
        "\\.gitignore"
        "\\.clang-format"
        "~$"
        "\\.swp$"
        "\\.swo$"
        "/CMakeFiles/"
        "/CMakeCache.txt"
        "/_CPack_Packages/"
        "\\.tar\\.gz$"
        "\\.zip$"
        "\\.deb$"
        "\\.rpm$"
    )

    if(ARGS_IGNORE_PATTERNS)
        list(APPEND DEFAULT_IGNORE_PATTERNS ${ARGS_IGNORE_PATTERNS})
    endif()

    set(CPACK_SOURCE_IGNORE_FILES "${DEFAULT_IGNORE_PATTERNS}" PARENT_SCOPE)
endfunction()

# Function to install documentation files
function(install_documentation)
    cmake_parse_arguments(
        ARGS
        ""
        "DESTINATION;COMPONENT"
        "FILES"
        ${ARGN}
    )

    if(NOT ARGS_DESTINATION)
        set(ARGS_DESTINATION "share/doc/${PROJECT_NAME}")
    endif()

    if(NOT ARGS_COMPONENT)
        set(ARGS_COMPONENT "development")
    endif()

    # Install provided files
    if(ARGS_FILES)
        foreach(file ${ARGS_FILES})
            if(EXISTS "${file}")
                install(FILES "${file}"
                    DESTINATION "${ARGS_DESTINATION}"
                    COMPONENT "${ARGS_COMPONENT}"
                )
            endif()
        endforeach()
    endif()

    # Install common documentation files if they exist
    set(COMMON_DOC_FILES
        "${CMAKE_SOURCE_DIR}/README.md"
        "${CMAKE_SOURCE_DIR}/README"
        "${CMAKE_SOURCE_DIR}/LICENSE"
        "${CMAKE_SOURCE_DIR}/CHANGELOG.md"
        "${CMAKE_SOURCE_DIR}/AUTHORS"
        "${CMAKE_CURRENT_SOURCE_DIR}/README.md"
        "${CMAKE_CURRENT_SOURCE_DIR}/README"
        "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE"
    )

    foreach(file ${COMMON_DOC_FILES})
        if(EXISTS "${file}")
            get_filename_component(filename "${file}" NAME)
            install(FILES "${file}"
                DESTINATION "${ARGS_DESTINATION}"
                COMPONENT "${ARGS_COMPONENT}"
            )
        endif()
    endforeach()

    # Create version file
    write_file("${CMAKE_CURRENT_BINARY_DIR}/VERSION" "${PROJECT_VERSION}")
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/VERSION"
        DESTINATION "${ARGS_DESTINATION}"
        COMPONENT "${ARGS_COMPONENT}"
    )
endfunction()

# Macro to finalize packaging configuration
macro(finalize_packaging)
    # Common settings
    set(CPACK_PACKAGE_INSTALL_DIRECTORY "${PROJECT_NAME}")
    set(CPACK_STRIP_FILES ON)
    set(CPACK_SOURCE_STRIP_FILES ON)

    # Include CPack
    include(CPack)
endmacro()
