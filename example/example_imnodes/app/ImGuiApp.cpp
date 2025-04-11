// ImGuiApp.cpp
#include "ImGuiApp.h"

#include <cstdio>
#include <cstdlib>
#include <stdexcept>

#include "imnodes.h"

// Volk (如果使用)
#ifdef IMGUI_IMPL_VULKAN_USE_VOLK
// Volk 实现已在 .h 中定义，这里不需要再次定义
#else
// 如果不用 Volk, 确保 Vulkan 头文件和加载器已正确链接
#endif

// 定义调试宏 (可以移到 CMake 或项目设置中)
// #define APP_USE_UNLIMITED_FRAME_RATE
#ifdef _DEBUG
#define APP_USE_VULKAN_DEBUG_REPORT
#endif

//-----------------------------------------------------------------------------
// ImGuiApp Implementation
//-----------------------------------------------------------------------------

ImGuiApp::ImGuiApp( const std::string& windowTitle, int width, int height ) {
    try {
        InitSDL( windowTitle, width, height );
        InitVulkan();
        SetupVulkanWindow( width, height );
        InitImGui();
        // 注意：字体上传推迟到第一次渲染前或显式调用 LoadFont 后
    }
    catch ( const std::exception& e ) {
        fprintf( stderr, "Error during ImGuiApp initialization: %s\n", e.what() );
        // 清理已初始化的部分
        CleanupImGui();  // 即使未完全初始化也尝试清理
        CleanupVulkanWindow();
        CleanupVulkan();
        CleanupSDL();
        throw;  // 重新抛出异常，让调用者知道失败了
    }
}

ImGuiApp::~ImGuiApp() {
    // 等待设备空闲，确保所有渲染操作完成
    if ( m_device != VK_NULL_HANDLE ) {
        VkResult err = vkDeviceWaitIdle( m_device );
        check_vk_result( err );
    }

    // 按初始化逆序清理
    CleanupImGui();
    CleanupVulkanWindow();
    CleanupVulkan();
    CleanupSDL();
}

void ImGuiApp::Run() {
    m_running = true;
    while ( m_running ) {
        SDL_Event event;
        while ( SDL_PollEvent( &event ) ) {
            ImGui_ImplSDL2_ProcessEvent( &event );  // 将事件传递给 ImGui
            if ( event.type == SDL_QUIT ) {
                m_running = false;
            }
            if ( event.type == SDL_WINDOWEVENT ) {
                if ( event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID( m_window ) ) {
                    m_running = false;
                }
                if ( event.window.event == SDL_WINDOWEVENT_MINIMIZED ) {
                    m_minimized = true;
                }
                if ( event.window.event == SDL_WINDOWEVENT_RESTORED || event.window.event == SDL_WINDOWEVENT_SHOWN
                     || event.window.event == SDL_WINDOWEVENT_MAXIMIZED ) {
                    m_minimized = false;
                }
                if ( event.window.event == SDL_WINDOWEVENT_RESIZED || event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED ) {
                    // 标记需要重建交换链，但延迟到渲染前处理
                    m_swapChainRebuild = true;
                }
            }
        }

        // 如果窗口最小化，则跳过渲染
        if ( m_minimized ) {
            SDL_Delay( 10 );  // 避免 CPU 忙等
            continue;
        }

        // 处理窗口大小变化 / 重建交换链
        int currentWidth, currentHeight;
        SDL_GetWindowSize( m_window, &currentWidth, &currentHeight );
        if ( currentWidth > 0 && currentHeight > 0
             && ( m_swapChainRebuild || m_mainWindowData.Width != currentWidth || m_mainWindowData.Height != currentHeight ) ) {
            HandleResize( currentWidth, currentHeight );
        }

        // 开始渲染新的一帧
        BeginFrame();

        // --- 用户绘制代码 ---
        OnFrame();  // 调用派生类实现的绘制逻辑
        // --------------------

        // 结束并渲染 ImGui 数据
        RenderFrame();

        // 呈现到屏幕
        PresentFrame();
    }
}

