#include "app/Application.h"

#include <cmath>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string_view>

namespace {

std::optional<double> parseMaximumRuntime(int argc, char* argv[])
{
    if (argc == 1) {
        return std::nullopt;
    }

    if (argc != 3 || std::string_view(argv[1]) != "--smoke-seconds") {
        throw std::invalid_argument("Usage: DentalViz [--smoke-seconds <positive number>]");
    }

    char* parseEnd = nullptr;
    const double seconds = std::strtod(argv[2], &parseEnd);
    if (parseEnd == argv[2] || *parseEnd != '\0' || !std::isfinite(seconds) || seconds <= 0.0) {
        throw std::invalid_argument("--smoke-seconds must be a positive number.");
    }
    return seconds;
}

} // namespace

int main(int argc, char* argv[])
{
    try {
        const auto maximumRuntime = parseMaximumRuntime(argc, argv);
        dentalviz::Application application;
        return application.run(maximumRuntime);
    } catch (const std::exception& error) {
        std::cerr << "DentalViz failed: " << error.what() << '\n';
        return 1;
    }
}
