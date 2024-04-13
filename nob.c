
#define NOB_IMPLEMENTATION
#include "nob.h"


int main(int argc, char **argv) {
	NOB_GO_REBUILD_URSELF(argc, argv);

	Nob_Cmd cmd = {0};
	nob_cmd_append(&cmd, "cl.exe");
	nob_cmd_append(&cmd, "/Wall");
	nob_cmd_append(&cmd, "main.c");

	// linker
	//nob_cmd_append(%cmd. "/link");
	//nob_cmd_append(%cmd. "asdf.lib");

	if (!nob_cmd_run_sync(cmd)) {
		return 1;
	}

	cmd.count = 0;
	nob_cmd_append(&cmd, "main.exe");

	if (!nob_cmd_run_sync(cmd)) {
		return 1;
	}


	return 0;
}