ImFont* ImGuiApp::LoadFont( const std::string& path, float size, const ImWchar* glyphRanges, bool merge ) {
    ImGuiIO& io = ImGui::GetIO();
    ImFontConfig font_cfg;
    font_cfg.MergeMode = merge;  // 设置是否合并

    ImFont* font = io.Fonts->AddFontFromFileTTF( path.c_str(), size, &font_cfg, glyphRanges );
    if ( !font ) {
        fprintf( stderr, "Error loading font: %s\n", path.c_str() );
        return nullptr;
    }
    // 标记需要重新上传字体纹理
    m_fontsLoaded = false;  // 强制在下一帧开始前重新上传
    return font;
}

void ImGuiApp::UploadFonts() {
    // 如果字体已上传，则跳过
    if ( m_fontsLoaded )
        return;

    VkResult err;
    // 使用一次性命令缓冲上传字体
    VkCommandPool command_pool = m_mainWindowData.Frames[ m_mainWindowData.FrameIndex ].CommandPool;
    VkCommandBuffer command_buffer = m_mainWindowData.Frames[ m_mainWindowData.FrameIndex ].CommandBuffer;

    err = vkResetCommandPool( m_device, command_pool, 0 );
    check_vk_result( err );
    VkCommandBufferBeginInfo begin_info = {};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    err = vkBeginCommandBuffer( command_buffer, &begin_info );
    check_vk_result( err );

    ImGui_ImplVulkan_CreateFontsTexture();
    // ImGui_ImplVulkan_CreateFontsTexture(command_buffer);

    VkSubmitInfo end_info = {};
    end_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    end_info.commandBufferCount = 1;
    end_info.pCommandBuffers = &command_buffer;
    err = vkEndCommandBuffer( command_buffer );
    check_vk_result( err );
    err = vkQueueSubmit( m_queue, 1, &end_info, VK_NULL_HANDLE );
    check_vk_result( err );

    err = vkDeviceWaitIdle( m_device );  // 等待上传完成
    check_vk_result( err );
    ImGui_ImplVulkan_DestroyFontsTexture();  // 清理临时资源

    m_fontsLoaded = true;
    printf( "Fonts uploaded to GPU.\n" );
}

// --- Private Initialization Methods ---

void ImGuiApp::InitSDL( const std::string& windowTitle, int width, int height ) {
    if ( SDL_Init( SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER ) != 0 ) {
        throw AppInitError( "SDL_Init Error: " + std::string( SDL_GetError() ) );
    }

    // From 2.0.18: Enable native IME.
#ifdef SDL_HINT_IME_SHOW_UI
    SDL_SetHint( SDL_HINT_IME_SHOW_UI, "1" );
#endif

    SDL_WindowFlags window_flags = ( SDL_WindowFlags )( SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI );
    m_window =
        SDL_CreateWindow( windowTitle.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, window_flags );
    if ( m_window == nullptr ) {
        SDL_Quit();  // 清理已初始化的 SDL
        throw AppInitError( "SDL_CreateWindow Error: " + std::string( SDL_GetError() ) );
    }
}

