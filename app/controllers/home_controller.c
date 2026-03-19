/* Default Home controller. */
#include "action_controller.h"
#include "action_view.h"

void home_index(ActionRequest *req, ActionResponse *res) {
    (void)req;
    render_view(res, "home/index");
}

