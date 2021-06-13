#include "json_builder.h"

namespace json {
// -------------- KeyItemContext --------------

    KeyItemContext::KeyItemContext(Builder& builder)
        : builder_(builder){
    }

    ValueAfterKeyItemContext KeyItemContext::Value(const Node::Value& value) {
        NodeGetter getter;
        if (builder_.IsBuild()) {
            throw std::logic_error("Build error");
        }
        auto& nodes_stack_ = builder_.GetNodesStack();
        if(nodes_stack_.empty()) {
            builder_.SetIsBuild(true);
            Node& root = builder_.GetRoot();
            root = std::visit(getter, value);
        } else if (nodes_stack_.top()->IsString()) {
            std::string str = nodes_stack_.top()->AsString();
            nodes_stack_.pop();
            const Dict& dict = nodes_stack_.top()->AsDict();
            const_cast<Dict&>(dict).emplace(str, std::visit(getter, value));
        } else {
            throw std::logic_error("Current type not Node::Value after Key");
        }
        return ValueAfterKeyItemContext(this->builder_);
    }

    DictItemContext KeyItemContext::StartDict() {
        auto& nodes_stack = builder_.GetNodesStack();
        nodes_stack.push(std::make_unique<Node>(Dict{}));
        return DictItemContext{this->builder_};
    }

    ArrayItemContext KeyItemContext::StartArray() {
        auto& nodes_stack = builder_.GetNodesStack();
        nodes_stack.push(std::make_unique<Node>(Array{}));
        return ArrayItemContext{builder_};
    }


// --------- ValueAfterKeyItemContext ---------

    ValueAfterKeyItemContext::ValueAfterKeyItemContext(Builder& builder)
        : builder_(builder) {
    }

    KeyItemContext ValueAfterKeyItemContext::Key(std::string key) {
        auto& nodes_stack = builder_.GetNodesStack();
        if (nodes_stack.empty()) {
            throw std::logic_error("Nodes_stack is empty");
        }
        if (!nodes_stack.top()->IsDict()) {
            throw std::logic_error("Кey adding error");
        }
        nodes_stack.push(std::make_unique<Node>(std::move(key)));
        return KeyItemContext{this->builder_};
    }

    Builder& ValueAfterKeyItemContext::EndDict() {
        auto& nodes_stack = builder_.GetNodesStack();
        if (nodes_stack.empty()) {
            throw std::logic_error("EndDict must end the dict");
        }
        if(!nodes_stack.top()->IsDict()) {
            throw std::logic_error("EndDict must end the dict");
        }
        Node node = *nodes_stack.top();
        nodes_stack.pop();
        return builder_.Value(node.AsDict());
    }

// ------------- DictItemContext --------------

    DictItemContext::DictItemContext(Builder& builder)
            : builder_(builder) {
    }

    KeyItemContext DictItemContext::Key(std::string key) {
        auto& nodes_stack = builder_.GetNodesStack();
        if (nodes_stack.empty()) {
            throw std::logic_error("Nodes_stack is empty");
        }
        if (!nodes_stack.top()->IsDict()) {
            throw std::logic_error("Кey adding error");
        }
        nodes_stack.push(std::make_unique<Node>(std::move(key)));
        return KeyItemContext{builder_};
    }

    Builder& DictItemContext::EndDict() {
        auto& nodes_stack = builder_.GetNodesStack();
        if (nodes_stack.empty()) {
            throw std::logic_error("EndDict must end the dict");
        }
        if(!nodes_stack.top()->IsDict()) {
            throw std::logic_error("EndDict must end the dict");
        }
        Node node = *nodes_stack.top();
        nodes_stack.pop();
        return builder_.Value(node.AsDict());
    }

// ------------- ArrayItemContext -------------

    ArrayItemContext::ArrayItemContext(Builder& builder)
        : builder_(builder){
    }

    ArrayItemContext ArrayItemContext::Value(const Node::Value& value) {
        NodeGetter getter;
        if (builder_.IsBuild()) {
            throw std::logic_error("Build error");
        }

        auto& nodes_stack = builder_.GetNodesStack();
        if(nodes_stack.empty()) {
            builder_.SetIsBuild(true);
            auto& root = builder_.GetRoot();
            root = std::visit(getter, value);
        } else if (nodes_stack.top()->IsArray()) {
            const Array& arr = nodes_stack.top()->AsArray();
            const_cast<Array&>(arr).push_back(std::visit(getter, value));
        } else {
            throw std::logic_error("Current type not Node::Value");
        }
        return *this;
    }

