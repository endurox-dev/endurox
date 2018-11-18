
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/glimpse_index/glim.in
    ${CMAKE_CURRENT_SOURCE_DIR}/glimpse_index/glim
    @ONLY
    )

file(
    COPY ${CMAKE_CURRENT_SOURCE_DIR}/glimpse_index/glim
    DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}
    FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
    )

