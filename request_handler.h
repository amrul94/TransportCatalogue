#pragma once

#include <optional>
#include <string_view>
#include <unordered_set>

#include "domain.h"
#include "graph.h"
#include "json.h"
#include "map_renderer.h"
#include "router.h"
#include "transport_catalogue.h"
#include "transport_router.h"

namespace transport_catalogue::request_handler {

    struct RouteInfo {
        json::Array items;
        router::Minutes total_time;
    };

    class RequestHandler {
    public:
        using BusPtr = std::string_view;
        using TransportCatalogue = transport_catalogue::TransportCatalogue;
        using MapRenderer = renderer::MapRenderer;
        using TransportRouter = router::TransportRouter;

        RequestHandler(const TransportCatalogue& catalogue, const MapRenderer& renderer,
                       const TransportRouter& transport_router, const graph::Router<router::Minutes>& router);

        [[nodiscard]] domain::Bus* GetBusStat(std::string_view bus_name) const;
        [[nodiscard]] const std::set<BusPtr>* GetBusesByStop(std::string_view stop_name) const;
        [[nodiscard]] svg::Document RenderMap() const;
        [[nodiscard]] std::optional<RouteInfo> GetItems(std::string_view from_stop, std::string_view to_stop) const;

    private:
        const TransportCatalogue& db_;
        const MapRenderer& renderer_;
        const TransportRouter& tr_;
        const graph::Router<router::Minutes>& router_;
    };
}
