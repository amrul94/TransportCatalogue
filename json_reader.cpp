#include <algorithm>
#include <fstream>
#include <string>
#include <vector>

#include "json_reader.h"
#include "map_renderer.h"
#include "serialization.h"

namespace transport_catalogue::reader {
    using request_handler::RequestHandler;
    using renderer::MapRenderer;
    using renderer::RenderSettings;
    using router::CustomWeight;
    using router::Item;
    using router::TransportRouter;

    namespace{
        //---------- BaseRequests ----------

        void AddStopInCatalogue(TransportCatalogue& catalogue, const json::Dict& dict) {
            std::string stop_name = dict.at("name").AsString();
            double latitude = dict.at("latitude").AsDouble();
            double longitude = dict.at("longitude").AsDouble();
            catalogue.AddStop(stop_name, {latitude, longitude});
        }

        void RequestToAddStop(TransportCatalogue& catalogue, const json::Dict& dict) {
            const auto found_type = dict.find("type");
            if (found_type != dict.end()) {
                if (found_type->second.AsString() == "Stop") {
                    AddStopInCatalogue(catalogue, dict);
                }
            }
        }

        void AddDistanceBetweenStops(TransportCatalogue& transport_catalog, const json::Dict& dict) {
            const std::string& from_stop_name = dict.at("name").AsString();
            const auto& road_distances = dict.at("road_distances").AsDict();
            for (const auto& [to_stop_name, distance] : road_distances) {
                transport_catalog.AddDistanceBetweenStops(from_stop_name, to_stop_name, distance.AsInt());
            }
        }

        void RequestToAddDistanceBetweenStops(TransportCatalogue& catalogue, const json::Dict& dict) {
            const auto found_type = dict.find("type");
            if (found_type != dict.end()) {
                if (found_type->second.AsString() == "Stop") {
                    AddDistanceBetweenStops(catalogue, dict);
                }
            }
        }

        void AddRouteInCatalogue(TransportCatalogue& catalogue, const json::Dict& dict) {
            std::string bus_name = dict.at("name").AsString();
            const auto& node_stops = dict.at("stops").AsArray();
            bool is_roundtrip = dict.at("is_roundtrip").AsBool();

            std::vector<std::string> stops(0);
            stops.reserve(node_stops.size());
            for (const auto& stop : node_stops) {
                stops.push_back(stop.AsString());
            }

            if (!is_roundtrip) {
                std::vector<std::string> rev_stop(stops.size() - 1);
                std::reverse_copy(stops.begin(), stops.end() - 1, rev_stop.begin());
                stops.insert(stops.end(), std::make_move_iterator(rev_stop.begin()), std::make_move_iterator(rev_stop.end()));
            }

            catalogue.AddRoute(bus_name, stops, is_roundtrip);
        }

        void RequestToAddRouteStop(TransportCatalogue& catalogue, const json::Dict& dict) {
            const auto found_type = dict.find("type");
            if (found_type != dict.end()) {
                if (found_type->second.AsString() == "Bus") {
                    AddRouteInCatalogue(catalogue, dict);
                }
            }
        }


        //--------- RenderSettings ---------

        svg::Color SetColor(const json::Node& color) {
            if (color.IsString()) {
                return color.AsString();
            } else {
                const auto& arr = color.AsArray();
                if (arr.size() == 3) {
                    return svg::Rgb{static_cast<uint8_t>(arr[0].AsInt()),
                                    static_cast<uint8_t>(arr[1].AsInt()),
                                    static_cast<uint8_t>(arr [2].AsInt())};
                } else {
                    return svg::Rgba{static_cast<uint8_t>(arr[0].AsInt()),
                                     static_cast<uint8_t>(arr[1].AsInt()),
                                     static_cast<uint8_t>(arr [2].AsInt()),
                                     arr[3].AsDouble()};
                }
            }
        }

        RenderSettings BuildRenderSettings(const json::Dict& dict) {
            RenderSettings settings;
            settings.width = dict.at("width").AsDouble();
            settings.height = dict.at("height").AsDouble();
            settings.padding = dict.at("padding").AsDouble();
            settings.line_width = dict.at("line_width").AsDouble();
            settings.stop_radius = dict.at("stop_radius").AsDouble();

            const auto& bus_label_offset = dict.at("bus_label_offset").AsArray();
            settings.bus_label_offset = { bus_label_offset[0].AsDouble(), bus_label_offset[1].AsDouble()};
            settings.bus_label_font_size = dict.at("bus_label_font_size").AsInt();

            const auto& stop_label_offset = dict.at(("stop_label_offset")).AsArray();
            settings.stop_label_offset = {stop_label_offset[0].AsDouble(), stop_label_offset[1].AsDouble()};
            settings.stop_label_font_size = dict.at("stop_label_font_size").AsInt();

            settings.underlayer_color = SetColor(dict.at("underlayer_color"));
            settings.underlayer_width = dict.at("underlayer_width").AsDouble();

            for (const auto& color : dict.at("color_palette").AsArray()) {
                settings.color_palette.push_back(SetColor(color));
            }
            return settings;
        }

