#include "request_handler.h"
#include "json_builder.h"

namespace transport_catalogue::request_handler {

    using BusPtr = std::string_view;

    RequestHandler::RequestHandler(const TransportCatalogue& catalogue, const MapRenderer& renderer,
                                   const TransportRouter& transport_router, const graph::Router<router::Minutes>& router)
        : db_(catalogue)
        , renderer_(renderer)
        , tr_(transport_router)
        , router_(router) {
    }

    domain::Bus* RequestHandler::GetBusStat(std::string_view bus_name) const {
        return db_.FindRoute(bus_name);
    }

    const std::set<BusPtr>* RequestHandler::GetBusesByStop(std::string_view stop_name) const {
        domain::Stop* pointer_to_stop = db_.FindStop(stop_name);
        if (!pointer_to_stop) {
            return nullptr;
        } else {
            return &db_.FindBusesAtStops(stop_name);
        }
    }

    svg::Document RequestHandler::RenderMap() const {
        return renderer_.Render();
    }

    std::optional<RouteInfo> RequestHandler::GetItems(std::string_view from_stop, std::string_view to_stop) const {
        auto route_info = router_.BuildRoute(tr_.GetWaitIndexes().at(from_stop), tr_.GetWaitIndexes().at(to_stop));
        if (!route_info) {
            return std::nullopt;
        }
        router::Minutes total_time = 0.;
        json::Builder builder;
        builder.StartArray();
        const auto& items = tr_.GetItems();
        for (const auto edge_id : route_info->edges) {
            builder.StartDict();
            const auto& [from, to, weight] = tr_.GetGraph().GetEdge(edge_id);
            total_time += weight;
            builder.Key("type");
            const router::Item& item = items.at(edge_id);
            if (item.type == router::ItemType::WAIT) {
                builder.Value("Wait").Key("stop_name").Value(std::string{item.name});
            } else {
                builder.Value("Bus").Key("bus").Value(std::string{item.name});
                builder.Key("span_count").Value(item.span_count);
            }
            builder.Key("time").Value(weight);
            builder.EndDict();
        }
        return RouteInfo{builder.EndArray().Build().AsArray(), total_time};
    }
}