void ImGuiApp::InitVulkan() {
    VkResult err;

#ifdef IMGUI_IMPL_VULKAN_USE_VOLK
    volkInitialize();
    if ( volkGetInstanceVersion() == 0 ) {
        throw AppInitError( "Vulkan loader not found." );
    }
#endif

    // 获取 SDL 需要的 Vulkan 实例扩展
    ImVector<const char*> instance_extensions;
    uint32_t extensions_count = 0;
    SDL_Vulkan_GetInstanceExtensions( m_window, &extensions_count, nullptr );
    instance_extensions.resize( extensions_count );
    SDL_Vulkan_GetInstanceExtensions( m_window, &extensions_count, instance_extensions.Data );

    // --- 创建 Vulkan Instance (与原 SetupVulkan 类似) ---
    {
        VkInstanceCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;

        // 查询可用扩展
        uint32_t properties_count;
        ImVector<VkExtensionProperties> properties;
        vkEnumerateInstanceExtensionProperties( nullptr, &properties_count, nullptr );
        properties.resize( properties_count );
        err = vkEnumerateInstanceExtensionProperties( nullptr, &properties_count, properties.Data );
        check_vk_result( err );  // 使用静态方法检查

        // 添加必要的扩展
        if ( IsExtensionAvailable( properties, VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME ) )
            instance_extensions.push_back( VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME );
#ifdef VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME
        if ( IsExtensionAvailable( properties, VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME ) ) {
            instance_extensions.push_back( VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME );
            create_info.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
        }
#endif

        // 启用验证层和调试报告 (Debug build)
#ifdef APP_USE_VULKAN_DEBUG_REPORT
        const char* layers[] = { "VK_LAYER_KHRONOS_validation" };
        create_info.enabledLayerCount = 1;
        create_info.ppEnabledLayerNames = layers;
        instance_extensions.push_back( VK_EXT_DEBUG_REPORT_EXTENSION_NAME );  // 注意宏名称
#endif

        create_info.enabledExtensionCount = ( uint32_t )instance_extensions.Size;
        create_info.ppEnabledExtensionNames = instance_extensions.Data;

        // 可选：设置应用程序信息
        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "ImGui App";
        appInfo.applicationVersion = VK_MAKE_VERSION( 1, 0, 0 );
        appInfo.pEngineName = "No Engine";
        appInfo.engineVersion = VK_MAKE_VERSION( 1, 0, 0 );
        // 根据你的 Vulkan 头文件版本设置 apiVersion，或者让驱动选择最新的
        appInfo.apiVersion = VK_API_VERSION_1_1;  // 例如，或者 VK_API_VERSION_1_0, 1_2, 1_3
        create_info.pApplicationInfo = &appInfo;

        err = vkCreateInstance( &create_info, m_allocator, &m_instance );
        if ( err != VK_SUCCESS ) {
            throw AppInitError( "vkCreateInstance failed with error code: " + std::to_string( err ) );
        }
#ifdef IMGUI_IMPL_VULKAN_USE_VOLK
        volkLoadInstance( m_instance );
#endif

        // 设置调试回调
#ifdef APP_USE_VULKAN_DEBUG_REPORT
        auto f_vkCreateDebugReportCallbackEXT =
            ( PFN_vkCreateDebugReportCallbackEXT )vkGetInstanceProcAddr( m_instance, "vkCreateDebugReportCallbackEXT" );
        if ( !f_vkCreateDebugReportCallbackEXT ) {
            fprintf( stderr, "Warning: vkCreateDebugReportCallbackEXT not found, debug reporting disabled.\n" );
        }
        else {
            VkDebugReportCallbackCreateInfoEXT debug_report_ci = {};
            debug_report_ci.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
            debug_report_ci.flags =
                VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
            debug_report_ci.pfnCallback = debug_report;  // 使用静态回调函数
            debug_report_ci.pUserData = nullptr;
            err = f_vkCreateDebugReportCallbackEXT( m_instance, &debug_report_ci, m_allocator, &m_debugReport );
            check_vk_result( err );
        }
#endif
    }

    // --- 选择物理设备 (GPU) ---
    m_physicalDevice = ImGui_ImplVulkanH_SelectPhysicalDevice( m_instance );  // 使用 ImGui 提供的帮助函数
    if ( m_physicalDevice == VK_NULL_HANDLE ) {
        throw AppInitError( "Failed to find suitable Vulkan physical device." );
    }

    // --- 选择图形队列族 ---
    m_queueFamily = ImGui_ImplVulkanH_SelectQueueFamilyIndex( m_physicalDevice );
    if ( m_queueFamily == ( uint32_t )-1 ) {
        throw AppInitError( "Failed to find suitable Vulkan queue family." );
    }

    // --- 创建逻辑设备 ---
    {
        ImVector<const char*> device_extensions;
        device_extensions.push_back( "VK_KHR_swapchain" );

        // 检查并添加可移植性子集扩展 (主要用于 macOS/MoltenVK)
        uint32_t properties_count;
        ImVector<VkExtensionProperties> properties;
        vkEnumerateDeviceExtensionProperties( m_physicalDevice, nullptr, &properties_count, nullptr );
        properties.resize( properties_count );
        vkEnumerateDeviceExtensionProperties( m_physicalDevice, nullptr, &properties_count, properties.Data );
#ifdef VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME
        if ( IsExtensionAvailable( properties, VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME ) )
            device_extensions.push_back( VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME );
#endif

        const float queue_priority[] = { 1.0f };
        VkDeviceQueueCreateInfo queue_info[ 1 ] = {};
        queue_info[ 0 ].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_info[ 0 ].queueFamilyIndex = m_queueFamily;
        queue_info[ 0 ].queueCount = 1;
        queue_info[ 0 ].pQueuePriorities = queue_priority;

        VkDeviceCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        create_info.queueCreateInfoCount = sizeof( queue_info ) / sizeof( queue_info[ 0 ] );
        create_info.pQueueCreateInfos = queue_info;
        create_info.enabledExtensionCount = ( uint32_t )device_extensions.Size;
        create_info.ppEnabledExtensionNames = device_extensions.Data;
        // 可选：启用设备特性 (如果需要的话)
        // VkPhysicalDeviceFeatures deviceFeatures = {};
        // create_info.pEnabledFeatures = &deviceFeatures;

        err = vkCreateDevice( m_physicalDevice, &create_info, m_allocator, &m_device );
        if ( err != VK_SUCCESS ) {
            throw AppInitError( "vkCreateDevice failed with error code: " + std::to_string( err ) );
        }
        vkGetDeviceQueue( m_device, m_queueFamily, 0, &m_queue );
    }

    // --- 创建描述符池 ---
    {
        // 增加池大小以支持更多纹理等 (如果需要)
        VkDescriptorPoolSize pool_sizes[] = {
            { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },  // 增加数量
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
            // 添加其他你可能需要的类型
        };
        VkDescriptorPoolCreateInfo pool_info = {};
        pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        pool_info.maxSets = 1000 * IM_ARRAYSIZE( pool_sizes );  // 总集合数
        pool_info.poolSizeCount = ( uint32_t )IM_ARRAYSIZE( pool_sizes );
        pool_info.pPoolSizes = pool_sizes;
        err = vkCreateDescriptorPool( m_device, &pool_info, m_allocator, &m_descriptorPool );
        if ( err != VK_SUCCESS ) {
            throw AppInitError( "vkCreateDescriptorPool failed with error code: " + std::to_string( err ) );
        }
    }
}

