#include "NodeEditor.h"

void MyApplication::OnFrame()
{
    {
        ImGui::Begin("Hello from MyApplication!"); // 开始一个新窗口

        ImGui::Text("This is your custom application window.");
        ImGui::Checkbox("Show ImGui Demo Window", &show_demo_window);
        ImGui::Checkbox("Show ImNodes Demo Window", &show_imnodes_demo_window);
        ImGui::SliderFloat("Float Slider", &my_float_value, 0.0f, 1.0f);
        ImGui::ColorEdit3("Clear Color", (float*)&m_clearColor); // 可以直接修改基类的清屏色

        if (ImGui::Button("Click Me"))
        {
            my_counter++;
        }
        ImGui::SameLine();
        ImGui::Text("Counter = %d", my_counter);

        ImGui::Text(
            "Application average %.3f ms/frame (%.1f FPS)",
            1000.0f / ImGui::GetIO().Framerate,
            ImGui::GetIO().Framerate);

        ImGui::End(); // 结束窗口
    }

    // 3. (可选) 添加 ImNodes 编辑器示例
    {
        ImGui::Begin("ImNodes Editor Example");

        ImNodes::BeginNodeEditor();

        // 添加一些节点和链接 (简单示例)
        ImNodes::BeginNode(1);
        ImNodes::BeginNodeTitleBar();
        ImGui::TextUnformatted("Input Node");
        ImNodes::EndNodeTitleBar();
        ImNodes::BeginOutputAttribute(101);
        ImGui::Text("Output");
        ImNodes::EndOutputAttribute();
        ImNodes::EndNode();

        ImNodes::BeginNode(2);
        ImNodes::BeginNodeTitleBar();
        ImGui::TextUnformatted("Output Node");
        ImNodes::EndNodeTitleBar();
        ImNodes::BeginInputAttribute(201);
        ImGui::Text("Input");
        ImNodes::EndInputAttribute();
        ImNodes::EndNode();

        // 添加链接 (如果之前没有)
        if (ImNodes::IsEditorHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        {
            // 简单的连接逻辑示例，实际应用会更复杂
        }
        // 始终绘制链接 (假设已经有链接数据)
        for (int i = 0; i < links.size(); ++i)
        {
            const std::pair<int, int> p = links[i];
            ImNodes::Link(i, p.first, p.second);
        }

        ImNodes::MiniMap();
        ImNodes::EndNodeEditor();

        int start_attr, end_attr;
        if (ImNodes::IsLinkCreated(&start_attr, &end_attr))
        {
            links.push_back(std::make_pair(start_attr, end_attr));
        }

        ImGui::End();
    }

    // --- ImGui 界面代码结束 ---
}
