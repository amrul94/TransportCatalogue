#pragma once

#include <map_renderer.pb.h>
#include <transport_catalogue.pb.h>
#include <transport_router.pb.h>

#include "map_renderer.h"
#include "transport_catalogue.h"
#include "transport_router.h"

namespace transport_catalogue::proto {
    namespace tcs = transport_catalogue_serialize;

    tcs::Catalogue SaveCatalogue(const TransportCatalogue& catalogue);
    tcs::MapRenderer SaveMapRenderer(const renderer::MapRenderer& map_renderer);
    tcs::TransportRouter SaveTransportRouter(const router::TransportRouter& router);
    tcs::Router SaveRouter(const graph::Router<router::Minutes>& router, graph::VertexId vertex_count);

    TransportCatalogue LoadCatalogue(const tcs::Catalogue& catalogue);
    renderer::MapRenderer LoadMapRenderer(const tcs::MapRenderer& map_renderer);
    router::TransportRouter LoadTransportRouter(const tcs::TransportRouter& router);
    graph::Router<router::Minutes> LoadRouter(const tcs::Router& router, const router::TransportRouter::Graph& graph);
}