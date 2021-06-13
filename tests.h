//#include "test_example_functions.h"

#include "geo.h"
#include "request_handler.h"

#include <algorithm>
#include <chrono>
#include <deque>
#include <execution>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <optional>
#include <random>
#include <sstream>
#include <string>
#include <vector>

using namespace std;

// -----------------------------------------------------------------------------

template <typename Type>
ostream& operator<< (ostream& out, const vector<Type>& vect) {
    out << "{ "s;
    for (auto it = vect.begin(); it != vect.end(); ++it) {
        if (it != vect.begin()) {
            out << ", ";
        }
        out << *it;
    }
    out << " }";
    return out;
}

// -----------------------------------------------------------------------------

template <typename Func>
void RunTestImpl(Func& func, const string& func_name) {
    func();
    cerr << func_name << " OK" << endl;
}

#define RUN_TEST(func) RunTestImpl((func), #func)

// -----------------------------------------------------------------------------

void AssertImpl(bool value, const string& value_str, const string& file,
                const string& func, unsigned line, const string& hint) {
    if (!value) {
        cerr << file << "("s << line << "): "s << func << ": "s;
        cerr << "ASSERT("s << value_str << ") failed."s;
        if (!hint.empty()) {
            cerr << " Hint: "s << hint;
        }
        cerr << endl;
        abort();
    }
}

#define ASSERT(a) AssertImpl((a), #a, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_HINT(a, hint) AssertImpl((a), #a, __FILE__, __FUNCTION__, __LINE__, hint)

// -----------------------------------------------------------------------------

template <typename T, typename U>
void AssertEqualImpl(const T& t, const U& u, const string& t_str, const string& u_str, const string& file,
                     const string& func, unsigned line, const string& hint) {
    if (t != u) {
        cerr << boolalpha;
        cerr << file << "("s << line << "): "s << func << ": "s;
        cerr << "ASSERT_EQUAL("s << t_str << ", "s << u_str << ") failed: "s;
        cerr << t << " != "s << u << "."s;
        if (!hint.empty()) {
            cerr << " Hint: "s << hint;
        }
        cerr << endl;
        abort();
    }
}

#define ASSERT_EQUAL(a, b) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_EQUAL_HINT(a, b, hint) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, (hint))

// -----------------------------------------------------------------------------

enum class ErrorCode {
    OUT_OF_RANGE,
    INVALID_ARGUMENT
};

const string ErrorCodeName(ErrorCode code) {
    switch(code) {
        case ErrorCode::OUT_OF_RANGE: return "out_of_range"s;
        case ErrorCode::INVALID_ARGUMENT: return "invalid_argument"s;
        default: break;
    }
    ASSERT_HINT(false, "invalid exception"s);
    return "invalid exception"s;
}

const string ErrorCodeHint(ErrorCode code) {
    return "Must be "s + ErrorCodeName(code) + " exception"s;
}

template <typename Func>
void AssertTrowImpl(Func& func, ErrorCode code,
                    const string& file, const string& func_name, unsigned line) {
    try
    {
        func();
        AssertImpl(false, "Task failed successfully"s, file, func_name, line, ErrorCodeHint(code));
    }
    catch(const out_of_range&) {
        AssertImpl(code == ErrorCode::OUT_OF_RANGE, ErrorCodeName(code), file, func_name, line, ErrorCodeHint(code));
    }
    catch(const invalid_argument&) {
        AssertImpl(code == ErrorCode::INVALID_ARGUMENT, ErrorCodeName(code), file, func_name, line, ErrorCodeHint(code));
    }
    catch(...) {
        AssertImpl(false, ErrorCodeName(code), file, func_name, line, ErrorCodeHint(code));
    }
    cerr << func_name << " OK" << endl;
}

#define ASSERT_OUT_OF_RANGE(a) AssertTrowImpl((a), ErrorCode::OUT_OF_RANGE, __FILE__, #a, __LINE__)

#define ASSERT_INVALID_ARGUMENT(a) AssertTrowImpl((a), ErrorCode::INVALID_ARGUMENT, __FILE__, #a, __LINE__)

