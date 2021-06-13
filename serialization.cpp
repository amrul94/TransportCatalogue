#include "serialization.h"

#include <fstream>

namespace transport_catalogue::proto {

    namespace {
        using ProtoMap = google::protobuf::Map<std::string, uint64_t>;

        void SaveStops(const TransportCatalogue& source, tcs::Catalogue& destination) {
            auto tcs_stops = destination.mutable_stop();
            for (const domain::Stop& stop : source.GetStops()) {
                tcs::Stop tcs_stop;
                tcs_stop.set_name(stop.name);

                tcs::Coordinates tcs_coordinates;
                tcs_coordinates.set_lat(stop.coordinates.lat);
                tcs_coordinates.set_lng(stop.coordinates.lng);
                *tcs_stop.mutable_coordinates() = std::move(tcs_coordinates);

                tcs_stops->Add(std::move(tcs_stop));
            }
        }

        void SaveDistances(const TransportCatalogue& source, tcs::Catalogue& destination) {
            auto tcs_distances = destination.mutable_distance();

            for (const auto&[stops, distance] : source.GetDistanceBetweenStops()) {
                const auto&[from_stop, to_stop] = stops;
                tcs::DistanceBetweenStops tcs_distance;
                tcs_distance.set_from_stop(from_stop->name);
                tcs_distance.set_to_stop(to_stop->name);
                tcs_distance.set_distance(distance);
                tcs_distances->Add(std::move(tcs_distance));
            }
        }

        void SaveBuses(const TransportCatalogue& source, tcs::Catalogue& destination) {
            auto tcs_buses = destination.mutable_bus();
            for (const auto& [bus_number, stops]: source.GetRoutes()) {
                tcs::Bus tcs_bus;
                domain::Bus* ptr_bus = source.FindRoute(bus_number);
                tcs_bus.set_number(ptr_bus->number);
                tcs_bus.set_is_roundtrip(ptr_bus->is_roundtrip);

                for (const std::string_view stop : stops) {
                    *tcs_bus.add_stop() = stop;
                }

                tcs_buses->Add(std::move(tcs_bus));
            }
        }

        tcs::Point SavePoint(const svg::Point& point) {
            tcs::Point result;
            result.set_x(point.x);
            result.set_y(point.y);
            return result;
        }

        tcs::Color SaveColor(const svg::Color& color) {
            tcs::Color result;
            if (std::holds_alternative<std::string>(color)) {
                std::string str = std::get<std::string>(color);
                result.set_name(str);
            } else {
                auto& tcs_rgba = *result.mutable_rgba();

                if (std::holds_alternative<svg::Rgb>(color)) {
                    svg::Rgb svg_rgb = std::get<svg::Rgb>(color);
                    tcs_rgba.set_red(svg_rgb.red);
                    tcs_rgba.set_blue(svg_rgb.blue);
                    tcs_rgba.set_green(svg_rgb.green);
                    tcs_rgba.set_is_rgba(false);

                } else if (std::holds_alternative<svg::Rgba>(color)) {
                    svg::Rgba svg_rgba = std::get<svg::Rgba>(color);
                    tcs_rgba.set_red(svg_rgba.red);
                    tcs_rgba.set_blue(svg_rgba.blue);
                    tcs_rgba.set_green(svg_rgba.green);
                    tcs_rgba.set_opacity(svg_rgba.opacity);
                    tcs_rgba.set_is_rgba(true);
                }
            }
            return result;
        }

        tcs::RouteRenderer SaveRouteRenderer(const renderer::RouteRenderer& renderer) {
            tcs::RouteRenderer result;
            result.set_line_width(renderer.GetLineWidth());
            *result.mutable_stroke_color() = SaveColor(renderer.GetStrokeColor());

            for (const auto& stop_coordinate : renderer.GetStopCoordinates()) {
                *result.add_stops_coordinates() = std::move(SavePoint(stop_coordinate));
            }
            return result;
        }