void ImGuiApp::SetupVulkanWindow( int width, int height ) {
    // VkResult     err;
    VkSurfaceKHR surface;

    // 创建 Vulkan Surface
    if ( SDL_Vulkan_CreateSurface( m_window, m_instance, &surface ) == SDL_FALSE ) {
        throw AppInitError( "Failed to create Vulkan surface: " + std::string( SDL_GetError() ) );
    }

    // --- 设置窗口数据 (与原 SetupVulkanWindow 类似) ---
    ImGui_ImplVulkanH_Window* wd = &m_mainWindowData;  // 使用成员变量
    wd->Surface = surface;

    // 检查 WSI 支持
    VkBool32 res;
    vkGetPhysicalDeviceSurfaceSupportKHR( m_physicalDevice, m_queueFamily, wd->Surface, &res );
    if ( res != VK_TRUE ) {
        vkDestroySurfaceKHR( m_instance, wd->Surface, m_allocator );  // 清理已创建的 surface
        throw AppInitError( "Error: No WSI support on selected physical device queue family." );
    }

    // 选择 Surface Format
    const VkFormat requestSurfaceImageFormat[] = { VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_B8G8R8_UNORM,
                                                   VK_FORMAT_R8G8B8_UNORM };
    const VkColorSpaceKHR requestSurfaceColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
    wd->SurfaceFormat =
        ImGui_ImplVulkanH_SelectSurfaceFormat( m_physicalDevice, wd->Surface, requestSurfaceImageFormat,
                                               ( size_t )IM_ARRAYSIZE( requestSurfaceImageFormat ), requestSurfaceColorSpace );

    // 选择 Present Mode
#ifdef APP_USE_UNLIMITED_FRAME_RATE
    VkPresentModeKHR present_modes[] = { VK_PRESENT_MODE_MAILBOX_KHR, VK_PRESENT_MODE_IMMEDIATE_KHR, VK_PRESENT_MODE_FIFO_KHR };
#else
    VkPresentModeKHR present_modes[] = { VK_PRESENT_MODE_FIFO_KHR };  // VSync
#endif
    wd->PresentMode =
        ImGui_ImplVulkanH_SelectPresentMode( m_physicalDevice, wd->Surface, &present_modes[ 0 ], IM_ARRAYSIZE( present_modes ) );

    // 创建 SwapChain, RenderPass, Framebuffer 等
    IM_ASSERT( m_minImageCount >= 2 );
    ImGui_ImplVulkanH_CreateOrResizeWindow( m_instance, m_physicalDevice, m_device, wd, m_queueFamily, m_allocator, width, height,
                                            m_minImageCount );
    if ( wd->Swapchain == VK_NULL_HANDLE ) {  // 检查创建是否成功
        vkDestroySurfaceKHR( m_instance, wd->Surface, m_allocator );
        throw AppInitError( "Failed to create Vulkan swapchain window resources." );
    }
}