// -----------------------------------------------------------------------------

template <typename UnitOfTime>
class AssertDuration {
public:
    using Clock = std::chrono::steady_clock;

    AssertDuration(int64_t max_duration, const std::string file, const std::string function, unsigned line)
            : max_dur_(max_duration)
            , file_(file)
            , function_(function)
            , line_(line) {
    }

    ~AssertDuration() {
        const auto dur = Clock::now() - start_time_;
        const auto converted_dur = std::chrono::duration_cast<UnitOfTime>(dur).count();
        if (converted_dur > max_dur_) {
            cerr << "Assert duration fail: "s << file_ << " "s << function_ << ": "s << line_ << endl;
            cerr << "Process duration is "s << converted_dur << " while max duration is " << max_dur_ << endl;
            cerr << "So the function worked longer on "s << converted_dur - max_dur_ << endl;
            abort();
        }
    }

private:
    int64_t max_dur_;
    std::string file_;
    std::string function_;
    unsigned line_;
    const Clock::time_point start_time_ = Clock::now();
};

#define ASSERT_DURATION_MILLISECONDS(x) AssertDuration<std::chrono::milliseconds> UNIQUE_VAR_NAME_PROFILE(x, __FILE__, __FUNCTION__, __LINE__)
#define ASSERT_DURATION_SECONDS(x) AssertDuration<std::chrono::seconds> UNIQUE_VAR_NAME_PROFILE(x, __FILE__, __FUNCTION__, __LINE__)

// -----------------------------------------------------------------------------

// -------- Начало модульных тестов ----------

static const std::string file_path = "/general/Amrulla/Programming/C++/Yandex/4. Elegance and scalability/Sprint 10/5. Inheritance and polymorphism/Lesson 13. Transport transport_catalogue (svg)/Tests/"s;
static const std::string file_path_in = file_path + "input/"s;
static const std::string file_path_out = file_path + "output/"s;

void LoadFile(std::stringstream& in, const std::string& path, const std::string& file_name) {
    std::ifstream file(path + file_name);
    in << file.rdbuf();
}

#define LOAD_FILE(in, file_name) (LoadFile(in, file_path_in, file_name))

void SaveFile(const std::string& path, const std::string& file_name, const std::string& file_data) {
    std::ofstream file(path + file_name);
    file << file_data;
    file.close();
}

#define SAVE_FILE(name, file_data) (SaveFile(file_path_out, name, file_data))

// ----------------------------------------------------------------------------

struct TestDataResult {
    std::string program;
    std::string expected;
};

void RemoveIndentInPlace(std::string& str) {
    str.erase(std::remove_if(str.begin(), str.end(), [](char c) { return c == ' ' || c == '\t' || c == '\n' || c == '\r'; }), str.end());
}

std::vector<std::string> GetFileNames(const std::string& path) {
    std::vector<std::string> files;
    for (const auto& entry : std::filesystem::directory_iterator(path)) {
        files.push_back(entry.path().filename().u8string());
    }
    return files;
}

void TestFromFile() {
    std::map<int, TestDataResult> test_data;

    auto GetId = [](const std::string& name) {
        size_t it_begin = name.find_first_of('_') + 1;
        size_t it_end = name.find_first_of('.', it_begin + 1);
        return std::stoi(std::string(name.begin() + it_begin, name.begin() + it_end));
    };

    // Загружаем проверочные данные из папки
    for (const std::string& file_name : GetFileNames(file_path_in)) {
        std::stringstream input;
        LOAD_FILE(input, file_name);

        int id = GetId(file_name);

        if (file_name.front() == 'i') {
            std::stringstream output;

            // Место для Вашего кода
            //request_handler::RequestHandler(input, output);

            transport_catalogue::TransportCatalogue catalogue;
            reader::JsonReader(catalogue, input, output);
            //ASSERT(false);

            test_data[id].program = output.str();
            SAVE_FILE(file_name, test_data[id].program);
        }
        else {
            test_data[id].expected = input.str();
        }
    }

    // Проверяем результат работы программы с ожидаемым
    for (auto& [key, value] : test_data) {
        // Удаляем все возможные специальные символы, чтобы легче было проверять
        // Это может привести к неправильному результату, если пробелы в названиях остановок и автобусных маршрутах важны
        RemoveIndentInPlace(value.program);
        RemoveIndentInPlace(value.expected);

        ASSERT_HINT(value.program == value.expected, "Error in file with id = \""s + std::to_string(key) + "\"");
    }
}

