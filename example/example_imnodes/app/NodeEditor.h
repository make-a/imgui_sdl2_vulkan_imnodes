#pragma once
#include <memory>

#include "ImGuiApp.h"
// #include "imnodes.h"

#include "node.h"

struct Link {
    int id;
    int start_attr, end_attr;

    Link() = default;
    Link( const int i, const int s, const int e )
        : id( i )
        , start_attr( s )
        , end_attr( e ) {}
};

struct Editor {
    ImNodesEditorContext* context = nullptr;
    std::vector<std::shared_ptr<NodeBase>> nodes;
    std::vector<Link> links;
    int current_id = 0;

    ~Editor() {
        if ( context )
            ImNodes::EditorContextFree( context );
    }
};

// 定义一个具体的应用程序类，继承自 ImGuiApp
class MyApplication : public ImGuiApp {
public:
    // 调用基类构造函数
    MyApplication( const std::string& title, int width, int height )
        : ImGuiApp( title, width, height ) {
        nodeitor.context = ImNodes::EditorContextCreate();
        ImNodes::PushAttributeFlag( ImNodesAttributeFlags_EnableLinkDetachWithDragClick );
        ImNodesIO& io = ImNodes::GetIO();
        io.LinkDetachWithModifierClick.Modifier = &ImGui::GetIO().KeyCtrl;
        io.MultipleSelectModifier.Modifier = &ImGui::GetIO().KeyCtrl;

        // ImNodesStyle &style = ImNodes::GetStyle();
        // style.Flags |= ImNodesStyleFlags_GridLinesPrimary | ImNodesStyleFlags_GridSnapping;

        nodeitor.nodes.emplace_back( std::make_shared<AddNode>() );
        nodeitor.nodes.emplace_back( std::make_shared<SubNode>() );

        ImNodesStyle& style = ImNodes::GetStyle();
        style.PinCircleRadius = 6.0f;
        style.NodePadding = ImVec2(22.0f, 8.0f);
    }
    ~MyApplication() {
        ImNodes::PopAttributeFlag();
    }

protected:
    void OnFrame() override;

    void DrawNodes() const;

private:
    Editor nodeitor;
};