        std::vector<renderer::RouteRenderer> AddRoutesRenderer(const TransportCatalogue& catalogue, const RenderSettings& settings,
                                                               const renderer::SphereProjector& sphere_projector) {

            const TransportCatalogue::RouteMap& routes = catalogue.GetRoutes();
            std::vector<renderer::RouteRenderer> routes_coordinates;
            routes_coordinates.reserve(routes.size());
            const size_t color_size = settings.color_palette.size();
            size_t number = 0;
            for (const auto& [bus_name, stops_at_route] : routes) {
                std::vector<svg::Point> points;
                if (!stops_at_route.empty()) {
                    if (number >= color_size) {
                        number = 0;
                    }
                    for (const auto& stop : stops_at_route) {
                        points.push_back(sphere_projector(catalogue.FindStop(stop)->coordinates));
                    }
                    routes_coordinates.emplace_back(points, settings.color_palette[number], settings.line_width);
                    ++number;
                }
            }
            return routes_coordinates;
        }

        std::vector<renderer::TextRenderer> AddRoutesNames(const TransportCatalogue& catalogue, const RenderSettings& settings,
                                                           const renderer::SphereProjector& sphere_projector) {
            const TransportCatalogue::RouteMap& routes = catalogue.GetRoutes();
            std::vector<renderer::TextRenderer> routes_names;
            const size_t color_size = settings.color_palette.size();
            size_t number = 0;
            for (const auto& [bus_name, stops_at_route] : routes) {
                if (!stops_at_route.empty()) {
                    if (number >= color_size) {
                        number = 0;
                    }
                    const auto first_stop = catalogue.FindStop(stops_at_route.front());
                    const auto end_stop = catalogue.FindStop(stops_at_route[stops_at_route.size() / 2]);
                    const svg::Point position = sphere_projector(catalogue.FindStop(stops_at_route.front())->coordinates);
                    routes_names.emplace_back(position, settings.bus_label_offset, static_cast<uint32_t>(settings.bus_label_font_size),
                                              "bold", std::string(bus_name), settings.underlayer_color, settings.underlayer_width,
                                              settings.color_palette[number]);
                    if (!catalogue.FindRoute(bus_name)->is_roundtrip && first_stop != end_stop) {
                        const svg::Point back_position = sphere_projector(catalogue.FindStop(stops_at_route[stops_at_route.size() / 2])->coordinates);
                        routes_names.emplace_back(back_position, settings.bus_label_offset, static_cast<uint32_t>(settings.bus_label_font_size),
                                                  "bold", std::string(bus_name), settings.underlayer_color, settings.underlayer_width,
                                                  settings.color_palette[number]);
                    }
                    ++number;
                }

            }
            return routes_names;
        }

        std::vector<renderer::StopRenderer> AddStopsRenderer(const TransportCatalogue& catalogue, const std::set<std::string_view>& ordered_stops,
                                                             const RenderSettings& settings, const renderer::SphereProjector& sphere_projector) {
            std::vector<renderer::StopRenderer> stops_for_draw;
            stops_for_draw.reserve(ordered_stops.size());
            for (const auto& stop : ordered_stops) {
                stops_for_draw.emplace_back(sphere_projector(catalogue.FindStop(stop)->coordinates), settings.stop_radius);
            }
            return stops_for_draw;
        }

        std::vector<renderer::TextRenderer> AddStopsNames(const TransportCatalogue& catalogue, const std::set<std::string_view>& ordered_stops,
                                                          const RenderSettings& settings, const renderer::SphereProjector& sphere_projector) {
            std::vector<renderer::TextRenderer> stops_for_draw;
            stops_for_draw.reserve(ordered_stops.size());
            for (const auto& stop : ordered_stops) {
                stops_for_draw.emplace_back(sphere_projector(catalogue.FindStop(stop)->coordinates), settings.stop_label_offset,
                                            settings.stop_label_font_size, std::nullopt, std::string{stop},
                                            settings.underlayer_color, settings.underlayer_width, "black");
            }
            return stops_for_draw;
        }

