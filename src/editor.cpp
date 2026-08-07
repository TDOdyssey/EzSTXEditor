#include "editor.h"

#include "imgui.h"

#include "stx.h"

#include <cstdio>
#include <Stringapiset.h>

#include <windows.h>
#include <commdlg.h>

static STXFile *file = NULL;
static int selected = -1;
static int loaded_id = -1;
static char edit_buffer[4096];
static char filename[MAX_PATH];

void draw_dialogue_list(STXFile *file)
{
    ImGui::BeginChild("DialogueList", ImVec2(500, 0), true);

    for(int i = 0; i < file->string_count; i++)
    {
        char label[256];

        snprintf(label, sizeof(label), "%d: %ls", i, file->strings[i]);

        if(ImGui::Selectable(label, selected == i))
            selected = i;
    }

    ImGui::EndChild();
}

void editor_draw()
{
    ImGuiViewport *viewport = ImGui::GetMainViewport();

    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(viewport->Size);

    ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoBringToFrontOnFocus;

    ImGui::Begin("Main", nullptr, flags);

    if(file)
    {
        if(ImGui::Button("Save"))
        {
            stx_save(file, filename);
            printf("Saved to \"%s\"\n", filename);
        }

        ImGui::SameLine();

        if(ImGui::Button("Save As"))
        {
            char new_filename[MAX_PATH];
            OPENFILENAMEA ofn = {};
            ofn.lStructSize = sizeof(ofn);
            ofn.lpstrFile = new_filename;
            ofn.nMaxFile = MAX_PATH;
            ofn.lpstrFilter =
                "STX files\0*.stx\0";

            if(GetSaveFileNameA(&ofn))
            {
                stx_save(file, new_filename);
                printf("Saved to \"%s\"\n", new_filename);
            }
        }

        ImGui::SameLine();
    }

    if(ImGui::Button("Load"))
    {
        OPENFILENAMEA ofn = {};
        ofn.lStructSize = sizeof(ofn);
        ofn.lpstrFile = filename;
        ofn.nMaxFile = MAX_PATH;
        ofn.lpstrFilter =
            "STX files\0*.stx\0All files\0*.*\0";

        if(GetOpenFileNameA(&ofn))
        {
            if(file)
                stx_free(file);
            file = stx_load(filename);
        }
    }

    if(file)
    {
        ImGui::Text(
            "Strings: %d",
            file->string_count
        );

        draw_dialogue_list(file);

        ImGui::SameLine();

        ImGui::BeginChild("Editor", ImVec2(0, 0), true);

        if(selected >= 0)
        {
            if(selected != loaded_id)
            {
                WideCharToMultiByte(CP_UTF8, 0, file->strings[selected], -1, edit_buffer, sizeof(edit_buffer), NULL, NULL);
                loaded_id = selected;
            }

            ImGui::Text("ID: %d", selected);

            if(ImGui::InputTextMultiline(
                "##text", 
                edit_buffer,
                sizeof(edit_buffer),
                ImVec2(-1, -1)))
            {
                wchar_t new_text[8192];
                MultiByteToWideChar(CP_UTF8, 0, edit_buffer, -1, new_text, 8192);

                free(file->strings[selected]);
                file->strings[selected] = wcsdup(new_text);
            }
        }

        ImGui::EndChild();
    }
}

void drop_callback(GLFWwindow* window, int count, const char** paths)
{
    if(count > 0)
        file = stx_load(paths[0]);

    strcpy(filename, paths[0]);
}