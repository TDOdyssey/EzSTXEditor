#include "editor.h"

#include "imgui.h"

#include "stx.h"

#include <cstdio>
#include <Stringapiset.h>

#include <windows.h>
#include <commdlg.h>

#include <string>

typedef struct EditorTab_s
{
    STXFile *file = NULL;
    int selected = -1;
    int loaded_id = -1;
    char filename[MAX_PATH] = "\0";
    bool modified = false;

    uint32_t id = 0; // imgui internal id

    EditorTab_s *next = NULL;
} EditorTab;

static EditorTab *head = NULL;
static uint32_t next_id = 0; // stable id so that imgui doesn't get fucked up with labels

static char edit_buffer[4096];

void draw_dialogue_list(EditorTab *tab);

void draw_tab(EditorTab *tab)
{
    if(!tab)
        return;

    if(tab->file)
    {
        if(ImGui::Button("Save"))
        {
            stx_save(tab->file, tab->filename);
            printf("Saved to \"%s\"\n", tab->filename);
            tab->modified = false;
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
                stx_save(tab->file, new_filename);
                printf("Saved to \"%s\"\n", new_filename);
            }
        }

        ImGui::SameLine();
    }

    if(ImGui::Button("Load"))
    {
        OPENFILENAMEA ofn = {};
        ofn.lStructSize = sizeof(ofn);
        ofn.lpstrFile = tab->filename;
        ofn.nMaxFile = MAX_PATH;
        ofn.lpstrFilter =
            "STX files\0*.stx\0All files\0*.*\0";

        if(GetOpenFileNameA(&ofn))
        {
            if(tab->file)
                stx_free(tab->file);
            tab->file = stx_load(tab->filename);
        }
    }

    if (head == NULL)
        ImGui::Text("Drop one or more STX file(s) here...");

    if(tab->file)
    {
        ImGui::Text(
            "Strings: %d",
            tab->file->string_count
        );

        draw_dialogue_list(tab);

        ImGui::SameLine();

        ImGui::BeginChild("Editor", ImVec2(0, 0), true);

        if(tab->selected >= 0)
        {
            if(tab->selected != tab->loaded_id)
            {
                WideCharToMultiByte(CP_UTF8, 0, tab->file->strings[tab->selected], -1, edit_buffer, sizeof(edit_buffer), NULL, NULL);
                tab->loaded_id = tab->selected;
            }

            ImGui::Text("ID: %d", tab->selected);

            if(ImGui::InputTextMultiline(
                "##text", 
                edit_buffer,
                sizeof(edit_buffer),
                ImVec2(-1, -1)))
            {
                wchar_t new_text[8192];
                MultiByteToWideChar(CP_UTF8, 0, edit_buffer, -1, new_text, 8192);

                free(tab->file->strings[tab->selected]);
                tab->file->strings[tab->selected] = wcsdup(new_text);

                tab->modified = true;
            }
        }

        ImGui::EndChild();
    }
}

void draw_tabs()
{
    if(ImGui::BeginTabBar("Files", ImGuiTabBarFlags_FittingPolicyShrink/* | ImGuiTabBarFlags_Reorderable*/))
    {
        EditorTab *cursor = head;
        EditorTab *prev = NULL;
        while(cursor != NULL)
        {
            ImGuiTabItemFlags flags = (cursor->modified ? ImGuiTabItemFlags_UnsavedDocument : 0);
            bool open = true;

            char label[512];

            std::string filename_without_path = cursor->filename;
            filename_without_path = filename_without_path.substr(filename_without_path.find_last_of("/\\") + 1);

            sprintf(label, "%s##%u", cursor->file ? filename_without_path.c_str() : "Empty", cursor->id);
            //sprintf(label, "%s##%u", cursor->file ? filename_without_path.c_str() : "Empty", cursor->id);
            //sprintf(label, "##%u", cursor->id);
            if(ImGui::BeginTabItem(label , &open, flags))
            {
                draw_tab(cursor);
                ImGui::EndTabItem();
            }

            if(!open)
            {
                stx_free(cursor->file);
                
                EditorTab *temp = cursor;

                if(prev)
                {
                    cursor = cursor->next;
                    prev->next = cursor;
                }
                else
                {
                    head = cursor->next;
                    cursor = head;
                }

                delete temp;
            }
            else
            {
                prev = cursor;
                cursor = cursor->next;
            }
        }

        if(ImGui::TabItemButton("+", ImGuiTabItemFlags_Trailing))
        {
            EditorTab *new_tab = new EditorTab();
            new_tab->id = next_id++;
            if(!prev)
                head = new_tab;
            else
                prev->next = new_tab;
        }
    }

    ImGui::EndTabBar();
}

void draw_dialogue_list(EditorTab *tab)
{
    ImGui::BeginChild("DialogueList", ImVec2(1000, 0), true);

    if(!tab)
        return;

    if(!tab->file)
        return;

    for(int i = 0; i < tab->file->string_count; i++)
    {
        char label[256];

        snprintf(label, sizeof(label), "%d: %ls", i, tab->file->strings[i]);

        if(ImGui::Selectable(label, tab->selected == i))
            tab->selected = i;
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

    draw_tabs();

    ImGui::End();
}

void drop_callback(GLFWwindow* window, int count, const char** paths)
{

    EditorTab *last_tab = head;

    while(last_tab && last_tab->next)
        last_tab = last_tab->next;

    for(int i = 0; i < count; i++)
    {
        STXFile *file = stx_load(paths[i]);
        if(file)
        {
            EditorTab *new_tab = new EditorTab();
            new_tab->id = next_id++;
            new_tab->file = file;
            strcpy(new_tab->filename, paths[i]);

            if(!last_tab)
                head = new_tab;
            else
                last_tab->next = new_tab;

            last_tab = new_tab;
        }
    }
}

void editor_cleanup()
{
    EditorTab *cursor = head;
    while(cursor)
    {
        EditorTab *next = cursor->next;
        stx_free(cursor->file);
        delete cursor;
        cursor = next;
    }
}