void ImGuiApp::InitImGui() {
    // 设置 Dear ImGui 上下文
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImNodes::CreateContext();  // 初始化 ImNodes

    ImGuiIO& io = ImGui::GetIO();
    ( void )io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // 启用键盘控制
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;   // 启用手柄控制
    // io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;    // 可选：启用 Docking
    // io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable; // 可选：启用多视口

    // 设置 Dear ImGui 风格
    ImGui::StyleColorsDark();
    // ImGui::StyleColorsLight();

    // 当启用视口时，调整窗口圆角和背景以匹配平台窗口
    // if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
    //     ImGuiStyle& style = ImGui::GetStyle();
    //     style.WindowRounding = 0.0f;
    //     style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    // }

    // 设置 Platform/Renderer 后端
    ImGui_ImplSDL2_InitForVulkan( m_window );
    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance = m_instance;
    init_info.PhysicalDevice = m_physicalDevice;
    init_info.Device = m_device;
    init_info.QueueFamily = m_queueFamily;
    init_info.Queue = m_queue;
    init_info.PipelineCache = m_pipelineCache;  // 可以是 VK_NULL_HANDLE
    init_info.DescriptorPool = m_descriptorPool;
    init_info.RenderPass = m_mainWindowData.RenderPass;
    init_info.Subpass = 0;
    init_info.MinImageCount = m_minImageCount;
    init_info.ImageCount = m_mainWindowData.ImageCount;
    init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;  // 或其他 MSAA 设置
    init_info.Allocator = m_allocator;
    init_info.CheckVkResultFn = check_vk_result;  // 使用静态检查函数
    // init_info.UseDynamicRendering = ...; // 如果使用 Vulkan 1.3 动态渲染
    // init_info.PipelineRenderingCreateInfo = ...; // 如果使用动态渲染

    if ( !ImGui_ImplVulkan_Init( &init_info ) ) {
        ImGui_ImplSDL2_Shutdown();
        ImNodes::DestroyContext();
        ImGui::DestroyContext();
        throw AppInitError( "ImGui_ImplVulkan_Init failed." );
    }

    // 加载默认字体 (如果用户不调用 LoadFont，则使用这个)
    // io.Fonts->AddFontDefault();
    // UploadFonts(); // 可以在这里上传默认字体，或者等待第一次渲染前
}

// --- Private Cleanup Methods ---

void ImGuiApp::CleanupImGui() {
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    if ( ImNodes::GetCurrentContext() ) {  // 检查 ImNodes 上下文是否存在
        ImNodes::DestroyContext();
    }
    if ( ImGui::GetCurrentContext() ) {  // 检查 ImGui 上下文是否存在
        ImGui::DestroyContext();
    }
}

void ImGuiApp::CleanupVulkanWindow() {
    ImGui_ImplVulkanH_DestroyWindow( m_instance, m_device, &m_mainWindowData, m_allocator );
    // Surface 在 DestroyWindow 内部被销毁
    m_mainWindowData.Surface = VK_NULL_HANDLE;  // 明确置空
}

