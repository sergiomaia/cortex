#include "cli.h"
#include "../core/main.h"

int main(int argc, char **argv) {
    int rc;

    cortex_main_bootstrap();
    rc = cli_run(argc, argv);
    cortex_main_shutdown();
    return rc;
}
