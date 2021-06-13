#include <filesystem>
#include <fstream>
#include <iostream>
#include <string_view>

#include "json_reader.h"
#include "log_duration.h"
#include "transport_catalogue.h"

using namespace std::literals;

namespace tc = transport_catalogue;

const int FIRST_TEST = 1;
const int LAST_TEST = 12;

void PrintUsage(std::ostream& stream = std::cerr) {
    stream << "Usage: transport_catalogue [make_base|process_requests]\n"sv;
}

void MakeBaseTests() {
    for (int i = FIRST_TEST; i < LAST_TEST + 1; ++i) {
        std::filesystem::path in_path = "input_example_make_base"s + std::to_string(i) + ".json"s;
        LOG_DURATION(in_path.filename().string());
        std::ifstream in(in_path);
        tc::TransportCatalogue catalogue;
        tc::reader::MakeBase(catalogue, in);
    }
}

void ProcessRequestsTests() {
    for (int i = FIRST_TEST; i < LAST_TEST + 1; ++i) {
        std::filesystem::path in_path = "input_example_process_requests"s + std::to_string(i) + ".json"s;
        std::filesystem::path out_path = "output_example_process_requests"s + std::to_string(i) + ".json"s;
        std::ifstream in(in_path);
        std::ofstream out(out_path);
        LOG_DURATION(in_path.filename().string());
        tc::reader::ProcessRequests(in, out);
    }
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        PrintUsage();
        return 1;
    }

    const std::string_view mode(argv[1]);

    if (mode == "make_base"sv) {
        tc::TransportCatalogue catalogue;
        tc::reader::MakeBase(catalogue, std::cin);
    } else if (mode == "process_requests"sv) {
        tc::reader::ProcessRequests(std::cin, std::cout);
    } else if (mode == "test") {
        MakeBaseTests();
        ProcessRequestsTests();
    } else {
        PrintUsage();
        return 1;
    }
}