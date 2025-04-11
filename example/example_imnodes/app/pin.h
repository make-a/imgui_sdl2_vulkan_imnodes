
#pragma once

#include <any>
#include <string>
#include <vector>

#include "imnodes.h"

enum class PinType { Input, Output };
struct Pin {
    PinType ptype;
    int pid;
    std::string pname;

    bool isLinked = false;

    Pin() = default;
    Pin( const int id, const std::string& name, const PinType& type )
        : pid( id )
        , pname( name )
        , ptype( type ) {}

    void Render();
};