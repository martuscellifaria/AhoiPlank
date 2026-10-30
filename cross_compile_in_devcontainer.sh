if [ ! -f /.dockerenv ]; then
    echo "You are not in a Docker container. Aborting."
    exit 1
fi

for config in "linux-x86_64:toolchain-x86_64-linux.cmake" "linux-arm64:toolchain-aarch64-linux.cmake"; do
    build_dir="${config%%:*}"
    toolchain_file="${config##*:}"
    echo "Building ${build_dir}..."
    mkdir -p "cross-build/${build_dir}" || exit 1
    cd "cross-build/${build_dir}" || exit 1
    cmake ../.. -DCMAKE_TOOLCHAIN_FILE="../../cmake/${toolchain_file}" -DCMAKE_BUILD_TYPE="Release" -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -G Ninja || exit 1
    ninja > build.log 2>&1
    cd ../.. || exit 1
done
