#include "application.hpp"
#include <stdio.h>

static char status_msg[512] = "就绪";

#define OUTPUT_ERROR_MESSAGE() \
    do { \
    char* error_msg = m_pManager->get_last_error_message(); \
    sprintf(status_msg, "%s", error_msg); \
    } while(1);

Application::Application()
{
    init();
}

Application::~Application()
{
    shutdown();
}

void Application::init()
{
    if (SDL_Init(SDL_INIT_EVERYTHING) != 0)
    {
        SDL_Log("Error: %s\n", SDL_GetError());
        return;
    }

    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_RESIZABLE);
    m_pWindow = SDL_CreateWindow("MCenter", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 480, 320, window_flags);
    
    m_pRenderer = SDL_CreateRenderer(m_pWindow, -1, SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED);
    if (NULL == m_pRenderer) 
    {
        SDL_Log("Error create SDL_Renderer!");
        return;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext(); 

    ImGui::StyleColorsDark();

    ImGui_ImplSDL2_InitForSDLRenderer(m_pWindow);
    ImGui_ImplSDLRenderer_Init(m_pRenderer);

    m_pManager = new MobileManager(NULL, 0, "./xixi.cfg");
}

void Application::write_location(bool reset)
{
    double lng, lat;
    if (!m_pManager->check_device(NULL)) 
    {
        OUTPUT_ERROR_MESSAGE();
        return;
    }

    if (!m_pManager->connect_device()) 
    {
        OUTPUT_ERROR_MESSAGE();
        return;
    }

    if (m_pManager->simulate_location(reset, &lng, &lat))
    {
        sprintf(status_msg, "成功!经度: %.6f, 纬度: %.6f", lng, lat); 
    } else {
        OUTPUT_ERROR_MESSAGE();
        return;
    }
}


int Application::run(int argc, char* argv[])
{
    ImGuiIO& io = ImGui::GetIO();(void)io;    
    ImFont* font3 = io.Fonts->AddFontFromFileTTF("font.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesChineseFull());
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    static bool flag = true;
    static int counter = 1;

    static double lat1 = 30.290670;
    static double lat2 = 30.291925;

    static double lng1 = 120.067904;
    static double lng2 = 120.069884;

    bool done = false;
    while (!done)
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL2_ProcessEvent(&event);

            if (event.type == SDL_QUIT)
                done = true;
            if (event.type == SDL_WINDOWEVENT && event.type == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(m_pWindow))
                done = true;
        }

        ImGui_ImplSDLRenderer_NewFrame();
        ImGui_ImplSDL2_NewFrame(m_pWindow);
        ImGui::NewFrame();

        // UI
        const ImGuiViewport* main_viewport = ImGui::GetMainViewport();
        ImGuiWindowFlags win_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings;
        ImGui::SetNextWindowPos(main_viewport->WorkPos);
        ImGui::SetNextWindowSize(main_viewport->WorkSize);

        if (flag) 
        {
            ImGui::Begin("MCenter", &flag, win_flags);
            ImGui::PushFont(font3);

            ImGui::Text("状态：\n%s", status_msg);
            if (ImGui::Button("写入随机位置", ImVec2(230, 50)))
            {
                write_location(false);
            }
            
            ImGui::SameLine();
            if (ImGui::Button("重置", ImVec2(230, 50))) 
            {
                write_location(true);
            }    
            
            static bool show_setting = false;
            ImGui::Checkbox("显示坐标范围参数", &show_setting);
            if (show_setting)
            {
                ImGui::InputDouble("经度起始", &lng1, 0, 0, "%.6f");
                ImGui::InputDouble("经度结束", &lng2, 0, 0, "%.6f");
                ImGui::InputDouble("纬度起始", &lat1, 0, 0, "%.6f");
                ImGui::InputDouble("纬度结束", &lat2, 0, 0, "%.6f");  

                ImGui::Separator();
                if (ImGui::Button("应用参数"))
                {
                    m_pManager->update_location_range(lng1, lng2, lat1, lat2);
                    sprintf(status_msg, "参数已更新。"); 
                }              
            }
            ImGui::PopFont();

            ImGui::End();
        }

        //Rendering
        ImGui::Render();
        SDL_SetRenderDrawColor(m_pRenderer,
            (Uint8)(clear_color.x * 255),
            (Uint8)(clear_color.y * 255),
            (Uint8)(clear_color.z * 255),
            (Uint8)(clear_color.w * 255)
        );

        SDL_RenderClear(m_pRenderer);
        ImGui_ImplSDLRenderer_RenderDrawData(ImGui::GetDrawData());
        SDL_RenderPresent(m_pRenderer);

        SDL_Delay(5);
    }

    return 0;
}

void Application::shutdown()
{
    // Cleanup
    ImGui_ImplSDLRenderer_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_DestroyRenderer(m_pRenderer);
    SDL_DestroyWindow(m_pWindow);
    SDL_Quit();

    delete m_pManager;
}