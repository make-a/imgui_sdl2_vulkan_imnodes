#pragma once
#include "ImGuiApp.h"
#include "imnodes.h"

struct Node
{
    int   id;
    float value;

    Node(const int i, const float v) : id(i), value(v) {}
};

struct Link
{
    int id;
    int start_attr, end_attr;
};

struct Editor
{
    ImNodesEditorContext* context = nullptr;
    std::vector<Node>     nodes;
    std::vector<Link>     links;
    int                   current_id = 0;

    ~Editor()
    {
        if (context)
            ImNodes::EditorContextFree(context);
    }
};

// 定义一个具体的应用程序类，继承自 ImGuiApp
class MyApplication : public ImGuiApp
{
public:
    // 调用基类构造函数
    MyApplication(const std::string& title, int width, int height) : ImGuiApp(title, width, height)
    {
        nodeitor.context = ImNodes::EditorContextCreate();
        ImNodes::PushAttributeFlag(ImNodesAttributeFlags_EnableLinkDetachWithDragClick);
        ImNodesIO& io = ImNodes::GetIO();
        io.LinkDetachWithModifierClick.Modifier = &ImGui::GetIO().KeyCtrl;
        io.MultipleSelectModifier.Modifier = &ImGui::GetIO().KeyCtrl;

        ImNodesStyle& style = ImNodes::GetStyle();
        style.Flags |= ImNodesStyleFlags_GridLinesPrimary | ImNodesStyleFlags_GridSnapping;
    }
    ~MyApplication() { ImNodes::PopAttributeFlag(); }

protected:
    void OnFrame() override;

private:
    Editor nodeitor;
};