void ImGuiApp::CleanupVulkan() {
    if ( m_descriptorPool != VK_NULL_HANDLE ) {
        vkDestroyDescriptorPool( m_device, m_descriptorPool, m_allocator );
        m_descriptorPool = VK_NULL_HANDLE;
    }
    if ( m_pipelineCache != VK_NULL_HANDLE ) {
        vkDestroyPipelineCache( m_device, m_pipelineCache, m_allocator );
        m_pipelineCache = VK_NULL_HANDLE;
    }

#ifdef APP_USE_VULKAN_DEBUG_REPORT
    if ( m_debugReport != VK_NULL_HANDLE ) {
        auto f_vkDestroyDebugReportCallbackEXT =
            ( PFN_vkDestroyDebugReportCallbackEXT )vkGetInstanceProcAddr( m_instance, "vkDestroyDebugReportCallbackEXT" );
        if ( f_vkDestroyDebugReportCallbackEXT ) {
            f_vkDestroyDebugReportCallbackEXT( m_instance, m_debugReport, m_allocator );
        }
        m_debugReport = VK_NULL_HANDLE;
    }
#endif

    if ( m_device != VK_NULL_HANDLE ) {
        vkDestroyDevice( m_device, m_allocator );
        m_device = VK_NULL_HANDLE;
    }
    if ( m_instance != VK_NULL_HANDLE ) {
        vkDestroyInstance( m_instance, m_allocator );
        m_instance = VK_NULL_HANDLE;
    }
}

void ImGuiApp::CleanupSDL() {
    if ( m_window != nullptr ) {
        SDL_DestroyWindow( m_window );
        m_window = nullptr;
    }
    SDL_Quit();
}

// --- Private Frame Logic Methods ---

void ImGuiApp::BeginFrame() {
    // 如果字体尚未上传 (例如，调用了 LoadFont 后)，立即上传
    if ( !m_fontsLoaded ) {
        UploadFonts();
    }

    // 开始 Dear ImGui 帧
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();
}

void ImGuiApp::RenderFrame() {
    ImGui::Render();
    ImDrawData* draw_data = ImGui::GetDrawData();
    const bool is_minimized = ( draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f );

    if ( is_minimized ) {
        return;  // 如果窗口最小化或无内容，则不执行渲染
    }

    ImGui_ImplVulkanH_Window* wd = &m_mainWindowData;

    // 设置清屏颜色 (可以由派生类修改 m_clearColor)
    wd->ClearValue.color.float32[ 0 ] = m_clearColor.x * m_clearColor.w;
    wd->ClearValue.color.float32[ 1 ] = m_clearColor.y * m_clearColor.w;
    wd->ClearValue.color.float32[ 2 ] = m_clearColor.z * m_clearColor.w;
    wd->ClearValue.color.float32[ 3 ] = m_clearColor.w;

    // --- 执行渲染命令 (与原 FrameRender 类似) ---
    VkSemaphore image_acquired_semaphore = wd->FrameSemaphores[ wd->SemaphoreIndex ].ImageAcquiredSemaphore;
    VkSemaphore render_complete_semaphore = wd->FrameSemaphores[ wd->SemaphoreIndex ].RenderCompleteSemaphore;
    VkResult err =
        vkAcquireNextImageKHR( m_device, wd->Swapchain, UINT64_MAX, image_acquired_semaphore, VK_NULL_HANDLE, &wd->FrameIndex );

    if ( err == VK_ERROR_OUT_OF_DATE_KHR || err == VK_SUBOPTIMAL_KHR ) {
        m_swapChainRebuild = true;  // 标记需要重建
        if ( err == VK_ERROR_OUT_OF_DATE_KHR )
            return;  // 如果完全过期，则跳过此帧的剩余渲染
    }
    else if ( err != VK_SUCCESS ) {
        check_vk_result( err );  // 处理其他获取图像错误
        return;                  // 出错则跳过渲染
    }

    ImGui_ImplVulkanH_Frame* fd = &wd->Frames[ wd->FrameIndex ];
    {
        // 等待上一帧使用此 Framebuffer 的操作完成
        err = vkWaitForFences( m_device, 1, &fd->Fence, VK_TRUE, UINT64_MAX );
        check_vk_result( err );
        err = vkResetFences( m_device, 1, &fd->Fence );
        check_vk_result( err );
    }
    {
        // 重置并开始记录命令缓冲
        err = vkResetCommandPool( m_device, fd->CommandPool, 0 );
        check_vk_result( err );
        VkCommandBufferBeginInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        err = vkBeginCommandBuffer( fd->CommandBuffer, &info );
        check_vk_result( err );
    }
    {
        // 开始 Render Pass
        VkRenderPassBeginInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        info.renderPass = wd->RenderPass;
        info.framebuffer = fd->Framebuffer;
        info.renderArea.extent.width = wd->Width;
        info.renderArea.extent.height = wd->Height;
        info.clearValueCount = 1;
        info.pClearValues = &wd->ClearValue;
        vkCmdBeginRenderPass( fd->CommandBuffer, &info, VK_SUBPASS_CONTENTS_INLINE );
    }

    // 记录 ImGui 绘制命令
    ImGui_ImplVulkan_RenderDrawData( draw_data, fd->CommandBuffer );

    // 结束 Render Pass 并提交命令缓冲
    vkCmdEndRenderPass( fd->CommandBuffer );
    {
        VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        VkSubmitInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        info.waitSemaphoreCount = 1;
        info.pWaitSemaphores = &image_acquired_semaphore;
        info.pWaitDstStageMask = &wait_stage;
        info.commandBufferCount = 1;
        info.pCommandBuffers = &fd->CommandBuffer;
        info.signalSemaphoreCount = 1;
        info.pSignalSemaphores = &render_complete_semaphore;

        err = vkEndCommandBuffer( fd->CommandBuffer );
        check_vk_result( err );
        err = vkQueueSubmit( m_queue, 1, &info, fd->Fence );  // 使用围栏确保命令完成
        check_vk_result( err );
    }
}