    DictItemContext ArrayItemContext::StartDict() {
        auto& nodes_stack = builder_.GetNodesStack();
        nodes_stack.push(std::make_unique<Node>(Dict{}));
        return DictItemContext(builder_);
    }

    ArrayItemContext ArrayItemContext::StartArray() {
        auto& nodes_stack = builder_.GetNodesStack();
        nodes_stack.push(std::make_unique<Node>(Array{}));
        return *this;
    }

    Builder& ArrayItemContext::EndArray() {
        auto& nodes_stack = builder_.GetNodesStack();
        if (nodes_stack.empty()) {
            throw std::logic_error("EndArray must end the array");
        }
        if (!nodes_stack.top()->IsArray()) {
            throw std::logic_error("EndArray must end the array");
        }
        Node node = *nodes_stack.top();
        nodes_stack.pop();
        return builder_.Value(node.AsArray());
    }

// ----------------- Builder ------------------

    KeyItemContext Builder::Key(std::string key) {
        if (nodes_stack_.empty()) {
            throw std::logic_error("Nodes_stack is empty");
        }
        if (!nodes_stack_.top()->IsDict()) {
            throw std::logic_error("Кey adding error");
        }
        nodes_stack_.push(std::make_unique<Node>(std::move(key)));
        return KeyItemContext{*this};
    }

    Builder& Builder::Value(const Node::Value& value) {
        NodeGetter getter;
        if (is_build_) {
            throw std::logic_error("Build error");
        }
        if(nodes_stack_.empty()) {
            is_build_ = true;
            root_ = std::visit(getter, value);
        } else if (nodes_stack_.top()->IsArray()) {
            const Array& arr = nodes_stack_.top()->AsArray();
            const_cast<Array&>(arr).push_back(std::visit(getter, value));
        } else if (nodes_stack_.top()->IsString()) {
            std::string str = nodes_stack_.top()->AsString();
            nodes_stack_.pop();
            const Dict& dict = nodes_stack_.top()->AsDict();
            const_cast<Dict&>(dict).emplace(str, std::visit(getter, value));
        } else {
            throw std::logic_error("Current type not Node::Value");
        }
        return *this;
    }

    DictItemContext Builder::StartDict() {
        //Node* dict = new Node{Dict{}};
        nodes_stack_.push(std::make_unique<Node>(Dict{}));
        return DictItemContext(*this);
    }

    Builder& Builder::EndDict() {
        if (nodes_stack_.empty()) {
            throw std::logic_error("EndDict must end the dict");
        }
        if(!nodes_stack_.top()->IsDict()) {
            throw std::logic_error("EndDict must end the dict");
        }
        Node node = *nodes_stack_.top();
        nodes_stack_.pop();
        return Value(node.AsDict());
    }

    ArrayItemContext Builder::StartArray(){
        nodes_stack_.push(std::make_unique<Node>(Array{}));
        return ArrayItemContext{*this};
    }

    Builder& Builder::EndArray() {
        if (nodes_stack_.empty()) {
            throw std::logic_error("EndArray must end the array");
        }
        if (!nodes_stack_.top()->IsArray()) {
            throw std::logic_error("EndArray must end the array");
        }
        Node node = *nodes_stack_.top();
        nodes_stack_.pop();
        return Value(node.AsArray());
    }

    Node Builder::Build() {
        if (!is_build_) {
            throw std::logic_error("Build error");
        }
        if (!nodes_stack_.empty()) {
            throw std::logic_error("Build error");
        }
        return root_;
    }

    Node& Builder::GetRoot() {
        return root_;
    }

    std::stack<std::unique_ptr<Node>>& Builder::GetNodesStack() {
        return nodes_stack_;
    }

    bool Builder::IsBuild() const{
        return is_build_;
    }

    void Builder::SetIsBuild(bool flag) {
        is_build_ = flag;
    }
}
