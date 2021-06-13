#pragma once

#include <vector>

#include <algorithm>
#include <map>
#include "domain.h"
#include "geo.h"
#include "svg.h"

namespace transport_catalogue::renderer {

    inline const double EPSILON = 1e-6;
    bool IsZero(double value);

    class SphereProjector {
    public:
        template <typename PointInputIt>
        SphereProjector(PointInputIt points_begin, PointInputIt points_end, double max_width,
                        double max_height, double padding)
                : padding_(padding) {
            if (points_begin == points_end) {
                return;
            }

            const auto [left_it, right_it]
            = std::minmax_element(points_begin, points_end, [](auto lhs, auto rhs) {
                return lhs.lng < rhs.lng;
            });
            min_lon_ = left_it->lng;
            const double max_lon = right_it->lng;

            const auto [bottom_it, top_it]
            = std::minmax_element(points_begin, points_end, [](auto lhs, auto rhs) {
                return lhs.lat < rhs.lat;
            });
            const double min_lat = bottom_it->lat;
            max_lat_ = top_it->lat;

            std::optional<double> width_zoom;
            if (!IsZero(max_lon - min_lon_)) {
                width_zoom = (max_width - 2 * padding) / (max_lon - min_lon_);
            }

            std::optional<double> height_zoom;
            if (!IsZero(max_lat_ - min_lat)) {
                height_zoom = (max_height - 2 * padding) / (max_lat_ - min_lat);
            }

            if (width_zoom && height_zoom) {
                zoom_coeff_ = std::min(*width_zoom, *height_zoom);
            } else if (width_zoom) {
                zoom_coeff_ = *width_zoom;
            } else if (height_zoom) {
                zoom_coeff_ = *height_zoom;
            }
        }

        svg::Point operator()(geo::Coordinates coords) const {
            return {(coords.lng - min_lon_) * zoom_coeff_ + padding_,
                    (max_lat_ - coords.lat) * zoom_coeff_ + padding_};
        }

    private:
        double padding_;
        double min_lon_ = 0.;
        double max_lat_ = 0.;
        double zoom_coeff_ = 0.;
    };

    struct RenderSettings {
        // Задают ширину и высоту области карты в пикселях
        double width = 0.;
        double height = 0.;

        // Отступ краёв карты от границ SVG-документа
        double padding = 0.;

        // Толщина линий, которыми рисуются автобусные маршруты
        double line_width = 0.;

        // Радиус окружностей, которыми обозначаются остановки
        double stop_radius = 0.;

        // Задает размер шрифта текста, которым
        // отображаются названия автобусных маршрутов
        int bus_label_font_size = 0.;

        // Задает смещение надписи с названием автобусного маршрута
        // относительно координат конечной остановки на карте.
        // Задаёт значения свойств dx и dy SVG-элемента <text>
        svg::Point bus_label_offset;

        // Задает размер шрифта текста, которым отображаются названия остановок
        int stop_label_font_size = 0.;

        // Задает смещение надписи с названием остановки
        // относительно координат остановки на карте.
        // Задаёт значения свойств dx и dy SVG-элемента <text>
        svg::Point stop_label_offset;

        // Цвет подложки, отображаемой под названиями
        // остановок и автобусных маршрутов.
        svg::Color underlayer_color;

        // Толщина подложки под названиями остановок и автобусных маршрутов.
        // Задаёт значение атрибута stroke-width элемента <text>
        double underlayer_width = 0.;

        // Массив, элементы которого задают цветовую палитру,
        // используемую для визуализации маршрутов.
        std::vector<svg::Color> color_palette;
    };

    class RouteRenderer : svg::Drawable {
    public:
        RouteRenderer(std::vector<svg::Point> stops_coordinates, svg::Color stroke_color, double line_width);

        void Draw(svg::ObjectContainer& container) const override;

        [[nodiscard]] const std::vector<svg::Point>& GetStopCoordinates() const;
        [[nodiscard]] const svg::Color& GetStrokeColor() const;
        [[nodiscard]] double GetLineWidth() const;

    private:
        [[nodiscard]] svg::Polyline CreateRoute() const;

        const std::vector<svg::Point> stops_coordinates_;
        svg::Color stroke_color_;
        double line_width_;
    };

    class TextRenderer : svg::Drawable {
    public:
        TextRenderer(const svg::Point& position, const svg::Point& offset, uint32_t font_size,
                     std::optional<std::string> font_weight, std::string bus_name,
                     svg::Color underlayer_color, double underlayer_width, svg::Color text_color);

        void Draw(svg::ObjectContainer& container) const override;

        [[nodiscard]] const svg::Point& GetPosition() const;
        [[nodiscard]] const svg::Point& GetLabelOffset() const;
        [[nodiscard]] uint32_t GetLabelFontSize() const;
        [[nodiscard]] const std::optional<std::string>& GetFontWeight() const;
        [[nodiscard]] const std::string& GetText() const;

        [[nodiscard]] const svg::Color& GetUnderlayerColor() const;
        [[nodiscard]] double GetUnderlayerWidth() const;

        [[nodiscard]] const svg::Color& GetTextColor() const;

    private:
        [[nodiscard]] svg::Text CreatAllText() const;
        [[nodiscard]] svg::Text CreatMainText() const;
        [[nodiscard]] svg::Text CreatUnderlayer() const;

        // Общие свойства
        svg::Point position_;
        svg::Point label_offset_;
        uint32_t label_font_size_ = 0;
        std::optional<std::string> font_weight_;
        std::string text_;

        // Свойства подложки
        svg::Color underlayer_color_;
        double underlayer_width_;

        // Свойства надписи
        svg::Color text_color_;
        
    };

    class StopRenderer : svg::Drawable {
    public:
        StopRenderer(const svg::Point& centre, double stop_radius);

        void Draw(svg::ObjectContainer& container) const override;

        [[nodiscard]] const svg::Point& GetCentre() const;
        [[nodiscard]] double GetStopRadius() const;

    private:
        [[nodiscard]] svg::Circle CreatStop() const;

        svg::Point centre_;
        double stop_radius_ = 0.;
    };

    class MapRenderer {
    public:

        [[nodiscard]] svg::Document Render() const;

        void SetRoutes(std::vector<RouteRenderer>&& routes);
        void SetRoutesNames(std::vector<TextRenderer>&& routes_names);
        void SetStops(std::vector<renderer::StopRenderer>&& stops);
        void SetStopsNames(std::vector<TextRenderer>&& stops_names);

        [[nodiscard]] const std::vector<RouteRenderer>& GetRoutes() const;
        [[nodiscard]] const std::vector<TextRenderer>& GetRoutesNames() const;
        [[nodiscard]] const std::vector<StopRenderer>& GetStops() const;
        [[nodiscard]] const std::vector<TextRenderer>& GetStopsNames() const;

    private:
        std::vector<RouteRenderer> routes_;
        std::vector<TextRenderer> routes_names_;
        std::vector<StopRenderer> stops_;
        std::vector<TextRenderer> stops_names_;
    };
}