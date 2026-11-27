#include "flix/codegen.hpp"
#include "flix/program.hpp"
#include "flix/version.hpp"

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#ifdef _WIN32
    #define NOMINMAX
    #include <windows.h>
#else
    #include <cerrno>
    #include <sys/types.h>
    #include <sys/wait.h>
    #include <unistd.h>
#endif

namespace fs = std::filesystem;

namespace {

void printUsage() {
    std::cout
        << "Flix Compiler " << flix::version() << "\n\n"
        << "Usage:\n"
        << "  flixc version\n"
        << "  flixc check <file.flix>\n"
        << "  flixc asm <file.flix> -o <file.asm>\n"
        << "  flixc build <file.flix> -o <file.exe>\n";
}

bool writeFile(
    const fs::path& path,
    const std::string& contents,
    std::string& error
) {
    std::ofstream output(path, std::ios::binary | std::ios::trunc);

    if (!output) {
        error = "Could not create output file: " + path.string();
        return false;
    }

    output.write(
        contents.data(),
        static_cast<std::streamsize>(contents.size())
    );

    if (!output.good()) {
        error = "Failed while writing output file: " + path.string();
        return false;
    }

    return true;
}

bool loadAndValidate(
    const fs::path& sourcePath,
    flix::Program& program
) {
    std::string error;

    if (!flix::loadProgram(sourcePath.string(), program, error)) {
        std::cerr << "Compilation failed:\n"
                  << error << "\n";
        return false;
    }

    return true;
}

std::string serializeVariable(const flix::Variable& variable) {
    std::ostringstream output;

    output << ".var "
           << variable.name << " "
           << flix::valueTypeName(variable.type)
           << " ";

    if (variable.type == flix::ValueType::String) {
        output << '"';

        for (char c : variable.initial_value) {
            switch (c) {
                case '\\': output << "\\\\"; break;
                case '"':  output << "\\\""; break;
                case '\n': output << "\\n"; break;
                case '\t': output << "\\t"; break;
                case '\r': output << "\\r"; break;
                default:   output << c; break;
            }
        }

        output << '"';
    } else {
        output << variable.initial_value;
    }

    return output.str();
}

std::string generateAssembly(const flix::Program& program) {
    std::ostringstream output;

    std::unordered_map<std::size_t, std::string> labelsAtPosition;

    for (const auto& [name, position] : program.labels) {
        labelsAtPosition[position] = name;
    }

    output
        << "; Flix Intermediate Assembly\n"
        << "; Compiler version: " << flix::version() << "\n"
        << "; This is a readable Flix IR listing, not CPU assembly.\n\n";

    output << ".flixasm 1\n";
    output << ".entry " << program.entry_point << "\n\n";

    output << "; Variable declarations\n";

    for (const auto& [name, variable] : program.variables) {
        output << serializeVariable(variable) << "\n";
    }

    output << "\n; Program instructions\n";

    for (std::size_t i = 0; i < program.instructions.size(); ++i) {
        const auto label = labelsAtPosition.find(i);

        if (label != labelsAtPosition.end()) {
            output << "\n." << label->second << ":\n";
        }

        const flix::Instruction& instruction =
            program.instructions[i];

        output << "    " << instruction.opcode;

        if (!instruction.operand.empty()) {
            output << " " << instruction.operand;
        }

        output << "    ; source line "
               << instruction.line << "\n";

        if (instruction.branch_id >= 0) {
            output
                << "    ; branch_id=" << instruction.branch_id
                << " branch_first="
                << (instruction.branch_first ? "true" : "false")
                << " branch_next=" << instruction.branch_next
                << " branch_end=" << instruction.branch_end
                << "\n";
        }
    }

    const auto endLabel =
        labelsAtPosition.find(program.instructions.size());

    if (endLabel != labelsAtPosition.end()) {
        output << "\n." << endLabel->second << ":\n";
    }

    output << "\n; End of Flix program\n";

    return output.str();
}

bool getExecutableDirectory(
    const char* argv0,
    fs::path& directory,
    std::string& error
) {
#ifdef _WIN32
    std::vector<wchar_t> buffer(32768);

    DWORD length = GetModuleFileNameW(
        nullptr,
        buffer.data(),
        static_cast<DWORD>(buffer.size())
    );

    if (length == 0 || length >= buffer.size()) {
        error = "Could not determine the compiler executable location.";
        return false;
    }

    directory = fs::path(
        std::wstring(buffer.data(), length)
    ).parent_path();

    return true;
#else
    std::vector<char> buffer(4096);

    const ssize_t length = readlink(
        "/proc/self/exe",
        buffer.data(),
        buffer.size() - 1
    );

    if (length > 0) {
        buffer[static_cast<std::size_t>(length)] = '\0';
        directory = fs::path(buffer.data()).parent_path();
        return true;
    }

    std::error_code ec;
    fs::path executable = fs::absolute(argv0, ec);

    if (ec) {
        error = "Could not determine the compiler executable location.";
        return false;
    }

    directory = executable.parent_path();
    return true;
#endif
}

fs::path findZig(const fs::path& executableDirectory) {
    if (const char* configured = std::getenv("FLIX_ZIG_PATH")) {
        if (*configured) {
            fs::path candidate(configured);

            if (fs::is_regular_file(candidate)) {
                return candidate;
            }
        }
    }

#ifdef _WIN32
    const fs::path bundled =
        executableDirectory / "compiler" / "toolchain" / "zig.exe";
#else
    const fs::path bundled =
        executableDirectory / "compiler" / "toolchain" / "zig";
#endif

    if (fs::is_regular_file(bundled)) {
        return bundled;
    }

    return {};
}

#ifdef _WIN32

std::wstring quoteWindowsArgument(const std::wstring& argument) {
    if (argument.find_first_of(L" \t\n\v\"") == std::wstring::npos) {
        return argument;
    }

    std::wstring result = L"\"";
    std::size_t backslashes = 0;

    for (wchar_t c : argument) {
        if (c == L'\\') {
            ++backslashes;
            continue;
        }

        if (c == L'"') {
            result.append(backslashes * 2 + 1, L'\\');
            result += L'"';
            backslashes = 0;
            continue;
        }

        result.append(backslashes, L'\\');
        backslashes = 0;
        result += c;
    }

    result.append(backslashes * 2, L'\\');
    result += L'"';

    return result;
}

int runProcess(
    const fs::path& executable,
    const std::vector<std::wstring>& arguments
) {
    std::wstring commandLine =
        quoteWindowsArgument(executable.wstring());

    for (const auto& argument : arguments) {
        commandLine += L" ";
        commandLine += quoteWindowsArgument(argument);
    }

    std::vector<wchar_t> mutableCommand(
        commandLine.begin(),
        commandLine.end()
    );
    mutableCommand.push_back(L'\0');

    STARTUPINFOW startup{};
    startup.cb = sizeof(startup);

    PROCESS_INFORMATION process{};

    if (!CreateProcessW(
            executable.c_str(),
            mutableCommand.data(),
            nullptr,
            nullptr,
            TRUE,
            0,
            nullptr,
            nullptr,
            &startup,
            &process
        )) {
        std::cerr
            << "Could not start Zig compiler. Windows error: "
            << GetLastError() << "\n";
        return -1;
    }

    WaitForSingleObject(process.hProcess, INFINITE);

    DWORD exitCode = 1;
    GetExitCodeProcess(process.hProcess, &exitCode);

    CloseHandle(process.hThread);
    CloseHandle(process.hProcess);

    return static_cast<int>(exitCode);
}

#else

int runProcess(
    const fs::path& executable,
    const std::vector<std::wstring>& arguments
) {
    std::vector<std::string> storage;
    storage.reserve(arguments.size() + 1);

    storage.push_back(executable.string());

    for (const auto& argument : arguments) {
        storage.emplace_back(argument.begin(), argument.end());
    }

    std::vector<char*> argv;
    argv.reserve(storage.size() + 1);

    for (auto& value : storage) {
        argv.push_back(value.data());
    }

    argv.push_back(nullptr);

    const pid_t child = fork();

    if (child < 0) {
        std::cerr << "Could not start Zig compiler.\n";
        return -1;
    }

    if (child == 0) {
        execv(executable.c_str(), argv.data());
        _exit(127);
    }

    int status = 0;

    if (waitpid(child, &status, 0) < 0) {
        return -1;
    }

    if (WIFEXITED(status)) {
        return WEXITSTATUS(status);
    }

    return -1;
}

#endif

class TemporaryDirectory {
public:
    explicit TemporaryDirectory(fs::path path)
        : path_(std::move(path)) {}

