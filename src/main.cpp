#include "app/Application.h"

#include <cmath>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string_view>

namespace {

double parsePositiveSeconds(const char* value)
{
    char* parseEnd = nullptr;
    const double seconds = std::strtod(value, &parseEnd);
    if (parseEnd == value || *parseEnd != '\0' || !std::isfinite(seconds) || seconds <= 0.0) {
        throw std::invalid_argument("--smoke-seconds must be a positive number.");
    }
    return seconds;
}

dentalviz::ApplicationRunOptions parseOptions(int argc, char* argv[])
{
    dentalviz::ApplicationRunOptions options;
    for (int index = 1; index < argc; ++index) {
        const std::string_view argument(argv[index]);
        if (argument == "--smoke-seconds" && index + 1 < argc) {
            options.maximumRuntimeSeconds = parsePositiveSeconds(argv[++index]);
        } else if (argument == "--model" && index + 1 < argc) {
            const std::filesystem::path modelPath(argv[++index]);
            if (modelPath.empty()) {
                throw std::invalid_argument("--model path must not be empty.");
            }
            options.modelPath = modelPath;
        } else {
            throw std::invalid_argument(
                "Usage: DentalViz [--model <STL-or-OBJ-path>] "
                "[--smoke-seconds <positive number>]");
        }
    }
    return options;
}

} // namespace

int main(int argc, char* argv[])
{
    try {
        const dentalviz::ApplicationRunOptions options = parseOptions(argc, argv);
        dentalviz::Application application;
        return application.run(options);
    } catch (const std::exception& error) {
        std::cerr << "DentalViz failed: " << error.what() << '\n';
        return 1;
    }
}
