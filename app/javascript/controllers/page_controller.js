import { Controller } from "../../../js/runtime/index.js";

export default class extends Controller {
  static targets = ["form", "output"];

  connect() {
    /* Runs when data-controller="page" is connected. */
  }

  submit(event) {
    if (event) event.preventDefault();
    const value = this.hasTarget("output") ? this.target("output").textContent : "";
    if (this.hasTarget("output")) this.target("output").textContent = value || "submitted";
  }
}
