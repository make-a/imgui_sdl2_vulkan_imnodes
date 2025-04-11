#include "NodeEditor.h"

void MyApplication::OnFrame() {
    // 3. (可选) 添加 ImNodes 编辑器示例
    {
        ImGui::Begin( "ImNodes Editor Example" );

        ImNodes::BeginNodeEditor();

        DrawNodes();

        // 添加链接 (如果之前没有)
        if ( ImNodes::IsEditorHovered() && ImGui::IsMouseClicked( ImGuiMouseButton_Left ) ) {
            // 简单的连接逻辑示例，实际应用会更复杂
        }
        // 始终绘制链接 (假设已经有链接数据)
        for ( int i = 0; i < nodeitor.links.size(); ++i ) {
            const Link p = nodeitor.links[ i ];
            ImNodes::Link( p.id, p.start_attr, p.end_attr );
        }

        ImNodes::MiniMap();
        ImNodes::EndNodeEditor();

        // AfterRender
        for ( size_t i = 0; i < nodeitor.nodes.size(); i++ ) {
            auto& node = nodeitor.nodes[ i ];
            node->AfterRender();
        }

        int start_attr, end_attr;
        if ( ImNodes::IsLinkCreated( &start_attr, &end_attr ) ) {
            nodeitor.links.emplace_back( UniqueId::get_id(), start_attr, end_attr );
            // ImNodes::IsPinHovered
        }

        ImGui::End();
    }
}

void MyApplication::DrawNodes() const {
    for ( size_t i = 0; i < nodeitor.nodes.size(); i++ ) {
        auto& node = nodeitor.nodes[ i ];
        node->Render();
    }
}
