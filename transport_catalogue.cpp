#include <algorithm>
#include <iostream>
#include <vector>
#include <unordered_set>

#include "transport_catalogue.h"

namespace transport_catalogue {

    void TransportCatalogue::AddRoute(std::string_view bus_number, const std::vector<std::string> &stops,
                                      bool is_roundtrip) {
        int stops_on_route = stops.size();
        int unique_stops = std::unordered_set<std::string_view>{stops.begin(), stops.end()}.size();
        double geographic_distance = 0;
        int route_length = 0;
        if (stops.size() > 1) {
            for (auto it = stops.begin(); it < stops.end() - 1; ++it) {
                Stop *from = FindStop(*it);
                Stop *to = FindStop(*(it + 1));
                geographic_distance += ComputeDistance(from->coordinates, to->coordinates);
                route_length += FindDistanceBetweenStops(from->name, to->name);
            }
        }
        buses_.push_back({std::string{bus_number}, stops_on_route, unique_stops, route_length,
                          route_length / geographic_distance, is_roundtrip});
        pointer_to_buses_[buses_.back().number] = &buses_.back();
        for (std::string_view str : stops) {
            buses_at_stops_[FindStop(str)].insert(buses_.back().number);
            routes_[buses_.back().number].push_back(FindStop(str)->name);
        }
    }

    void TransportCatalogue::AddStop(std::string_view stop_name, const geo::Coordinates &coordinates) {
        stops_.push_back({std::string{stop_name}, coordinates});
        pointer_to_stops_[stops_.back().name] = &stops_.back();
        if (!buses_at_stops_.count(&stops_.back())) {
            buses_at_stops_[&stops_.back()];
        }
    }

    void TransportCatalogue::AddDistanceBetweenStops(std::string_view from_stop_name, std::string_view to_stop_name,
                                                     double distance) {
        std::pair pair_stops{FindStop(from_stop_name), FindStop(to_stop_name)};
        distance_between_stops_[pair_stops] = distance;
    }

    domain::Bus *TransportCatalogue::FindRoute(std::string_view bus_name) const {
        auto found_bus = pointer_to_buses_.find(bus_name);
        if (found_bus != pointer_to_buses_.end()) {
            return found_bus->second;
        } else {
            return nullptr;
        }
    }

    domain::Stop *TransportCatalogue::FindStop(std::string_view stop_name) const {
        auto found_stop = pointer_to_stops_.find(stop_name);
        if (found_stop != pointer_to_stops_.end()) {
            return found_stop->second;
        } else {
            return nullptr;
        }
    }

    const std::set<std::string_view> &TransportCatalogue::FindBusesAtStops(std::string_view stop_name) const {
        static const std::set<std::string_view> empty_set;
        const auto found_buses = buses_at_stops_.find(FindStop(stop_name));
        if (found_buses != buses_at_stops_.end()) {
            return found_buses->second;
        } else {
            return empty_set;
        }
    }

    int TransportCatalogue::FindDistanceBetweenStops(std::string_view from_stop, std::string_view to_stop) const {
        Stop *pointer_from_stop = FindStop(from_stop);
        Stop *pointer_to_stop = FindStop(to_stop);
        if (distance_between_stops_.count(std::pair(pointer_from_stop, pointer_to_stop)) > 0) {
            return distance_between_stops_.at(std::pair(pointer_from_stop, pointer_to_stop));
        } else {
            return distance_between_stops_.at(std::pair(pointer_to_stop, pointer_from_stop));
        }
    }

    const TransportCatalogue::RouteMap &TransportCatalogue::GetRoutes() const {
        return routes_;
    }

    const std::deque<domain::Stop> &TransportCatalogue::GetStops() const {
        return stops_;
    }

    const TransportCatalogue::DistanceMap &TransportCatalogue::GetDistanceBetweenStops() const {
        return distance_between_stops_;
    }
}