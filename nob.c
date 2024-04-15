#define NOB_IMPLEMENTATION
#include "./src/nob.h"


// Uncomment your platform (linux, macos) here and comment out windows
#define WINDOWS
//#define LINUX
//#define MACOS

int main(int argc, char **argv) {
	NOB_GO_REBUILD_URSELF(argc, argv);

	Nob_Cmd cmd = {0};

#ifdef MACOS
	nob_log(NOB_INFO, "Compiling for Mac OS\n");
	nob_cmd_append(&cmd, "clang");
    //nob_cmd_append(&cmd, "-Wall", "-Wextra", "-g", "-lm");
    nob_cmd_append(&cmd, "-o", "main");
    nob_cmd_append(&cmd, "./src/main.c");
#endif //MACOS

#ifdef LINUX
	nob_log(NOB_INFO, "Compiling for Linux\n");
    nob_cmd_append(&cmd, "cc");
    //nob_cmd_append(&cmd, "-Wall", "-Wextra", "-ggdb");
    nob_cmd_append(&cmd, "-o", "main");
    nob_cmd_append(&cmd, "./src/main.c");
    nob_cmd_append(&cmd, "-lm");
#endif // LINUX


#ifdef WINDOWS
    nob_log(NOB_INFO, "Compiling for Windows\n");
	nob_cmd_append(&cmd, "cl.exe");
	//nob_cmd_append(&cmd, "/Debug");
	nob_cmd_append(&cmd, "/Ox");
	//nob_cmd_append(&cmd, "/Wall");
	nob_cmd_append(&cmd, "./src/main.c");
#endif // WINDOWS


	if (!nob_cmd_run_sync(cmd)) {
		return 1;
	}


	/* // RUN ON WINDOWS
	cmd.count = 0;
	nob_cmd_append(&cmd, "main.exe");

	if (!nob_cmd_run_sync(cmd)) {
		return 1;
	} */


	return 0;
}