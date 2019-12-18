#include <glad/glad.h>

#include "imgui.h"
#include "imgui_impl_sdl.h"
#include "imgui_impl_opengl3.h"

#include <SDL2/SDL.h>

#include <iostream>

int windowWidth = 640;
int windowHeight = 480;

int main( int argc, char** argv )
{
	if ( SDL_Init( SDL_INIT_VIDEO ) != 0 )
		return 1;

	SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );
	SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, 24 );
	SDL_GL_SetAttribute( SDL_GL_STENCIL_SIZE, 8 );

	SDL_GL_SetAttribute( SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE );

	const char* glslVersion = "#version 130";
    SDL_GL_SetAttribute( SDL_GL_CONTEXT_FLAGS, 0); 
    SDL_GL_SetAttribute( SDL_GL_CONTEXT_MAJOR_VERSION, 3 );
    SDL_GL_SetAttribute( SDL_GL_CONTEXT_MINOR_VERSION, 2 );

    SDL_WindowFlags windowFlags = (SDL_WindowFlags)( SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI );

    SDL_Window* window = SDL_CreateWindow( "Imgui test", -1, -1, windowWidth, windowHeight, windowFlags );

    SDL_SetWindowMinimumSize( window, 640, 480 );

    SDL_GLContext glContext = SDL_GL_CreateContext( window );
    SDL_GL_MakeCurrent( window, glContext );

    SDL_GL_SetSwapInterval( 1 );

    if ( !gladLoadGLLoader( (GLADloadproc)SDL_GL_GetProcAddress ) )
    	return 2;

    glViewport( 0, 0, windowWidth, windowHeight );

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    ImGui::StyleColorsDark();

    ImGui_ImplSDL2_InitForOpenGL( window, glContext );
    ImGui_ImplOpenGL3_Init( glslVersion );

    ImVec4 bg( 0.137f, 0.137f, 0.137f, 1.0f );

    glClearColor( bg.x, bg.y, bg.z, bg.w );

    bool loop = true;
    while( loop )
    {
    	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT );

    	SDL_Event event;
    	while( SDL_PollEvent( &event ) )
    	{
    		ImGui_ImplSDL2_ProcessEvent( &event );

    		switch( event.type )
    		{
    			case SDL_QUIT:
    				loop = false;
    				break;

    			case SDL_WINDOWEVENT:
    				switch( event.window.event )
    				{
    					case SDL_WINDOWEVENT_RESIZED:
    						windowWidth = event.window.data1;
    						windowHeight = event.window.data2;
    						glViewport( 0, 0, windowWidth, windowHeight );
    						break;
    				}
    				break;
    		}
    	}

    	ImGui_ImplOpenGL3_NewFrame();
    	ImGui_ImplSDL2_NewFrame( window );
    	ImGui::NewFrame();

        // a window is defined by Begin/End pair
        {
            static int counter = 0;
            // get the window size as a base for calculating widgets geometry
            int sdl_width = 0, sdl_height = 0, controls_width = 0;
            SDL_GetWindowSize(window, &sdl_width, &sdl_height);
            controls_width = sdl_width;
            // make controls widget width to be 1/3 of the main window width
            if ((controls_width /= 3) < 300) { controls_width = 300; }

            // position the controls widget in the top-right corner with some margin
            ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Always);
            // here we set the calculated width and also make the height to be
            // be the height of the main window also with some margin
            ImGui::SetNextWindowSize(
                ImVec2(static_cast<float>(controls_width), static_cast<float>(sdl_height - 20)),
                ImGuiCond_Always
                );
            // create a window and append into it
            ImGui::Begin("Controls", NULL, ImGuiWindowFlags_NoResize);

            ImGui::Dummy(ImVec2(0.0f, 1.0f));
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 1.0f, 1.0f), "Platform");
            ImGui::Text("%s", SDL_GetPlatform());
            ImGui::Text("CPU cores: %d", SDL_GetCPUCount());
            ImGui::Text("RAM: %.2f GB", SDL_GetSystemRAM() / 1024.0f);
            
            // buttons and most other widgets return true when clicked/edited/activated
            if (ImGui::Button("Counter button"))
            {
                std::cout << "counter button clicked\n";
                counter++;
            }
            ImGui::SameLine();
            ImGui::Text("counter = %d", counter);

            ImGui::End();
        }

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData( ImGui::GetDrawData() );

        SDL_GL_SwapWindow( window );
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DeleteContext( glContext );
    SDL_DestroyWindow( window );
    SDL_Quit();

	return 0;
}