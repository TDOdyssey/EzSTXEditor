CXX = g++
CC = gcc

CXXFLAGS = -std=c++17 -Wall -Iimgui -Iimgui/backends -Isrc -D__USE_MINGW_ANSI_STDIO=0
CFLAGS = -Wall -Isrc

LDFLAGS = -lglfw3 -lopengl32 -lgdi32 -lcomdlg32 -static-libstdc++

TARGET = bin/eztt_gui

CPP_SOURCES = \
	src/main.cpp \
	src/editor.cpp \
	imgui/imgui.cpp \
	imgui/imgui_draw.cpp \
	imgui/imgui_tables.cpp \
	imgui/imgui_widgets.cpp \
	imgui/backends/imgui_impl_glfw.cpp \
	imgui/backends/imgui_impl_opengl3.cpp

C_SOURCES = \
	src/stx.c

CPP_OBJECTS = $(CPP_SOURCES:%.cpp=obj/%.o)
C_OBJECTS = $(C_SOURCES:%.c=obj/%.o)

all: $(TARGET)

$(TARGET): $(CPP_OBJECTS) $(C_OBJECTS)
	@mkdir -p $(@D)
	$(CXX) $^ -o $@ $(LDFLAGS)

obj/%.o: %.cpp
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -c $< -o $@

obj/%.o: %.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm $(CPP_OBJECTS) $(C_OBJECTS) $(TARGET).exe