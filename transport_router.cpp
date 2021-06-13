#include <limits>

#include "transport_router.h"

namespace transport_catalogue::router {
    namespace {
        const TransportCatalogue FAKE_CATALOGUE;
    }

    CustomWeight operator+(const CustomWeight& lhs, const CustomWeight& rhs) {
        return CustomWeight{-1, lhs.span_count + lhs.span_count, lhs.time + rhs.time, };
    }

    bool CustomWeight::operator==(const CustomWeight& other) const {
        return std::abs(this->time - other.time) < std::numeric_limits<double>::epsilon();
    }
    bool CustomWeight::operator<(const CustomWeight& other) const {
        return time < other.time;
    }

    bool CustomWeight::operator>(const CustomWeight& other) const {
        return other < *this;
    }

    size_t CustomWeightHash::operator()(const CustomWeight& weight) const {
        size_t hash_id = int_hasher_(weight.unique_id);
        size_t hash_span = int_hasher_(weight.span_count);
        size_t hash_time = min_hasher_(weight.time);
        return hash_id + hash_span * prime_ + hash_time * (prime_ * prime_);
    }

    Item::Item(ItemType type, int span_count, std::string_view name)
        : type(type)
        , span_count(span_count)
        , name(name) {
    }

    TransportRouter::TransportRouter(size_t vertex_count)
        : graph_(vertex_count)
        , catalogue_(FAKE_CATALOGUE) {
    }

    TransportRouter::TransportRouter(const TransportCatalogue& catalogue)
        : graph_(catalogue.GetStops().size() * 2)
        , catalogue_(catalogue) {
    }

    void TransportRouter::AddStops(Minutes bus_wait_time) {
        const auto& stops = catalogue_.GetStops();
        for (const auto& stop : stops) {
            trip_indexes_.emplace(stop.name, stop_indexes_.size());
            stop_indexes_.push_back(stop.name);
            wait_indexes_.emplace(stop.name, stop_indexes_.size());
            stop_indexes_.push_back(stop.name);
            items_.emplace_back(ItemType::WAIT, 0, stop.name);
            graph_.AddEdge({wait_indexes_[stop.name], trip_indexes_[stop.name], bus_wait_time});
        }
    }


    void TransportRouter::AddRoutes(MetersPerMinutes bus_velocity) {
        const auto& routes = catalogue_.GetRoutes();
        for (const auto& [bus_name, route] : routes) {
            const size_t end_stop_index = route.size() / 2;
            if(catalogue_.FindRoute(bus_name)->is_roundtrip) {
                AddRouteOuterLoop(bus_name, route.cbegin(), route.cend(), bus_velocity);
            } else {
                AddRouteOuterLoop(bus_name, route.cbegin(), route.cbegin() + end_stop_index + 1, bus_velocity);
                AddRouteOuterLoop(bus_name, route.cbegin() + end_stop_index, route.cend(), bus_velocity);
            }
        }
    }

    const TransportRouter::Graph& TransportRouter::GetGraph() const {
        return graph_;
    }

    const TransportRouter::IndexMap& TransportRouter::GetTripIndexes() const {
        return trip_indexes_;
    }

    const TransportRouter::IndexMap& TransportRouter::GetWaitIndexes() const {
        return wait_indexes_;
    }

    const std::vector<std::string_view>& TransportRouter::GetStopIndexes() const {
        return stop_indexes_;
    }

    const std::vector<Item>& TransportRouter::GetItems() const {
        return items_;
    }

    void TransportRouter::AddWaitIndex(std::string_view name, graph::VertexId id) {
         wait_indexes_.emplace(name, id);
    }

    void TransportRouter::AddItem(const Item& item) {
        items_.push_back(item);
    }

    graph::EdgeId TransportRouter::AddEdge(const graph::Edge<Minutes>& edge) {
        return graph_.AddEdge(edge);
    }

    void TransportRouter::AddRouteOuterLoop(std::string_view bus_name, RundomIt begin, RundomIt end, MetersPerMinutes bus_velocity) {
        for (auto it = begin; it != (end - 1); ++it) {
            RouteInfo route_info {bus_name, *it, *it, 0., 0};
            AddRouteInnerLoop(route_info, it + 1, end, bus_velocity);
        }
    }

    void TransportRouter::AddRouteInnerLoop(RouteInfo& route_info, RundomIt begin, RundomIt end, MetersPerMinutes bus_velocity) {
        for (auto it = begin; it != end; ++it) {
            std::string_view to_stop = *it;
            std::pair from_to_stops {trip_indexes_[route_info.from_stop], wait_indexes_[to_stop]};
            double distance = catalogue_.FindDistanceBetweenStops(route_info.prev_stop, to_stop);
            route_info.travel_time += (distance / bus_velocity);
            ++route_info.span_count;
            items_.emplace_back(ItemType::BUS, route_info.span_count, route_info.bus_name);
            graph_.AddEdge({trip_indexes_[route_info.from_stop], wait_indexes_[to_stop], route_info.travel_time});
            route_info.prev_stop = to_stop;
        }
    }
}