#pragma once

#include <string>
#include "geo.h"

namespace transport_catalogue::domain {

    struct Stop {
        std::string name;
        geo::Coordinates coordinates;
    };

    struct Bus {
        bool operator==(const Bus& other) const;

        std::string number;
        int stops_on_route = 0;
        int unique_stops = 0;
        int route_length = 0;
        double curvature = 0.;
        bool is_roundtrip = false;
    };

    template<typename First, typename Second>
    class PairPtrHash {
    public:
        uint64_t operator()(const std::pair<First, Second>& pair_pointers) const {
            return hasher_(pair_pointers.first) + 37 * hasher_(pair_pointers.second);
        }

    private:
        std::hash<const void*> hasher_;
    };
}