        tcs::TextRenderer SaveTextRenderer(const renderer::TextRenderer& renderer) {
            tcs::TextRenderer result;

            *result.mutable_position() = std::move(SavePoint(renderer.GetPosition()));
            *result.mutable_label_offset() = std::move(SavePoint(renderer.GetLabelOffset()));
            result.set_label_font_size(renderer.GetLabelFontSize());

            const auto& font_weight = renderer.GetFontWeight();
            if (font_weight) {
                result.set_has_font_weight(true);
                result.set_font_weight(font_weight.value());
                //std::cout << result.font_weight() << std::endl;
            } else {
                result.set_has_font_weight(false);
            }

            result.set_text(renderer.GetText());
            *result.mutable_underlayer_color() = std::move(SaveColor(renderer.GetUnderlayerColor()));
            result.set_underlayer_width(renderer.GetUnderlayerWidth());
            *result.mutable_text_color() = std::move(SaveColor(renderer.GetTextColor()));

            return result;
        }

        tcs::StopRenderer SaveStopRenderer(const renderer::StopRenderer& renderer) {
            tcs::StopRenderer result;
            *result.mutable_centre() = std::move(SavePoint(renderer.GetCentre()));
            result.set_stop_radius(renderer.GetStopRadius());
            return result;
        }

        tcs::Item SaveItem(const router::Item& item) {
            tcs::Item result;

            switch (item.type) {
                case router::ItemType::WAIT:
                    result.set_type(tcs::Item::WAIT);
                    break;
                case router::ItemType::BUS:
                    result.set_type(tcs::Item::BUS);
                    break;
                default:
                    break;
            }

            result.set_span_count(item.span_count);
            result.set_name(std::string{item.name});
            return result;
        }

        tcs::Edge SaveEdge(const graph::Edge<router::Minutes>& edge) {
            tcs::Edge result;
            result.set_from(edge.from);
            result.set_to(edge.to);
            result.set_weight(edge.weight);
            return result;
        }

        void SaveGraph(const router::TransportRouter::Graph& source, tcs::Graph& destination) {
            destination.set_vertex_count(source.GetVertexCount());
            const graph::EdgeId edges_count = source.GetEdgeCount();

            for (graph::EdgeId id = 0; id < edges_count; ++id) {
                const auto& edge = source.GetEdge(id);
                *destination.add_edge() = std::move(SaveEdge(edge));
            }
        }

        void SaveIndexes(const router::TransportRouter::IndexMap& source, ProtoMap& destination) {
            using MapPair = google::protobuf::MapPair<std::string, uint64_t>;

            for (const auto& [name, index] : source) {
                destination.insert(MapPair {std::string{name}, index});
            }
        }

        void SaveItems(const router::TransportRouter& source, tcs::TransportRouter& destination) {
            for (const auto& item : source.GetItems()) {
                *destination.add_item() = std::move(SaveItem(item));
            }
        }

        void LoadStops(const tcs::Catalogue& source, TransportCatalogue& destination) {
            for (const tcs::Stop& stop : source.stop()) {
                const tcs::Coordinates& coordinates = stop.coordinates();
                destination.AddStop(stop.name(), {coordinates.lat(), coordinates.lng()});
            }
        }

        void LoadDistances(const tcs::Catalogue& source, TransportCatalogue& destination) {
            for (const tcs::DistanceBetweenStops& dbs : source.distance()) {
                destination.AddDistanceBetweenStops(dbs.from_stop(), dbs.to_stop(), dbs.distance());
            }
        }

        void LoadBuses(const tcs::Catalogue& source, TransportCatalogue& destination) {
            for (const tcs::Bus& bus : source.bus()) {
                std::vector<std::string> stops(bus.stop_size());
                std::move(bus.stop().begin(), bus.stop().end(), stops.begin());
                destination.AddRoute(bus.number(), stops, bus.is_roundtrip());
            }
        }

        svg::Point LoadPoint(const tcs::Point& point) {
            svg::Point result;
            result.x = point.x();
            result.y = point.y();
            return result;
        }

        svg::Color LoadColor(const tcs::Color& color) {
            svg::Color result;
            if (color.has_rgba()) {
                if (!color.rgba().is_rgba()) {
                    svg::Rgb svg_rgb;
                    svg_rgb.red = color.rgba().red();
                    svg_rgb.green = color.rgba().green();
                    svg_rgb.blue = color.rgba().blue();
                    result = svg_rgb;
                } else {
                    svg::Rgba svg_rgba;
                    svg_rgba.red = color.rgba().red();
                    svg_rgba.green = color.rgba().green();
                    svg_rgba.blue = color.rgba().blue();
                    svg_rgba.opacity = color.rgba().opacity();
                    result = svg_rgba;
                }
            } else {
                result = color.name();
            }
            return result;
        }