void ImGuiApp::PresentFrame() {
    if ( m_swapChainRebuild ) {
        return;  // 如果标记了重建，则不进行呈现
    }

    ImGui_ImplVulkanH_Window* wd = &m_mainWindowData;
    VkSemaphore render_complete_semaphore = wd->FrameSemaphores[ wd->SemaphoreIndex ].RenderCompleteSemaphore;
    VkPresentInfoKHR info = {};
    info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    info.waitSemaphoreCount = 1;
    info.pWaitSemaphores = &render_complete_semaphore;
    info.swapchainCount = 1;
    info.pSwapchains = &wd->Swapchain;
    info.pImageIndices = &wd->FrameIndex;

    VkResult err = vkQueuePresentKHR( m_queue, &info );
    if ( err == VK_ERROR_OUT_OF_DATE_KHR || err == VK_SUBOPTIMAL_KHR ) {
        m_swapChainRebuild = true;  // 标记需要重建
    }
    else if ( err != VK_SUCCESS ) {
        check_vk_result( err );  // 处理其他呈现错误
    }

    // 移动到下一个信号量索引
    wd->SemaphoreIndex = ( wd->SemaphoreIndex + 1 ) % wd->SemaphoreCount;
}

void ImGuiApp::HandleResize( int newWidth, int newHeight ) {
    if ( newWidth <= 0 || newHeight <= 0 )
        return;

    // 等待设备空闲，确保没有操作正在使用旧的交换链资源
    VkResult device_wait_err = vkDeviceWaitIdle( m_device );
    check_vk_result( device_wait_err );  // 最好处理这个错误，例如记录日志或抛出异常

    // --- 重建 Vulkan 窗口资源 ---
    // (可选) 设置新的最小镜像数量，如果你允许它动态变化
    // ImGui_ImplVulkan_SetMinImageCount(m_minImageCount); // 通常 m_minImageCount 是固定的

    ImGui_ImplVulkanH_CreateOrResizeWindow( m_instance, m_physicalDevice, m_device,
                                            &m_mainWindowData,  // 传递成员变量
                                            m_queueFamily, m_allocator, newWidth, newHeight, m_minImageCount );

    // 检查重建是否成功
    if ( m_mainWindowData.Swapchain == VK_NULL_HANDLE ) {
        fprintf( stderr, "Error: Failed to resize Vulkan swapchain window resources.\n" );
        m_running = false;  // 标记应用停止运行
        // 或者抛出异常: throw std::runtime_error("Failed to resize Vulkan swapchain window
        // resources.");
        return;
    }

    m_mainWindowData.FrameIndex = 0;  // 重置帧索引
    m_swapChainRebuild = false;       // 清除重建标记
    m_minimized = false;              // 窗口不再是最小化状态
    printf( "Swapchain resized to %dx%d\n", newWidth, newHeight );

    // --- 重新初始化 ImGui Vulkan 后端 ---
    // 先销毁旧的 ImGui Vulkan 实例
    ImGui_ImplVulkan_Shutdown();

    // 重新构建 InitInfo 结构，使用更新后的 RenderPass 和 ImageCount
    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance = m_instance;
    init_info.PhysicalDevice = m_physicalDevice;
    init_info.Device = m_device;
    init_info.QueueFamily = m_queueFamily;
    init_info.Queue = m_queue;
    init_info.PipelineCache = m_pipelineCache;  // 通常是 VK_NULL_HANDLE 或之前创建的缓存
    init_info.DescriptorPool = m_descriptorPool;
    init_info.Subpass = 0;
    init_info.MinImageCount = m_minImageCount;           // 使用当前的最小图像数
    init_info.ImageCount = m_mainWindowData.ImageCount;  // **使用更新后的图像数**
    init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;       // 或你使用的 MSAA 级别
    init_info.Allocator = m_allocator;
    init_info.CheckVkResultFn = check_vk_result;
    // **关键：使用新创建或调整大小后的 RenderPass**
    init_info.RenderPass = m_mainWindowData.RenderPass;
    // 如果使用 Vulkan 1.3 动态渲染，还需要设置 PipelineRenderingCreateInfo

    // 使用新的 InitInfo 重新初始化
    if ( !ImGui_ImplVulkan_Init( &init_info ) ) {
        fprintf( stderr, "Error: Failed to re-initialize ImGui Vulkan backend after resize.\n" );
        m_running = false;  // 标记应用停止运行
        // 或者抛出异常: throw std::runtime_error("Failed to re-initialize ImGui Vulkan backend
        // after resize.");
    }
    else {
        // 字体纹理需要重新上传，因为之前的资源可能已销毁
        m_fontsLoaded = false;  // 标记字体需要重新上传
        // UploadFonts(); // 不需要在这里显式调用，BeginFrame() 会检查并调用
        printf( "ImGui Vulkan backend re-initialized after resize.\n" );
    }
}

