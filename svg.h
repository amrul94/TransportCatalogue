#pragma once

#define _USE_MATH_DEFINES
#include <cmath>
#include <cstdint>
#include <deque>
#include <iosfwd>
#include <iostream>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <variant>
#include <vector>

namespace svg {

    struct Point {
        Point() = default;
        Point(double x, double y)
                : x(x)
                , y(y) {
        }
        double x = 0;
        double y = 0;
    };

/*
    // Вспомогательная структура, хранящая контекст для вывода SVG-документа с отступами.
    // Хранит ссылку на поток вывода, текущее значение и шаг отступа при выводе элемента
*/
    struct RenderContext {
        explicit RenderContext(std::ostream& out)
                : out(out) {
        }

        [[maybe_unused]] RenderContext(std::ostream& out, int indent_step, int indent = 0)
                : out(out)
                , indent_step(indent_step)
                , indent(indent) {
        }

        [[nodiscard]] [[maybe_unused]] RenderContext Indented() const {
            return {out, indent_step, indent + indent_step};
        }

        void RenderIndent() const {
            for (int i = 0; i < indent; ++i) {
                out.put(' ');
            }
        }

        std::ostream& out;
        int indent_step = 0;
        int indent = 0;
    };

    struct Rgb {
        Rgb() = default;

        Rgb(uint8_t red, uint8_t green, uint8_t blue)
            : red(red)
            , green(green)
            , blue(blue) {
        }

        uint8_t red = 0;
        uint8_t green = 0;
        uint8_t blue = 0;
    };

    std::ostream& operator<<(std::ostream& out, const Rgb& color);

    struct Rgba {
        Rgba() = default;

        Rgba(uint8_t red, uint8_t green, uint8_t blue, double opacity = 0)
                : red(red)
                , green(green)
                , blue(blue)
                , opacity(opacity) {
        }

        uint8_t red = 0;
        uint8_t green = 0;
        uint8_t blue = 0;
        double opacity = 1;
    };

    std::ostream& operator<<(std::ostream& out, const Rgba& color);

    using Color = std::variant<std::monostate, std::string, Rgb, Rgba>;

    // Объявив в заголовочном файле константу со спецификатором inline,
    // мы сделаем так, что она будет одной на все единицы трансляции,
    // которые подключают этот заголовок.
    // В противном случае каждая единица трансляции будет использовать свою копию этой константы
    inline const Color NoneColor{};

    struct OstreamColorPrinter {
        explicit OstreamColorPrinter(std::ostream& out)
            : out(out) {
        }

        void operator()(std::monostate) const {
            out << "none";
        }
        void operator()(const std::string& color) const {
            out << color;
        }
        void operator()(Rgb color) const {
            out << color;
        }
        void operator()(Rgba color) const {
            out << color;
        }

        std::ostream& out;
    };

/*
 * Абстрактный базовый класс Object служит для унифицированного хранения
 * конкретных тегов SVG-документа
 * Реализует паттерн "Шаблонный метод" для вывода содержимого тега
 */
    class Object {
    public:
        void Render(const RenderContext& context) const;

        virtual ~Object() = default;

    private:
        virtual void RenderObject(const RenderContext& context) const = 0;
    };


    enum class StrokeLineCap {
        BUTT,
        ROUND,
        SQUARE,
    };

    std::ostream& operator<<(std::ostream& out, StrokeLineCap line_cap);

    enum class StrokeLineJoin {
        ARCS,
        BEVEL,
        MITER,
        MITER_CLIP,
        ROUND,
    };

    std::ostream& operator<<(std::ostream& out, StrokeLineJoin line_join);

/*
 * Вспомогательный базовый класс svg::PathProps, который
 * задает цвет заливки и обводки фигур
 */
    template <typename Owner>
    class PathProps {
    public:
        Owner& SetFillColor(Color color) {
            fill_color_ = std::move(color);
            return AsOwner();
        }
        Owner& SetStrokeColor(Color color) {
            stroke_color_ = std::move(color);
            return AsOwner();
        }

        Owner& SetStrokeWidth(double width) {
            stroke_width_ = width;
            return AsOwner();
        }

        Owner& SetStrokeLineCap(StrokeLineCap line_cap) {
            stroke_linecap_ = line_cap;
            return AsOwner();
        }

        Owner& SetStrokeLineJoin(StrokeLineJoin line_join) {
            stroke_linejoin_ = line_join;
            return AsOwner();
        }

    protected:
        ~PathProps() = default;

