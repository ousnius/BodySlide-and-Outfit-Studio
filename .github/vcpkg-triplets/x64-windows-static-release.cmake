# x64-windows-static with release libraries only. CI builds RelWithDebInfo, which links the release
# libraries, so building the debug ones too would only double the time a dependency takes to build.
set(VCPKG_TARGET_ARCHITECTURE x64)
set(VCPKG_CRT_LINKAGE static)
set(VCPKG_LIBRARY_LINKAGE static)
set(VCPKG_BUILD_TYPE release)
