# Absolute desktop destinations share DESTDIR with the private /opt payload.
# Runtime.cmake registers the prefix/DESTDIR guard before any file is installed.
install(FILES "${CMAKE_CURRENT_LIST_DIR}/jobtracker.desktop"
    DESTINATION "/usr/share/applications"
)

foreach(icon_size IN ITEMS 16 24 32 48 64 128 256)
    install(FILES "${CMAKE_CURRENT_LIST_DIR}/icons/${icon_size}x${icon_size}/jobtracker.png"
        DESTINATION "/usr/share/icons/hicolor/${icon_size}x${icon_size}/apps"
    )
endforeach()