        void RenderAttrs(std::ostream& out) const {
            using namespace std::literals;

            if (fill_color_) {
                out << R"(fill=")";
                std::visit(OstreamColorPrinter{out}, *fill_color_);
                out << '\"';
            }
            if (stroke_color_) {
                out << R"( stroke=")";
                std::visit(OstreamColorPrinter{out}, *stroke_color_ );
                out << '\"';
            }
            if (stroke_width_) {
                out << R"( stroke-width=")" << *stroke_width_ << '\"';
            }
            if (stroke_linecap_) {
                out << R"( stroke-linecap=")" << *stroke_linecap_ << '\"';
            }
            if (stroke_linejoin_) {
                out << R"( stroke-linejoin=")" << *stroke_linejoin_ << '\"';
            }

        }

    private:
        Owner& AsOwner() {
            // static_cast безопасно преобразует *this к Owner&,
            // если класс Owner — наследник PathProps
            return static_cast<Owner&>(*this);
        }

        std::optional<Color> fill_color_;
        std::optional<Color> stroke_color_;
        std::optional<double> stroke_width_;
        std::optional<StrokeLineCap> stroke_linecap_;
        std::optional<StrokeLineJoin> stroke_linejoin_;

    };

/*
 * Класс Circle моделирует элемент <circle> для отображения круга
 * https://developer.mozilla.org/en-US/docs/Web/SVG/Element/circle
 */
    class Circle final : public Object, public PathProps<Circle> {
    public:
        Circle& SetCenter(Point center);
        Circle& SetRadius(double radius);

    private:
        void RenderObject(const RenderContext& context) const override;

        Point center_;
        double radius_ = 1.0;
    };

/*
 * Класс Polyline моделирует элемент <polyline> для отображения ломаных линий
 * https://developer.mozilla.org/en-US/docs/Web/SVG/Element/polyline
 */
    class Polyline final : public Object, public PathProps<Polyline> {
    public:
        // Добавляет очередную вершину к ломаной линии
        Polyline& AddPoint(Point point);

    private:
        void RenderObject(const RenderContext& context) const override;

        std::vector<Point> points_;
    };

/*
 * Класс Text моделирует элемент <text> для отображения текста
 * https://developer.mozilla.org/en-US/docs/Web/SVG/Element/text
 */
    class Text final : public Object, public PathProps<Text> {
    public:
        // Задаёт координаты опорной точки (атрибуты x и y)
        Text& SetPosition(Point pos);

        // Задаёт смещение относительно опорной точки (атрибуты dx, dy)
        Text& SetOffset(Point offset);

        // Задаёт размеры шрифта (атрибут font-size)
        Text& SetFontSize(uint32_t size);

        // Задаёт название шрифта (атрибут font-family)
        Text& SetFontFamily(std::string font_family);

        // Задаёт толщину шрифта (атрибут font-weight)
        Text& SetFontWeight(std::string font_weight);

        // Задаёт текстовое содержимое объекта (отображается внутри тега text)
        Text& SetData(std::string data);

        // Прочие данные и методы, необходимые для реализации элемента <text>
    private:

        // Экранизирует определенный символ
        static void ReplaceCharacterInString(std::string& text, char symbol, const std::string& new_symbol);

        // Экранизирует все символы
        static std::string& FormatDataForOutput(std::string& text);

        void RenderObject(const RenderContext& context) const override;

        Point position_;
        Point offset_;
        uint32_t font_size_ = 1;
        std::string font_family_;
        std::string font_weight_;
        std::string data_;
    };

    class ObjectContainer {
    public:
        template <typename Obj>
        void Add(Obj obj) {
            AddPtr(std::make_unique<Obj>(std::move(obj)));
        }

        // Добавляет в svg-документ объект-наследник svg::Object
        virtual void AddPtr(std::unique_ptr<Object>&& obj) = 0;

        virtual ~ObjectContainer() = default;
    };

    class Document : public ObjectContainer {
    public:

        // Добавляет в svg-документ объект-наследник svg::Object
        void AddPtr(std::unique_ptr<Object>&& obj) override;

        // Выводит в ostream svg-представление документа
        void Render(std::ostream& out) const;

        // Прочие методы и данные, необходимые для реализации класса Document
    private:
        std::deque<std::unique_ptr<Object>> objects_;
    };

    // Интерфейс Drawable задаёт объекты, которые можно нарисовать с помощью Graphics
    class Drawable {
    public:
        virtual void Draw(ObjectContainer& container) const = 0;

        virtual ~Drawable() = default;
    };

}  // namespace svg