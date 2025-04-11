#include "node.h"

#define Pin_Default IM_COL32( 200, 100, 100, 255 )
#define Pin_Linked IM_COL32( 100, 200, 100, 255 )

int UniqueId::id = 100;
std::vector<int> UniqueId::ids = {};

void NodeBase::Render() {
    const float node_width = 100.f;
    ImNodes::BeginNode( node_id );
    int column = 2;
    if ( title_pos == TitlePos::Top ) {
        ImNodes::BeginNodeTitleBar();
        ImGui::TextUnformatted( name.c_str() );
        ImNodes::EndNodeTitleBar();
    }
    else if ( title_pos == TitlePos::Center ) {
        column = 3;
    }

    ImNodes::PushStyleVar( ImNodesStyleVar_PinLineThickness, 2 );
    ImNodes::PushStyleVar( ImNodesStyleVar_PinCircleRadius, 16 );

    if ( ImGui::BeginTable( ( "##table" + std::to_string( node_id ) ).c_str(), column, ImGuiTableFlags_RowBg,
                            ImVec2( node_width, 0 ) ) ) {
        int col = 0;
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex( col++ );
        for ( auto& pin : pins ) {
            if ( pin.ptype == PinType::Input )
                pin.Render();
        }

        if ( title_pos == TitlePos::Center ) {
            ImGui::TableSetColumnIndex( col++ );
            ImGui::TextUnformatted( name.c_str() );
        }

        ImGui::TableSetColumnIndex( col++ );
        for ( auto& pin : pins ) {
            if ( pin.ptype == PinType::Output )
                pin.Render();
        }

        ImGui::EndTable();
    }

    ImNodes::PopStyleVar();
    ImNodes::PopStyleVar();

    ImNodes::EndNode();
}

void NodeBase::AfterRender() {}
