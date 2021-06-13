#include "svg.h"

namespace svg {

    using namespace std::literals;

    void Object::Render(const RenderContext& context) const {
        context.RenderIndent();

        // Делегируем вывод тега своим подклассам
        RenderObject(context);

        context.out << std::endl;
    }

    std::ostream& operator<<(std::ostream& out, const Rgb& rgb) {
        out << "rgb(" << static_cast<int>(rgb.red)
            << "," << static_cast<int>(rgb.green)
            << "," << static_cast<int>(rgb.blue) << ")";
        return out;
    }

    std::ostream& operator<<(std::ostream& out, const Rgba& rgba) {
        out << "rgba(" << static_cast<int>(rgba.red)
            << "," << static_cast<int>(rgba.green)
            << "," << static_cast<int>(rgba.blue)
            << "," << rgba.opacity << ")";

        return out;
    }

    std::ostream& operator<<(std::ostream& out, StrokeLineCap line_cap) {
        switch (line_cap) {
            case StrokeLineCap::BUTT:
                out << "butt";
                break;
            case StrokeLineCap::ROUND:
                out << "round";
                break;
            case StrokeLineCap::SQUARE:
                out << "square";
                break;
            default: break;
        }
        return out;
    }

    std::ostream& operator<<(std::ostream& out, StrokeLineJoin line_join) {
        switch (line_join) {
            case StrokeLineJoin::ARCS:
                out << "arcs";
                break;
            case StrokeLineJoin::BEVEL:
                out << "bevel";
                break;
            case StrokeLineJoin::MITER:
                out << "miter";
                break;
            case StrokeLineJoin::MITER_CLIP:
                out << "miter-clip";
                break;
            case StrokeLineJoin::ROUND:
                out << "round";
                break;
            default:
                break;
        }
        return out;
    }

// ---------- Circle ------------------

    Circle& Circle::SetCenter(Point center)  {
        center_ = center;
        return *this;
    }

    Circle& Circle::SetRadius(double radius)  {
        radius_ = radius;
        return *this;
    }

    void Circle::RenderObject(const RenderContext& context) const {
        auto& out = context.out;
        out << R"(<circle cx=")" << center_.x << R"(" cy=")" << center_.y << R"(" )";
        out << R"(r=")" << radius_ << R"(" )";
        // Выводим атрибуты, унаследованные от PathProps
        RenderAttrs(out);
        out << "/>"sv;
    }

// --------- Polyline -----------------

    Polyline& Polyline::AddPoint(Point point) {
        points_.push_back(point);
        return *this;
    }

    void Polyline::RenderObject(const RenderContext& context) const {
        auto& out = context.out;
        int size = points_.size();
        out << R"(<polyline points=")";
        for (const auto [x, y] : points_) {
            if (size > 1) {
                out << x << "," << y << " ";
            } else {
                out << x << "," << y;
            }
            --size;
        }
        out  << "\" ";
        RenderAttrs(out);
        out << "/>"sv;
    }

// ----------- Text -------------------

    // Задаёт координаты опорной точки (атрибуты x и y)
    Text& Text::SetPosition(Point pos) {
        position_ = pos;
        return *this;
    }

    // Задаёт смещение относительно опорной точки (атрибуты dx, dy)
    Text& Text::SetOffset(Point offset) {
        offset_ = offset;
        return *this;
    }

    // Задаёт размеры шрифта (атрибут font-size)
    Text& Text::SetFontSize(uint32_t size) {
        font_size_ = size;
        return *this;
    }

    // Задаёт название шрифта (атрибут font-family)
    Text& Text::SetFontFamily(std::string font_family) {
        font_family_ = std::move(font_family);
        return *this;
    }

    // Задаёт толщину шрифта (атрибут font-weight)
    Text& Text::SetFontWeight(std::string font_weight) {
        font_weight_ = std::move(font_weight);
        return *this;
    }

    // Задаёт текстовое содержимое объекта (отображается внутри тега text)
    Text& Text::SetData(std::string data) {
        data_ = std::move(data);
        return *this;
    }

    // Экранизирует символы
    void Text::ReplaceCharacterInString(std::string& text, char symbol, const std::string& new_symbol) {
        for (auto pos = text.find(symbol); pos != std::string::npos; pos = text.find(symbol, pos + 1)) {
            text = text.substr(0, pos).append(new_symbol).append(text.substr(pos + 1));
        }
    }

    // Вторая версия функции для экранизации символов
    std::string& Text::FormatDataForOutput(std::string& text) {
        ReplaceCharacterInString(text, '&', "&amp;"s);
        ReplaceCharacterInString(text, '\"', "&quot;"s);
        ReplaceCharacterInString(text, '\'', "&apos;"s);
        ReplaceCharacterInString(text, '<', "&lt;"s);
        ReplaceCharacterInString(text, '>', "&gt;"s);
        return text;
    }

    void Text::RenderObject(const RenderContext& context) const {
        auto& out = context.out;

        out << "<text ";
        RenderAttrs(out);
        out << R"( x=")" << position_.x << R"(" y=")" << position_.y << R"(" )";
        out << R"(dx=")" << offset_.x << R"(" dy=")" << offset_.y << R"(" )";



        out << R"(font-size=")" << font_size_ << '\"';
        if (!font_family_.empty()) {
            out << R"( font-family=")" << font_family_ << '\"';
        }
        if (!font_weight_.empty()) {
            out << R"( font-weight=")" << font_weight_ << '\"';
        }
        auto text = data_;
        out << '>' << FormatDataForOutput(text) << "</text>"sv;
    }

// --------- Document -----------------

    void Document::AddPtr(std::unique_ptr<Object>&& obj) {
        objects_.push_back(std::move(obj));
    }

    void Document::Render(std::ostream &out) const {
        out << R"(<?xml version="1.0" encoding="UTF-8" ?>)" << std::endl;
        out << R"(<svg xmlns="http://www.w3.org/2000/svg" version="1.1">)" << std::endl;
        RenderContext render_context(out);
        for (const auto& obj : objects_) {
            out << "  ";
            obj->Render(render_context);
        }
        out << "</svg>";
    }

}  // namespace svg