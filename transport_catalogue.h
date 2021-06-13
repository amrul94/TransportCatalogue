#pragma once

#include <deque>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>

#include "domain.h"
#include "geo.h"

namespace transport_catalogue {

    // Транспортный справочник
    class TransportCatalogue {
    public:
        using Bus = domain::Bus;
        using Stop = domain::Stop;
        using RouteMap = std::map<std::string_view, std::vector<std::string_view>>;
        using DistanceMap = std::unordered_map<std::pair<Stop*, Stop*>, int, domain::PairPtrHash<Stop*, Stop*>>;

        TransportCatalogue() noexcept = default;

        void AddRoute(std::string_view bus_number, const std::vector<std::string>& stops, bool is_roundtrip);
        void AddStop(std::string_view stop_name, const geo::Coordinates& coordinates);
        void AddDistanceBetweenStops(std::string_view from_stop_name, std::string_view to_stop_name, double distance);

        Bus* FindRoute(std::string_view bus_name) const;
        Stop* FindStop(std::string_view stop_name) const;
        const std::set<std::string_view>& FindBusesAtStops(std::string_view stop_name) const;
        int FindDistanceBetweenStops(std::string_view from_stop, std::string_view to_stop) const;

        const RouteMap& GetRoutes() const;
        const std::deque<Stop>& GetStops() const;
        const DistanceMap& GetDistanceBetweenStops() const;

    private:
        std::deque<Bus> buses_;
        std::deque<Stop> stops_;
        std::unordered_map<std::string_view, Bus*> pointer_to_buses_;
        std::unordered_map<std::string_view, Stop*> pointer_to_stops_;
        std::unordered_map<Stop*, std::set<std::string_view>> buses_at_stops_;
        DistanceMap distance_between_stops_;
        RouteMap routes_;
    };
}