        renderer::RouteRenderer LoadRouteRenderer(const tcs::RouteRenderer& renderer) {
            std::vector<svg::Point> stops_coordinates;
            for (const auto& stop_coordinate : renderer.stops_coordinates()) {
                stops_coordinates.push_back(LoadPoint(stop_coordinate));
            }

            svg::Color stroke_color = LoadColor(renderer.stroke_color());

            return {std::move(stops_coordinates), std::move(stroke_color), renderer.line_width()};
        }

        renderer::TextRenderer LoadTextRenderer(const tcs::TextRenderer& renderer) {
            svg::Point position = LoadPoint(renderer.position());
            svg::Point label_offset = LoadPoint(renderer.label_offset());

            std::optional<std::string> font_weight;
            if(renderer.has_font_weight()) {
                font_weight = renderer.font_weight();
            }

            svg::Color underlayer_color = LoadColor(renderer.underlayer_color());
            svg::Color text_color = LoadColor(renderer.text_color());

            return {position, label_offset, renderer.label_font_size(), font_weight, renderer.text(),
                    underlayer_color, renderer.underlayer_width(), text_color};
        }

        renderer::StopRenderer LoadStopRenderer(const tcs::StopRenderer& renderer) {
            return {LoadPoint(renderer.centre()), renderer.stop_radius()};
        }

        std::vector<renderer::RouteRenderer> LoadRoutesRenderer(const tcs::MapRenderer& map_renderer) {
            std::vector<renderer::RouteRenderer> routes;
            routes.reserve(map_renderer.routes_size());
            for (const auto& route : map_renderer.routes()) {
                routes.push_back(std::move(LoadRouteRenderer(route)));
            }
            return routes;
        }

        std::vector<renderer::TextRenderer> LoadRoutesNames(const tcs::MapRenderer& map_renderer) {
            std::vector<renderer::TextRenderer> routes_names;
            routes_names.reserve(map_renderer.routes_names_size());
            for (const auto& text : map_renderer.routes_names()) {
                routes_names.push_back(std::move(LoadTextRenderer(text)));
            }
            return routes_names;
        }

        std::vector<renderer::StopRenderer> LoadStopsRenderer(const tcs::MapRenderer& map_renderer) {
            std::vector<renderer::StopRenderer> stops;
            stops.reserve(map_renderer.stops_size());
            for (const auto& stop : map_renderer.stops()) {
                stops.push_back(std::move(LoadStopRenderer(stop)));
            }
            return stops;
        }

        std::vector<renderer::TextRenderer> LoadStopsNames(const tcs::MapRenderer& map_renderer) {
            std::vector<renderer::TextRenderer> stops_names;
            stops_names.reserve(map_renderer.stops_names_size());
            for (const auto& text : map_renderer.stops_names()) {
                stops_names.push_back(std::move(LoadTextRenderer(text)));
            }
            return stops_names;
        }

        router::Item LoadItem(const tcs::Item& item) {
            router::ItemType type;
            switch (item.type()) {
                case tcs::Item::WAIT:
                    type = router::ItemType::WAIT;
                    break;
                case tcs::Item::BUS:
                    type = router::ItemType::BUS;
                    break;
                default:
                    assert(false);
            }
            return {type, item.span_count(), item.name()};
        }

        void LoadGraph(const tcs::TransportRouter& source, router::TransportRouter& destination) {
            for (const auto& edge : source.graph().edge()) {
                destination.AddEdge({edge.from(), edge.to(), edge.weight()});
            }
        }

        void LoadIndexes(const tcs::TransportRouter& source, router::TransportRouter& destination) {
            for(const auto& [name, index] : source.wait_index()) {
                destination.AddWaitIndex(name, index);
            }
        }

        void LoadItems(const tcs::TransportRouter& source, router::TransportRouter& destination) {
            for(const auto& item : source.item()) {
                router::Item added = LoadItem(item);
                destination.AddItem(added);
            }
        }
    }