// ----------------------------------------------------------------------------

static const std::string stop_prefix = "stop_"s;
static const std::string bus_prefix = "bus_"s;

std::string CreateStop(std::mt19937& generator, const std::string& stop_name) {
    std::string stop;
    stop += "\"type\": \"RenderStop\",\n"s;
    stop += "\"name\": \"" + stop_prefix + stop_name + "\",\n"s;

    double latitude = std::uniform_real_distribution<double>(-90.0, 90.0)(generator);
    double longitude = std::uniform_real_distribution<double>(-180.0, 180.0)(generator);

    stop += "\"latitude\":"s + std::to_string(latitude) + ",\n"s;
    stop += "\"longitude\":"s + std::to_string(longitude) + ",\n"s;

    return stop;
}

void AddRoadDistances(std::mt19937& generator, std::string& current_stop, int max_stop_name) {
    std::string road_distances = "\"road_distances\": {"s;
    bool first = true;
    for (int i = 0; i < max_stop_name; ++i) {
        if (!first) {
            road_distances += ", "s;
        }
        int distance = std::uniform_int_distribution<int>(100, 10000)(generator);
        road_distances += "\"" + stop_prefix + std::to_string(i) + "\": "s + std::to_string(distance);
        first = false;
    }
    road_distances += "}"s;
    current_stop += road_distances;
}

std::vector<std::string> CreateStops(std::mt19937& generator, int count) {
    std::vector<std::string> stops;
    for (int i = 0; i < count; ++i) {
        stops.push_back(CreateStop(generator, std::to_string(i)));
        AddRoadDistances(generator, stops.back(), i + 1);
    }
    return stops;
}

std::string CreateBus(std::mt19937& generator, const std::string& name, const std::vector<std::string>& stops, int stops_in_route_count) {
    std::string bus;
    bus += "\"type\": \"Bus\",\n"s;
    bus += "\"name\": \"" + bus_prefix + name + "\",\n"s;
    bus += "\"stops\": [";

    bool is_roundtrip = uniform_int_distribution<size_t>(0, 1)(generator);
    size_t stops_count = uniform_int_distribution<size_t>(0, stops_in_route_count)(generator);
    std::optional<size_t> first_stop;
    bool first = true;
    for (size_t i = 0; i < stops_count; ++i) {
        if (!first) {
            bus += ", "s;
        }
        size_t index = uniform_int_distribution<size_t>(0, stops.size() - 1)(generator);
        bus += "\"" + stop_prefix + std::to_string(index) +"\""s;
        first = false;

        if (!first_stop) {
            first_stop = index;
        }
    }
    if (is_roundtrip && first_stop) {
        bus += ", \""s + stop_prefix + std::to_string(*first_stop) + "\""s;
    }
    bus += "],\n"s;
    if (is_roundtrip) {
        bus += "\"is_roundtrip\": true"s;
    }
    else {
        bus += "\"is_roundtrip\": false"s;
    }
    return bus;
}

std::vector<std::string> CreateBuses(std::mt19937& generator, const std::vector<std::string>& stops, int bus_count, int max_stops_in_route) {
    std::vector<std::string> buses;
    for (int i = 0; i < bus_count; ++i) {
        buses.push_back(CreateBus(generator, std::to_string(i), stops, max_stops_in_route));
    }
    return buses;
}

