#include <utility>
#include "map_renderer.h"

namespace transport_catalogue::renderer {

    bool IsZero(double value) {
        return std::abs(value) < EPSILON;
    }

// ---------- RouteRenderer -------------------

    RouteRenderer::RouteRenderer(std::vector<svg::Point> stops_coordinates, svg::Color stroke_color, double line_width)
        : stops_coordinates_(std::move(stops_coordinates))
        , stroke_color_(std::move(stroke_color))
        , line_width_(line_width) {
    }

    void RouteRenderer::Draw(svg::ObjectContainer& container) const {
        container.Add(CreateRoute());
    }

    const std::vector<svg::Point>& RouteRenderer::GetStopCoordinates() const {
        return stops_coordinates_;
    }

    const svg::Color& RouteRenderer::GetStrokeColor() const {
        return stroke_color_;
    }

    double RouteRenderer::GetLineWidth() const {
        return line_width_;
    }

    svg::Polyline RouteRenderer::CreateRoute() const {
        using namespace svg;
        Polyline polyline;
        for (const auto& coordinates : stops_coordinates_) {
            polyline.AddPoint(coordinates);
        }
        polyline.SetStrokeColor(stroke_color_).SetFillColor(svg::NoneColor);
        polyline.SetStrokeWidth(line_width_);
        polyline.SetStrokeLineCap(StrokeLineCap::ROUND);
        polyline.SetStrokeLineJoin(StrokeLineJoin::ROUND);
        return polyline;
    }

// -------- TextRenderer -----------------

    TextRenderer::TextRenderer(const svg::Point& position, const svg::Point& offset, uint32_t font_size,
                               std::optional<std::string> font_weight, std::string bus_name, svg::Color underlayer_color,
                               double underlayer_width, svg::Color text_color)
              : position_(position)
              , label_offset_(offset)
              , label_font_size_(font_size)
              , font_weight_(std::move(font_weight))
              , text_(std::move(bus_name))
              , underlayer_color_(std::move(underlayer_color))
              , underlayer_width_(underlayer_width)
              , text_color_(std::move(text_color)) {
    }

    void TextRenderer::Draw(svg::ObjectContainer& container) const {
        container.Add(CreatUnderlayer());
        container.Add(CreatMainText());
    }

    const svg::Point& TextRenderer::GetPosition() const {
        return position_;
    }
    const svg::Point& TextRenderer::GetLabelOffset() const {
        return label_offset_;
    }

    uint32_t TextRenderer::GetLabelFontSize() const {
        return label_font_size_;
    }

    const std::optional<std::string>& TextRenderer::GetFontWeight() const {
        return font_weight_;
    }

    const std::string& TextRenderer::GetText() const {
        return text_;
    }

    const svg::Color& TextRenderer::GetUnderlayerColor() const {
        return underlayer_color_;
    }

    double TextRenderer::GetUnderlayerWidth() const {
        return underlayer_width_;
    }

    const svg::Color& TextRenderer::GetTextColor() const {
        return text_color_;
    }

    svg::Text TextRenderer::CreatAllText() const{
        svg::Text text;
        text.SetPosition(position_).SetOffset(label_offset_);
        text.SetFontSize(label_font_size_).SetFontFamily("Verdana");
        if (font_weight_) {
            text.SetFontWeight(*font_weight_);
        }
        text.SetData(text_);
        return text;
    }

    svg::Text TextRenderer::CreatMainText() const {
        return CreatAllText().SetFillColor(text_color_);
    }

    svg::Text TextRenderer::CreatUnderlayer() const {
        svg::Text text {CreatAllText()};
        text.SetFillColor(underlayer_color_).SetStrokeColor(underlayer_color_);
        text.SetStrokeWidth(underlayer_width_);
        text.SetStrokeLineCap(svg::StrokeLineCap::ROUND);
        text.SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);
        return text;
    }

// ----------- RenderStop -------------------

    StopRenderer::StopRenderer(const svg::Point& centre, double stop_radius)
        : centre_(centre)
        , stop_radius_(stop_radius) {
    }

    void StopRenderer::Draw(svg::ObjectContainer& container) const {
        container.Add(CreatStop());
    }

    const svg::Point& StopRenderer::GetCentre() const {
        return centre_;
    }

    double StopRenderer::GetStopRadius() const {
        return stop_radius_;
    }

    svg::Circle StopRenderer::CreatStop() const {
        svg::Circle circle;
        circle.SetCenter(centre_).SetRadius(stop_radius_).SetFillColor("white");
        return circle;
    }

// ------- MapRenderer ----------------

    svg::Document MapRenderer::Render() const {
        svg::Document doc;
        for (const auto& route : routes_) {
            route.Draw(doc);
        }

        for (const auto& text : routes_names_) {
            text.Draw(doc);
        }

        for (const auto& stop : stops_) {
            stop.Draw(doc);
        }

        for (const auto& text : stops_names_) {
            text.Draw(doc);
        }
        return doc;
    }

    void MapRenderer::SetRoutes(std::vector<RouteRenderer>&& routes) {
        routes_ = std::move(routes);
    }

    void MapRenderer::SetRoutesNames(std::vector<TextRenderer>&& routes_names) {
        routes_names_ = std::move(routes_names);
    }

    void MapRenderer::SetStops(std::vector<renderer::StopRenderer>&& stops) {
        stops_ = std::move(stops);
    }

    void MapRenderer::SetStopsNames(std::vector<TextRenderer>&& stops_names) {
        stops_names_ = std::move(stops_names);
    }

    const std::vector<RouteRenderer>& MapRenderer::GetRoutes() const {
        return routes_;
    }

    const std::vector<TextRenderer>& MapRenderer::GetRoutesNames() const {
        return routes_names_;
    }

    const std::vector<StopRenderer>& MapRenderer::GetStops() const {
        return stops_;
    }

    const std::vector<TextRenderer>& MapRenderer::GetStopsNames() const {
        return stops_names_;
    }
}