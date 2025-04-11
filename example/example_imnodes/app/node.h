#pragma once
#include <any>
#include <string>
#include <vector>

#include "imnodes.h"

#include "pin.h"

class UniqueId {
public:
    UniqueId() = default;
    static int get_id() {
        ids.emplace_back( id++ );
        return ids.back();
    }

private:
    static int id;
    static std::vector<int> ids;
};

class NodeBase {
public:
    enum class TitlePos { Top = 0, Center, None };

    NodeBase()
        : node_id( UniqueId::get_id() ) {}

    void Render();

    void AfterRender();

    std::string name;
    std::vector<Pin> pins;
    TitlePos title_pos = TitlePos::Top;

    int node_id = 0;
};

class AddNode : public NodeBase {
public:
    AddNode()
        : NodeBase() {
        name = "Add";
        pins = std::vector<Pin>{ Pin{ UniqueId::get_id(), "A", PinType::Input }, Pin{ UniqueId::get_id(), "B", PinType::Input },
                                 Pin{ UniqueId::get_id(), "C", PinType::Output } };
    }
};

class SubNode : public NodeBase {
public:
    SubNode()
        : NodeBase() {
        name = "Sub";
        pins =
            std::vector<Pin>{ Pin{ UniqueId::get_id(), "A", PinType::Input }, Pin{ UniqueId::get_id(), "B", PinType::Input },
                              Pin{ UniqueId::get_id(), "C", PinType::Output }, Pin{ UniqueId::get_id(), "D", PinType::Output } };
    }
};