        //---------- StatRequests ----------

        json::Node GetBusInfo(const RequestHandler& request_hand, const json::Dict& dict) {
            const std::string& bus_name = dict.at("name").AsString();
            int id = dict.at("id").AsInt();
            domain::Bus* bus_ptr = request_hand.GetBusStat(bus_name);
            json::Builder json_builder;
            json_builder.StartDict().Key("request_id").Value(id);
            if (!bus_ptr) {
                json_builder.Key("error_message").Value(std::string("not found"));
            } else {
                json_builder.Key("curvature").Value(bus_ptr->curvature);
                json_builder.Key("route_length").Value(bus_ptr->route_length);
                json_builder.Key("stop_count").Value(bus_ptr->stops_on_route);
                json_builder.Key("unique_stop_count").Value(bus_ptr->unique_stops);
            }
            return json_builder.EndDict().Build();
        }

        json::Node GetStopInfo(const RequestHandler& request_hand, const json::Dict& dict) {
            const std::string& bus_name = dict.at("name").AsString();
            int id = dict.at("id").AsInt();
            const auto buses_by_stop = request_hand.GetBusesByStop(bus_name);
            json::Builder json_builder;
            json_builder.StartDict().Key("request_id").Value(id);
            if (!buses_by_stop) {
                json_builder.Key("error_message").Value(std::string("not found"));
            } else {
                json_builder.Key("buses").StartArray();
                for (std::string_view bus : *buses_by_stop) {
                    json_builder.Value(std::string(bus));
                }
                json_builder.EndArray();
            }
            return json_builder.EndDict().Build();
        }

        json::Node GetMapInfo(const RequestHandler& request_hand, const json::Dict& dict) {
            svg::Document doc = request_hand.RenderMap();
            int id = dict.at("id").AsInt();
            std::ostringstream strm;

            std::ofstream out("map.svg");
            doc.Render(out);
            doc.Render(strm);
            return json::Builder{}.StartDict().Key("request_id").Value(id).Key("map").Value(strm.str()).EndDict().Build();
        }

        json::Node GetRouteInfo(const RequestHandler& request_hand, const json::Dict& dict) {
            int id = dict.at("id").AsInt();
            const std::string& from = dict.at("from").AsString();
            const std::string& to = dict.at("to").AsString();
            const auto items = request_hand.GetItems(from, to);
            json::Builder json_builder;
            json_builder.StartDict().Key("request_id").Value(id);
            if (!items) {
                json_builder.Key("error_message").Value(std::string("not found"));
            } else {
                json_builder.Key("items").Value(items->items);
                json_builder.Key("total_time").Value(items->total_time);
            }
            return json_builder.EndDict().Build();
        }
    }

    void BaseRequests(TransportCatalogue& catalogue, const json::Array& arr) {
        for (const json::Node& node : arr) {
            RequestToAddStop(catalogue, node.AsDict());
        }

        for (const json::Node& node : arr) {
            RequestToAddDistanceBetweenStops(catalogue, node.AsDict());
        }

        for (const json::Node& node : arr) {
            RequestToAddRouteStop(catalogue, node.AsDict());
        }
    }

    MapRenderer RenderSettingsRequests(const TransportCatalogue& catalogue, const RenderSettings& settings) {
        MapRenderer map_renderer;
        const auto& stops = catalogue.GetStops();
        std::vector<geo::Coordinates> stops_to_draw;
        std::set<std::string_view> ordered_stops;

        for (const auto& stop : stops) {
            if (!catalogue.FindBusesAtStops(stop.name).empty()) {
                stops_to_draw.push_back(stop.coordinates);
                ordered_stops.insert(stop.name);
            }
        }
        renderer::SphereProjector sphere_projector(stops_to_draw.begin(), stops_to_draw.end(),
                                                   settings.width, settings.height, settings.padding);

        map_renderer.SetRoutes(AddRoutesRenderer(catalogue, settings, sphere_projector));
        map_renderer.SetRoutesNames(AddRoutesNames(catalogue, settings, sphere_projector));
        map_renderer.SetStops(AddStopsRenderer(catalogue, ordered_stops, settings, sphere_projector));
        map_renderer.SetStopsNames(AddStopsNames(catalogue, ordered_stops, settings, sphere_projector));
        return map_renderer;
    }

    void RoutingSettingsRequest(TransportRouter& transport_router, const json::Dict& dict) {
        router::Minutes bus_wait_time = dict.at("bus_wait_time").AsDouble();
        transport_router.AddStops(bus_wait_time);

        router::MetersPerMinutes bus_velocity = dict.at("bus_velocity").AsDouble() * 1000. / 60.;
        transport_router.AddRoutes(bus_velocity);
    }

