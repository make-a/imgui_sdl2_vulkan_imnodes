// main.cpp

#include <iostream>
#include <vector>

#include "app/NodeEditor.h"

// 主函数
int main(int argc, char** argv)
{
    (void)argc;
    (void)argv; // 防止未使用参数警告

    try
    {
        MyApplication app("ImGui Vulkan App", 1280, 720);

        // 可选：在运行前加载字体
        // app.LoadFont("C:/Windows/Fonts/consola.ttf", 16.0f);
        // app.LoadFont("C:/Windows/Fonts/simsun.ttc", 18.0f,
        // ImGui::GetIO().Fonts->GetGlyphRangesChinese()); // 加载宋体支持中文

        app.Run(); // 启动应用程序主循环
    }
    catch (const AppInitError& e)
    {
        std::cerr << "Application Initialization Failed: " << e.what() << std::endl;
        // 可以显示一个消息框或记录日志
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Initialization Error", e.what(), nullptr);
        return EXIT_FAILURE;
    }
    catch (const std::exception& e)
    {
        std::cerr << "An unexpected error occurred: " << e.what() << std::endl;
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Runtime Error", e.what(), nullptr);
        return EXIT_FAILURE;
    }
    catch (...)
    {
        std::cerr << "An unknown error occurred." << std::endl;
        SDL_ShowSimpleMessageBox(
            SDL_MESSAGEBOX_ERROR,
            "Unknown Error",
            "An unknown error occurred during execution.",
            nullptr);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}