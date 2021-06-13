#pragma once

#include <iostream>

#include <map_renderer.pb.h>
#include <transport_catalogue.pb.h>

#include "graph.h"
#include "json.h"
#include "json_builder.h"
#include "request_handler.h"
#include "transport_catalogue.h"
#include "transport_router.h"



namespace transport_catalogue::reader {
    namespace tcs = transport_catalogue_serialize;

    void BaseRequests(TransportCatalogue& catalogue, const json::Array& arr);
    void RenderSettingsRequests(const TransportCatalogue& catalogue, renderer::MapRenderer& map_renderer);
    void RoutingSettingsRequest(router::TransportRouter& transport_router, const json::Dict& dict);
    json::Document StatRequests(const request_handler::RequestHandler& request_hand, const json::Array& arr);

    void SaveBase(const TransportCatalogue& catalogue, const renderer::MapRenderer& map_renderer,
                  const router::TransportRouter& transport_router, const json::Dict& dict);
    void MakeBase(TransportCatalogue& catalogue, std::istream& in_json);

    tcs::TransportCatalogue LoadBase(const json::Dict& dict);
    void ProcessRequests(std::istream& in, std::ostream& out);
}