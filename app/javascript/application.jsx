import React from "react";
import { createRoot } from "react-dom/client";
import { componentRegistry } from "./resources/index.jsx";

function parseProps(node) {
  const raw = node.getAttribute("data-react-props") || "{}";
  try { return JSON.parse(raw); } catch (_err) { return {}; }
}

function mountReactIslands() {
  const nodes = document.querySelectorAll("[data-react-component]");
  nodes.forEach((node) => {
    const name = node.getAttribute("data-react-component");
    const Component = componentRegistry[name];
    if (!Component) {
      console.warn(`Cortex React: component '${name}' not found`);
      return;
    }
    const props = parseProps(node);
    const root = createRoot(node);
    root.render(React.createElement(Component, props));
  });
}

if (document.readyState === "loading") {
  document.addEventListener("DOMContentLoaded", mountReactIslands);
} else {
  mountReactIslands();
}