    ~TemporaryDirectory() {
        std::error_code ignored;
        fs::remove_all(path_, ignored);
    }

    const fs::path& path() const {
        return path_;
    }

private:
    fs::path path_;
};

bool createTemporaryDirectory(
    fs::path& result,
    std::string& error
) {
    const auto timestamp =
        std::chrono::steady_clock::now().time_since_epoch().count();

#ifdef _WIN32
    const auto processId = GetCurrentProcessId();
#else
    const auto processId = getpid();
#endif

    const fs::path base = fs::temp_directory_path();

    for (unsigned int attempt = 0; attempt < 20; ++attempt) {
        fs::path candidate = base /
            ("flix-build-" + std::to_string(processId) + "-" +
             std::to_string(timestamp) + "-" + std::to_string(attempt));

        std::error_code ec;

        if (fs::create_directory(candidate, ec)) {
            result = candidate;
            return true;
        }

        if (ec && ec != std::errc::file_exists) {
            error = "Could not create temporary build directory: "
                + ec.message();
            return false;
        }
    }

    error = "Could not create a unique temporary build directory.";
    return false;
}

int commandCheck(const fs::path& sourcePath) {
    flix::Program program;

    if (!loadAndValidate(sourcePath, program)) {
        return 1;
    }

    std::cout
        << "Successfully checked "
        << sourcePath.filename().string() << "\n"
        << "Instructions: " << program.instructions.size() << "\n"
        << "Variables: " << program.variables.size() << "\n"
        << "Labels: " << program.labels.size() << "\n";

    return 0;
}

int commandAssembly(
    const fs::path& sourcePath,
    const fs::path& outputPath
) {
    flix::Program program;

    if (!loadAndValidate(sourcePath, program)) {
        return 1;
    }

    std::string error;

    if (!writeFile(outputPath, generateAssembly(program), error)) {
        std::cerr << error << "\n";
        return 1;
    }

    std::cout
        << "Generated Flix intermediate assembly:\n"
        << outputPath.string() << "\n";

    return 0;
}

int commandBuild(
    const fs::path& sourcePath,
    const fs::path& requestedOutput,
    const char* argv0
) {
    flix::Program program;

    if (!loadAndValidate(sourcePath, program)) {
        return 1;
    }

    std::string error;
    std::string generatedC;

    if (!flix::generateC(program, generatedC, error)) {
        std::cerr << "Code generation failed:\n"
                  << error << "\n";
        return 1;
    }

    fs::path executableDirectory;

    if (!getExecutableDirectory(argv0, executableDirectory, error)) {
        std::cerr << error << "\n";
        return 1;
    }

    const fs::path zig = findZig(executableDirectory);

    if (zig.empty()) {
        std::cerr
            << "Bundled Zig compiler was not found.\n\n"
            << "Expected location:\n"
            << (executableDirectory / "compiler" / "toolchain"
#ifdef _WIN32
                / "zig.exe"
#else
                / "zig"
#endif
            ).string()
            << "\n\n"
            << "Place the Zig toolchain in that directory, or set "
               "FLIX_ZIG_PATH to its executable path.\n";
        return 1;
    }

    const fs::path runtimeSource =
        executableDirectory / "compiler" / "runtime.c";

    const fs::path runtimeInclude =
        executableDirectory / "compiler" / "include";

    const fs::path runtimeHeader =
        runtimeInclude / "runtime.h";

    if (!fs::is_regular_file(runtimeSource) ||
        !fs::is_regular_file(runtimeHeader)) {
        std::cerr
            << "The Flix runtime files are missing.\n"
            << "Expected:\n"
            << runtimeSource.string() << "\n"
            << runtimeHeader.string() << "\n";
        return 1;
    }

    fs::path outputPath = fs::absolute(requestedOutput);

    if (outputPath.extension() != ".exe") {
        std::cerr << "Windows executable output must use the .exe extension.\n";
        return 1;
    }

    std::error_code directoryError;
    fs::create_directories(outputPath.parent_path(), directoryError);

    if (directoryError) {
        std::cerr
            << "Could not create output directory: "
            << directoryError.message() << "\n";
        return 1;
    }

    fs::path temporaryPath;

    if (!createTemporaryDirectory(temporaryPath, error)) {
        std::cerr << error << "\n";
        return 1;
    }

    TemporaryDirectory cleanup(temporaryPath);

    const fs::path generatedSource =
        temporaryPath / "flix_generated.c";

    const fs::path temporaryExecutable =
        temporaryPath / outputPath.filename();

    if (!writeFile(generatedSource, generatedC, error)) {
        std::cerr << error << "\n";
        return 1;
    }

    std::cout << "Flix: generating native C code...\n";
    std::cout << "Zig: compiling Windows x86-64 executable...\n";

    const std::vector<std::wstring> arguments = {
        L"build-exe",
        L"-target",
        L"x86_64-windows-gnu",
        L"-O",
        L"ReleaseFast",
        L"-fstrip",
        L"-femit-bin=" + temporaryExecutable.wstring(),
        L"-I",
        runtimeInclude.wstring(),
        generatedSource.wstring(),
        runtimeSource.wstring(),
        L"-lc"
    };

    const int result = runProcess(zig, arguments);

    if (result != 0) {
        std::cerr
            << "Zig compilation failed with exit code "
            << result << ".\n"
            << "No output executable was installed.\n";
        return 1;
    }

    if (!fs::is_regular_file(temporaryExecutable)) {
        std::cerr
            << "Zig reported success, but the expected executable "
               "was not created.\n";
        return 1;
    }

    std::error_code removeError;
    fs::remove(outputPath, removeError);

    if (removeError) {
        std::cerr
            << "Could not replace existing output file: "
            << removeError.message() << "\n";
        return 1;
    }

    std::error_code renameError;
    fs::rename(temporaryExecutable, outputPath, renameError);

    if (renameError) {
        std::cerr
            << "Could not install generated executable: "
            << renameError.message() << "\n";
        return 1;
    }

    std::cout
        << "\nBuild successful!\n"
        << "Output: " << outputPath.string() << "\n"
        << "Target: Windows x86-64\n"
        << "Backend: bundled Zig\n";

    return 0;
}

} // namespace

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printUsage();
        return 1;
    }

    const std::string command = argv[1];

    if (command == "version") {
        std::cout << "Flix Compiler " << flix::version() << "\n";
        return 0;
    }

    if (command == "check") {
        if (argc != 3) {
            printUsage();
            return 1;
        }

        return commandCheck(argv[2]);
    }

    if (command == "asm") {
        if (argc != 5 || std::string(argv[3]) != "-o") {
            printUsage();
            return 1;
        }

        return commandAssembly(argv[2], argv[4]);
    }

    if (command == "build") {
        if (argc != 5 || std::string(argv[3]) != "-o") {
            printUsage();
            return 1;
        }

        return commandBuild(argv[2], argv[4], argv[0]);
    }

    std::cerr << "Unknown command: " << command << "\n\n";
    printUsage();

    return 1;
}
