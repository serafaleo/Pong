import os
import sys
import time
import subprocess

def is_executable_in_path(executable):
    if sys.platform == "win32":
        find_exe_cmd = "where"
    else:
        find_exe_cmd = "which"

    output_find_exe_cmd = subprocess.run([find_exe_cmd, executable], capture_output=True)
    assert len(output_find_exe_cmd.stderr) == 0
    executable_path = output_find_exe_cmd.stdout.decode("utf-8")
    if len(executable_path) > 0:
        if executable_path[-1] == "\n":
            executable_path = executable_path[:-1]
            if executable_path[-1] == "\r":
                executable_path = executable_path[:-1]

    return os.path.isfile(executable_path)

def check_clang():
    is_clang_in_path = is_executable_in_path("clang")
    is_target_x64 = False
    if is_clang_in_path:
        output_clang_version = subprocess.run(["clang", "-v"], capture_output=True)
        assert len(output_clang_version.stdout) == 0
        is_target_x64 = "x86_64" in output_clang_version.stderr.decode("utf-8")

    return is_clang_in_path, is_target_x64

def build_main():
    time_start = time.time()

    # Assign these variables manually =======================================================

    project_root_dir_relative = ".."

    # Leave these strings empty to use the project root directory name as reference.
    program_name = "Pong"
    executable_name = "pong"

    win32_source_files = ["win32/win32_main.c"]
    win32_libraries = ["-lkernel32", "-luser32", "-lwinmm", "-lgdi32", "-lole32"]

    linux_source_files = []
    linux_libraries = []

    macos_source_files = []
    macos_libraries = []

    warnings_to_enable = ["-Wconversion"]

    errors_to_disable = ["-Wno-error=unused-function",
                         "-Wno-error=unused-variable",
                         "-Wno-error=unused-parameter",
                         "-Wno-error=unused-but-set-variable"]

    compile_command = ["clang",
                       "-std=c99",
                       "-fuse-ld=lld",
                       "-nodefaultlibs",
                       "-nostdlib",
                       "-mno-stack-arg-probe",
                      f"-D PROGRAM_NAME=\"{program_name}\"",
                       "-Wall",
                       "-Wextra",
                       "-Werror"] + warnings_to_enable + errors_to_disable

    development_flags = ["-D ASSERTIONS_ON", "-D DEVELOPMENT", "-g"]
    release_flags = ["-D OPTIMIZATIONS_ON", "-O3"]
    slow_flags = development_flags + ["-O0"]
    fast_flags = development_flags + release_flags

    win32_linker_flags = "-Wl,-wx,-subsystem:windows,-incremental:no,-opt:ref"
    linux_linker_flags = ""
    macos_linker_flags = ""

    # =======================================================================================

    if sys.platform != "win32" and sys.platform != "linux" and sys.platform != "darwin":
        print(f"Your running on an unsupported platform: {sys.platform}")
        sys.exit()

    is_clang_in_path, is_target_x64 = check_clang()

    if not is_clang_in_path:
        print("Clang compiler not found. Please, install it and make sure it's in the PATH.")
        sys.exit()

    if not is_target_x64:
        print("Clang target is not x64. This code should only be compiled for x64.")
        sys.exit()

    if len(project_root_dir_relative) > 0:
        if project_root_dir_relative[-1] == "/" or project_root_dir_relative[-1] == "\\":
            project_root_dir_relative = project_root_dir_relative[:-1]

    if len(program_name) == 0:
        program_name = os.path.basename(os.path.abspath(project_root_dir_relative))

    if len(executable_name) == 0:
        executable_name = program_name.lower().replace(" ", "_")

    if sys.platform == "win32":
        build_directory = f"{project_root_dir_relative}/build/win32"
        executable_file = executable_name + ".exe"
    elif sys.platform == "linux":
        build_directory = f"{project_root_dir_relative}/build/linux"
        print("Please, make sure the Linux build settings are configured properly.")
        sys.exit()
    else:
        build_directory = f"{project_root_dir_relative}/build/mac"
        print("Please, make sure the MacOS build settings are configured properly.")
        sys.exit()

    if not os.path.isdir(build_directory):
        os.makedirs(build_directory)
        print(f"Build directory created: {build_directory}\n")

    argv = sys.argv
    argc = len(argv)

    if argc == 1:
        compile_command += slow_flags
    else:
        if argv[1] == "--slow":
            compile_command += slow_flags
        elif argv[1] == "--fast":
            compile_command += fast_flags
        elif argv[1] == "--release":
            compile_command += release_flags
        else:
            compile_command += slow_flags
            compile_command += argv[1:]

        if argc > 2:
            compile_command += argv[2:]

    executable_path = f"{build_directory}/{executable_file}"
    compile_command += ["-o", executable_path]

    if sys.platform == "win32":
        compile_command += win32_source_files
        compile_command += win32_libraries
    elif sys.platform == "linux":
        compile_command += linux_source_files
        compile_command += linux_libraries
    else: # darwin
        compile_command += macos_source_files
        compile_command += macos_libraries

    if sys.platform == "win32":
        compile_command += [win32_linker_flags]
    elif sys.platform == "linux":
        # TODO: linker arguments for linux.
        compile_command += [linux_linker_flags]
        assert 0
    else:
        # TODO: linker arguments for macos.
        compile_command += [macos_linker_flags]
        assert 0

    compile_command_str = " ".join(compile_command)
    print(compile_command_str + "\n")

    with open(f"{build_directory}/last_compile_command.txt", "w") as compile_command_file:
        compile_command_file.write(compile_command_str)

    subprocess.run(compile_command)
    print(f"Total build script time: {round(time.time() - time_start, 2)} seconds.")

if __name__ == "__main__":
    build_main()
