#define NOB_IMPLEMENTATION
#include "nob.h"

int main(int argc, char **argv)
{
	NOB_GO_REBUILD_URSELF(argc, argv);

	Nob_Cmd cmd = {0};

	nob_cmd_append(&cmd, "g++", "-std=c++23", "-Wall", "-Wextra", "-O0", "-ggdb", "-o", "snake_game");
	// kernel part
	nob_cmd_append(&cmd, "./src/kernel/ecs.cpp", "./src/kernel/game_state.cpp");
	// snake game part
	nob_cmd_append(&cmd, "./src/snake_game/field_track.cpp", "./src/snake_game/game_context.cpp", "./src/snake_game/items.cpp");
	nob_cmd_append(&cmd, "./src/snake_game/snake.cpp", "./src/snake_game/systems.cpp");
	nob_cmd_append(&cmd, "./src/snake_game/level.cpp");
	// app
	nob_cmd_append(&cmd, "./src/main.cpp");
	// libs and include
	nob_cmd_append(&cmd, "-lSDL3", "-lSDL3_ttf", "-I./include", "-I./");

	if (!nob_cmd_run(&cmd))
		return 1;

	return 0;
}
