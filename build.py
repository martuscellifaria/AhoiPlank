import os
import sys
import platform
import shutil


def run_cmake_on_linux(build_type: str, version: str = "99.99.99", company: str = "", description: str = "", build_tests: bool = False):
    if version == "":
        version = "99.99.99"

    flags = (
        f'-DCMAKE_BUILD_TYPE={build_type} '
        f'-DVERSION_ARG="{version}" '
        f'-DCOMPANY_ARG="{company}" '
        f'-DDESCRIPTION_ARG="{description}" '
        f'-DCMAKE_EXPORT_COMPILE_COMMANDS=ON'
    )
    if build_tests:
        flags += ' -DBUILD_TESTING=ON'

    run_tests = " && ctest --output-on-failure" if build_tests else ""

    if shutil.which("ninja") is not None:
        build_command = f"""
        mkdir -p build &&
        cd build &&
        cmake .. -G Ninja {flags} &&
        ln -sf build/compile_commands.json ../ &&
        ninja > build.log 2>&1{run_tests}
        """
    else:
        build_command = f"""
        mkdir -p build &&
        cd build &&
        cmake .. {flags} &&
        ln -sf build/compile_commands.json ../ &&
        make -j$(nproc) > build.log 2>&1{run_tests}
        """

    status = os.system(build_command)
    if status != 0:
        exit_code = os.waitstatus_to_exitcode(status)
        sys.exit(exit_code)
    sys.exit(0)


def run_app(build_type: str, version: str = "", company: str = "", description: str = "", build_tests: bool = False):
    operating_system = platform.system()
    match operating_system:
        case "Linux":
            print("Linux detected, let us run the build.")
            run_cmake_on_linux(build_type, version, company, description, build_tests)


if __name__ == '__main__':
    with_tests = "--with-tests" in sys.argv
    argv = [a for a in sys.argv if a != "--with-tests"]

    if len(argv) < 2:
        print("Welcome to AhoiCpp build process. Pick your build or just press enter to go fast.")
        print("[Enter]  FastBuild")
        print("[0]      Release")
        print("[1]      Debug")
        print("[2]      Release + Tests")
        print("[3]      Debug + Tests")
        build_selection = input()

        match(build_selection):
            case "":
                run_app("Release", "99.99.99", "AhoiTools", "", with_tests)
            case "0":
                print("You selected Release. Please add a version to your build. Format recommended: XX.XX.XX. Hint: you can just press enter and leave it empty.")
                version = input()
                run_app("Release", version, "AhoiTools", "", with_tests)
            case "1":
                print("Build as Debug")
                run_app("RelWithDebInfo", "", "AhoiTools", "", with_tests)
            case "2":
                print("Build as Release with tests.")
                run_app("Release", "99.99.99", "AhoiTools", "", True)
            case "3":
                print("Build as Debug with tests.")
                run_app("RetWithDebInfo", "99.99.99", "AhoiTools", "", True)
            case _:
                print("Invalid input. Bye.")
    elif (argv[1] == "debug"):
        version = argv[2] if len(argv) > 2 else "99.99.99"
        company = argv[3] if len(argv) > 3 else "AhoiTools"
        description = argv[4] if len(argv) > 4 else "Your project is owned by AhoiTools."
        run_app("RelWithDebInfo", version, company, description, with_tests)
    elif (argv[1] == "tests"):
        version = argv[2] if len(argv) > 2 else "99.99.99"
        company = argv[3] if len(argv) > 3 else "AhoiTools"
        description = argv[4] if len(argv) > 4 else "Your project is owned by AhoiTools."
        run_app("Release", version, company, description, True)
    else:
        version = argv[2] if len(argv) > 2 else "99.99.99"
        company = argv[3] if len(argv) > 3 else "AhoiTools"
        description = argv[4] if len(argv) > 4 else "Your project is owned by AhoiTools."
        run_app("Release", version, company, description, with_tests)
