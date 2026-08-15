set(VCPKG_TARGET_ARCHITECTURE x64)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE static)

# The vcpkg glad port defaults to the compatibility profile. DentalViz requests
# an OpenGL 3.3 Core context, so generate only the matching core entry points.
set(GLAD_PROFILE core)