// --- Static Helper Functions ---

void ImGuiApp::check_vk_result( VkResult err ) {
    if ( err == VK_SUCCESS )
        return;
    fprintf( stderr, "[vulkan] Error: VkResult = %d\n", err );
    if ( err < 0 ) {
        // 抛出异常而不是 abort，以便可以捕获
        throw std::runtime_error( "[vulkan] Fatal error encountered." );
        // abort(); // 或者根据需要决定是否终止
    }
}

#ifdef APP_USE_VULKAN_DEBUG_REPORT
VKAPI_ATTR VkBool32 VKAPI_CALL ImGuiApp::debug_report( VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objectType,
                                                       uint64_t object, size_t location, int32_t messageCode,
                                                       const char* pLayerPrefix, const char* pMessage, void* pUserData ) {
    ( void )flags;
    ( void )object;
    ( void )location;
    ( void )messageCode;
    ( void )pUserData;
    ( void )pLayerPrefix;  // Unused
    fprintf( stderr, "[vulkan] Debug report from ObjectType: %i\nMessage: %s\n\n", objectType, pMessage );
    return VK_FALSE;  // 返回 VK_FALSE 表示不中止触发回调的操作
}
#endif

bool ImGuiApp::IsExtensionAvailable( const ImVector<VkExtensionProperties>& properties, const char* extension ) {
    for ( const VkExtensionProperties& p : properties )
        if ( strcmp( p.extensionName, extension ) == 0 )
            return true;
    return false;
}