import { Application } from "../../js/runtime/index.js";
import { registerControllers } from "./controllers/index.js";

const application = new Application();
registerControllers(application);
application.start();
