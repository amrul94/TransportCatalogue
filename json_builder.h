#pragma once

#include <stack>
#include <memory>
#include "json.h"

namespace json {
    class KeyItemContext;
    class ValueAfterKeyItemContext;
    class DictItemContext;
    class ArrayItemContext;

    class Builder {
    public:
        KeyItemContext Key(std::string key);
        Builder& Value(const Node::Value& value);
        DictItemContext StartDict();
        Builder& EndDict();
        ArrayItemContext StartArray();
        Builder& EndArray();
        Node Build();

        Node& GetRoot();
        std::stack<std::unique_ptr<Node>>& GetNodesStack();
        [[nodiscard]] bool IsBuild() const;
        void SetIsBuild(bool flag);

    private:
        Node root_;
        std::stack<std::unique_ptr<Node>> nodes_stack_;
        bool is_build_ = false;
    };

    class KeyItemContext {
    public:
        explicit KeyItemContext(Builder& builder);
        ValueAfterKeyItemContext Value(const Node::Value& value);
        DictItemContext StartDict();
        ArrayItemContext StartArray();

    private:
        Builder& builder_;
    };

    class ValueAfterKeyItemContext {
    public:
        explicit ValueAfterKeyItemContext(Builder& builder);
        KeyItemContext Key(std::string key);
        Builder& EndDict();

    private:
        Builder& builder_;
    };


    class DictItemContext {
    public:
        explicit DictItemContext(Builder& builder);
        KeyItemContext Key(std::string key);
        Builder& EndDict();

    private:
        Builder& builder_;
    };

    class ArrayItemContext {
    public:
        explicit ArrayItemContext(Builder& builder);
        ArrayItemContext Value(const Node::Value& value);
        DictItemContext StartDict();
        ArrayItemContext StartArray();
        Builder& EndArray();

    private:
        Builder& builder_;
    };
}