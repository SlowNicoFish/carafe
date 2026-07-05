set shell := ['bash', '-eu', '-o', 'pipefail', '-c']

# Configure
configure type="Debug":
    cmake -S . -B build/{{ type }} -DCMAKE_BUILD_TYPE={{ type }} -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++

# Build
build type="Debug":
    cmake --build build/{{ type }}

# Install
install type="Debug":
    cmake --install build/{{ type }}

# Uninstall
uninstall type="Debug":
    cmake --build build/{{ type }} --target uninstall

# Clean
clean:
    rm -rf build/Debug build/Release
