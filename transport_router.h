#pragma once

#include "graph.h"
#include "router.h"
#include "transport_catalogue.h"

namespace transport_catalogue::router {
    using Minutes = double;
    using MetersPerMinutes = double;

    enum class ItemType {
        WAIT,
        BUS
    };

    struct CustomWeight {
        int unique_id;
        int span_count;
        Minutes time;

        bool operator==(const CustomWeight& other) const;
        bool operator<(const CustomWeight& other) const;
        bool operator>(const CustomWeight& other) const;
    };

    class CustomWeightHash {
    public:
        size_t operator()(const CustomWeight& weight) const;

    private:
        std::hash<int> int_hasher_;
        std::hash<Minutes> min_hasher_;
        static const std::size_t prime_ = 37;
    };

    struct Item {
        Item(ItemType type, int span_count, std::string_view name);

        ItemType type;
        int32_t span_count;
        std::string_view name;
    };

    CustomWeight operator+(const CustomWeight& lhs, const CustomWeight& rhs);

    class TransportRouter {
    public:
        using Graph = graph::DirectedWeightedGraph<Minutes>;
        using IndexMap = std::unordered_map<std::string_view , graph::VertexId>;

        explicit TransportRouter(size_t vertex_count);
        explicit TransportRouter(const TransportCatalogue& catalogue);
        void AddStops(Minutes bus_wait_time);
        void AddRoutes(MetersPerMinutes bus_velocity);

        const Graph& GetGraph() const;
        const IndexMap& GetTripIndexes() const;
        const IndexMap& GetWaitIndexes() const;
        const std::vector<std::string_view>& GetStopIndexes() const;
        const std::vector<Item>& GetItems() const;

        void AddWaitIndex(std::string_view name, graph::VertexId id);
        void AddItem(const Item& info);
        graph::EdgeId AddEdge(const graph::Edge<Minutes>& edge);

    private:
        struct RouteInfo {
            const std::string_view bus_name;
            const std::string_view from_stop;
            std::string_view prev_stop;
            Minutes travel_time = 0.;
            int span_count = 0;
        };

        using RundomIt = std::vector<std::string_view>::const_iterator;
        void AddRouteOuterLoop(std::string_view bus_name, RundomIt begin, RundomIt end, double bus_velocity);
        void AddRouteInnerLoop(RouteInfo& route_info, RundomIt begin, RundomIt end, double bus_velocity);

        Graph graph_;
        IndexMap trip_indexes_;
        IndexMap wait_indexes_;
        std::vector<std::string_view> stop_indexes_;
        std::vector<Item> items_;
        const TransportCatalogue& catalogue_;
    };
}