    tcs::Catalogue SaveCatalogue(const TransportCatalogue& catalogue) {
        tcs::Catalogue result;
        SaveBuses(catalogue, result);
        SaveStops(catalogue, result);
        SaveDistances(catalogue, result);
        return result;
    }

    tcs::MapRenderer SaveMapRenderer(const renderer::MapRenderer& map_renderer) {
        tcs::MapRenderer result;

        for (const auto& route : map_renderer.GetRoutes()) {
            *result.add_routes() = std::move(SaveRouteRenderer(route));
        }

        for (const auto& text : map_renderer.GetRoutesNames()) {
            *result.add_routes_names() = std::move(SaveTextRenderer(text));
        }

        for (const auto& stop : map_renderer.GetStops()) {
            *result.add_stops() = std::move(SaveStopRenderer(stop));
        }

        for (const auto& text : map_renderer.GetStopsNames()) {
            *result.add_stops_names() = std::move(SaveTextRenderer(text));
        }

        return result;
    }

    tcs::TransportRouter SaveTransportRouter(const router::TransportRouter& router) {
        tcs::TransportRouter result;
        SaveGraph(router.GetGraph(), *result.mutable_graph());
        SaveIndexes(router.GetWaitIndexes(), *result.mutable_wait_index());
        SaveItems(router, result);
        return result;
    }

    tcs::Router SaveRouter(const graph::Router<router::Minutes>& router, graph::VertexId vertex_count) {
        tcs::Router result;
        for (graph::VertexId from = 0; from < vertex_count; ++from) {
            auto& routes_internal_data = *result.add_routes_internal_data();
            for (graph::VertexId to = 0; to < vertex_count; ++to) {
                const auto& route_info = router.GetData(from, to);
                auto& data = *routes_internal_data.add_data();
                if (route_info) {
                    data.set_has_data(true);
                    data.set_weight(route_info->weight);
                    if (route_info->prev_edge) {
                        data.set_has_prev_edge(true);
                        data.set_prev_edge(*route_info->prev_edge);
                    } else {
                        data.set_has_prev_edge(false);
                    }
                } else {
                    data.set_has_data(false);
                }
            }
        }
        return result;
    }

    TransportCatalogue LoadCatalogue(const tcs::Catalogue& catalogue) {
        TransportCatalogue result;
        LoadStops(catalogue, result);
        LoadDistances(catalogue, result);
        LoadBuses(catalogue, result);
        return result;
    }

    renderer::MapRenderer LoadMapRenderer(const tcs::MapRenderer& map_renderer) {
        renderer::MapRenderer result;
        result.SetRoutes(std::move(LoadRoutesRenderer(map_renderer)));
        result.SetRoutesNames(std::move(LoadRoutesNames(map_renderer)));
        result.SetStops(std::move(LoadStopsRenderer(map_renderer)));
        result.SetStopsNames(std::move(LoadStopsNames(map_renderer)));
        return result;
    }

    router::TransportRouter LoadTransportRouter(const tcs::TransportRouter& router) {
        size_t vertex_count = router.graph().vertex_count();
        router::TransportRouter result(vertex_count);
        LoadGraph(router, result);
        LoadIndexes(router, result);
        LoadItems(router, result);
        return result;
    }

    graph::Router<router::Minutes> LoadRouter(const tcs::Router& router, const router::TransportRouter::Graph& graph) {
        using Helper = graph::RouterCreatorHelper<router::Minutes>;
        Helper helper(graph);

        for (graph::VertexId from = 0; from < router.routes_internal_data_size(); ++from) {
            for (graph::VertexId to = 0; to < router.routes_internal_data(from).data_size(); ++to) {
                const auto& route_internal_data = router.routes_internal_data(from).data(to);
                if (route_internal_data.has_data()) {
                    Helper::RouteInternalData data;
                    data.weight = route_internal_data.weight();
                    if (route_internal_data.has_prev_edge()) {
                        data.prev_edge = route_internal_data.prev_edge();
                    } else {
                        data.prev_edge = std::nullopt;
                    }
                    helper.AddData(from, to, data);
                } else {
                    helper.AddData(from, to, std::nullopt);
                }
            }
        }
        return helper.BuildRouter();
    }
}