    json::Document StatRequests(const RequestHandler& request_hand, const json::Array& requests) {
        json::Builder json_builder;
        json_builder.StartArray();
        for (const auto& request : requests) {
            json::Node value;
            const auto& dict = request.AsDict();
            const auto found_type = dict.find("type");
            if (found_type != dict.end()) {
                if (found_type->second.AsString() == "Bus") {
                    value = GetBusInfo(request_hand, dict);
                } else if (found_type->second.AsString() == "Stop") {
                    value = GetStopInfo(request_hand, dict);
                } else if (found_type->second.AsString() == "Map") {
                    value = GetMapInfo(request_hand, dict);
                } else if (found_type->second.AsString() == "Route") {
                    value = GetRouteInfo(request_hand, dict);
                }
                json_builder.Value(value.AsDict());
            }
        }
        return json::Document{json_builder.EndArray().Build()};
    }

    void SaveBase(const TransportCatalogue& catalogue, const MapRenderer& map_renderer,
                  const TransportRouter& transport_router, const json::Dict& dict) {
        const auto found_file = dict.find("file");
        if (found_file == dict.end()) {
            return;
        }
        const std::string& path = found_file->second.AsString();
        std::ofstream out(path, std::ios::binary);

        tcs::TransportCatalogue database;
        graph::Router<router::Minutes> router(transport_router.GetGraph());
        *database.mutable_catalogue() = std::move(proto::SaveCatalogue(catalogue));
        *database.mutable_map_renderer() = std::move(proto::SaveMapRenderer(map_renderer));
        *database.mutable_transport_router() = std::move(proto::SaveTransportRouter(transport_router));
        *database.mutable_router() = std::move(proto::SaveRouter(router, transport_router.GetGraph().GetVertexCount()));
        database.SerializeToOstream(&out);
    }

    void MakeBase(TransportCatalogue& catalogue, std::istream& in_json) {
        const auto dict = json::Load(in_json).GetRoot().AsDict();

        const auto found_base_requests = dict.find("base_requests");
        if (found_base_requests != dict.end()) {
            BaseRequests(catalogue, found_base_requests->second.AsArray());
        }

        RenderSettings settings;
        MapRenderer map_renderer;
        const auto found_render_settings = dict.find("render_settings");
        if (found_render_settings != dict.end()) {
            settings = BuildRenderSettings(found_render_settings->second.AsDict());
            map_renderer = RenderSettingsRequests(catalogue, settings);
        }

        TransportRouter transport_router(catalogue);
        const auto found_routing_settings = dict.find("routing_settings");
        if (found_routing_settings != dict.end()) {
            RoutingSettingsRequest(transport_router, found_routing_settings->second.AsDict());
        }

        const auto found_serialization_settings = dict.find("serialization_settings");
        if (found_serialization_settings != dict.end()) {
            SaveBase(catalogue, map_renderer, transport_router, found_serialization_settings->second.AsDict());
        }
    }

    tcs::TransportCatalogue LoadBase(const json::Dict& dict) {
        const auto found_file = dict.find("file");
        tcs::TransportCatalogue database;
        if (found_file == dict.end()) {
            return database;
        }

        const std::string& path = found_file->second.AsString();
        std::ifstream in(path, std::ios::binary);
        database.ParseFromIstream(&in);
        return database;
    }

    void ProcessRequests(std::istream& in, std::ostream& out) {
        const auto dict = json::Load(in).GetRoot().AsDict();

        const auto found_serialization_settings = dict.find("serialization_settings");
        if (found_serialization_settings == dict.end()) {
            return;
        }

        const auto serialization_settings = found_serialization_settings->second.AsDict();
        tcs::TransportCatalogue database = LoadBase(serialization_settings);

        const TransportCatalogue catalogue = proto::LoadCatalogue(*database.mutable_catalogue());
        const MapRenderer map_renderer = proto::LoadMapRenderer(*database.mutable_map_renderer());
        const TransportRouter transport_router = proto::LoadTransportRouter(database.transport_router());
        const graph::Router<router::Minutes>& router = proto::LoadRouter(database.router(), transport_router.GetGraph());
        const request_handler::RequestHandler request_hand { catalogue, map_renderer, transport_router, router };
        const auto found_stat_requests = dict.find("stat_requests");
        if (found_stat_requests != dict.end()) {
            const auto doc = StatRequests(request_hand, found_stat_requests->second.AsArray());
            json::Print(doc, out);
        }
    }
}