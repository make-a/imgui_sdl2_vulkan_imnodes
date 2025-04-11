
#include "pin.h"

#define Pin_Default IM_COL32( 200, 100, 100, 255 )
#define Pin_Linked IM_COL32( 100, 200, 100, 255 )

void Pin::Render() {
    if ( ptype == PinType::Input ) {
        ImNodes::BeginInputAttribute( pid );
    }
    else {
        ImNodes::BeginOutputAttribute( pid );
    }

    ImGui::Text( pname.c_str() );

    if ( ptype == PinType::Input ) {
        ImNodes::EndInputAttribute();
    }
    else {
        ImNodes::EndOutputAttribute();
    }
}

// void Pin::Render( std::vector<Pin>& pins ) {
//     for ( const auto& pin : pins ) {
//         if ( pin.ptype == PinType::Input ) {
//             ImNodes::BeginInputAttribute( pin.pid );
//         }
//         else {
//             ImNodes::BeginOutputAttribute( pin.pid );
//         }

//         ImGui::Text( pin.pname.c_str() );

//         if ( pin.ptype == PinType::Input ) {
//             ImNodes::EndInputAttribute();
//         }
//         else {
//             ImNodes::EndOutputAttribute();
//         }
//     }
// }