std::string CreateBaseRequests(std::mt19937& generator, const std::vector<std::string>& buses, const std::vector<std::string>& stops) {
    std::string request;

    int bus_counter = 0;
    bool is_bus_valid = !buses.empty();
    int stop_counter = 0;
    bool is_stop_valid = !stops.empty();

    bool first = true;
    while (true) {
        if (!first) {
            request += ",\n";
        }
        first = false;

        int which_turn = std::uniform_int_distribution<int>(0, 1)(generator);

        if ((which_turn == 0 || !is_stop_valid) && is_bus_valid) {
            request += "{\n"s + buses.at(bus_counter++) + "\n}";
            is_bus_valid = !(bus_counter == buses.size());
        }
        else if (is_stop_valid) {
            request += "{\n"s + stops.at(stop_counter++) + "\n}";
            is_stop_valid = !(stop_counter == stops.size());
        }

        if (!is_bus_valid && !is_stop_valid) {
            break;
        }
    }

    return request;
}

std::string CreateRenderSettings() {
    std::string render_settings =
            "\"width\": 1000,\n"s +
            "\"height\": 1000,\n"s +
            "\"padding\": 30,\n"s +
            "\"stop_radius\": 5,\n"s +
            "\"line_width\": 14,\n"s +
            "\"bus_label_font_size\": 20,\n"s +
            "\"bus_label_offset\": [7, 15],\n"s +
            "\"stop_label_font_size\": 20,\n"s +
            "\"stop_label_offset\": [7, -3],\n"s +
            "\"underlayer_color\": [255, 255, 255, 0.85],\n"s +
            "\"underlayer_width\": 3,\n"s +
            "\"color_palette\": [\"green\", [255, 160, 0], \"red\", [255, 111, 3, 4]]"s;
    return render_settings;
}

std::string CreateStatRequest(std::mt19937 generator, int request_count, int stop_count, int bus_count) {
    std::string request;
    bool first = true;
    for (int i = 0; i < request_count; ++i) {
        if (!first) {
            request += ",\n";
        }
        first = false;

        if (i == 0 || i == (request_count / 2)) {
            request += "{ \"id\": " + std::to_string(i) + ", \"type\": \"Map\" }";
        }
        else if (i % 2 == 0) {
            int stop_id = std::uniform_int_distribution<int>(0, stop_count - 1)(generator);
            request += "{ \"id\": " + std::to_string(i) + ", \"type\": \"RenderStop\", \"name\": \"" + stop_prefix + std::to_string(stop_id) + "\" }"s;
        }
        else {
            int bus_id = std::uniform_int_distribution<int>(0, bus_count - 1)(generator);
            request += "{ \"id\": " + std::to_string(i) + ", \"type\": \"Bus\", \"name\": \"" + bus_prefix + std::to_string(bus_id) + "\" }"s;
        }
    }
    return request;
}

void TestRandomValues() {
    std::mt19937 generator;
    generator.seed();

    int stop_count = 100;
    int bus_count = 100;
    int stops_in_route_count = 100;
    int request_count = 20000;

    std::vector<std::string> stops = std::move(CreateStops(generator, stop_count));
    std::vector<std::string> buses = std::move(CreateBuses(generator, stops, bus_count, stops_in_route_count));

    std::string request = "{\n"s;

    request += "\"base_requests\": [\n"s;
    request += CreateBaseRequests(generator, buses, stops);
    request += "\n],\n"s;

    request += "\"render_settings\": {\n"s;
    request += CreateRenderSettings();
    request += "\n},\n"s;

    request += "\"stat_requests\": [\n"s;
    request += CreateStatRequest(generator, request_count, stop_count, bus_count);
    request += "]\n}";

    SAVE_FILE("request.txt"s, request);

    std::stringstream in;
    std::stringstream out;
    in << request;

    {
        ASSERT_DURATION_SECONDS(2);

        // Место для Вашего кода
        //request_handler::RequestHandler(input, output);
        transport_catalogue::TransportCatalogue catalogue;
        reader::JsonReader(catalogue, in, out);
        //ASSERT(false);
    }

    SAVE_FILE("request_result.txt"s, out.str());
}

// --------- Окончание модульных тестов -----------

// Функция TestTransportCatalogue является точкой входа для запуска тестов
void TestTransportCatalogue() {
   //RUN_TEST(TestFromFile);

#ifndef _DEBUG
    RUN_TEST(TestRandomValues);
#endif
}
