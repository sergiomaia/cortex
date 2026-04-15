export class Controller {
  constructor(context) { this.context = context; this.element = context.element; this.identifier = context.identifier; this.targetsByName = context.targetsByName; }
  connect() {}
  disconnect() {}
  target(name) { const list = this.targetsByName[name] || []; return list[0] || null; }
  targets(name) { return this.targetsByName[name] || []; }
  hasTarget(name) { return this.targets(name).length > 0; }
}
export class Application {
  constructor() { this.registry = new Map(); this.instances = []; }
  register(identifier, ControllerClass) { this.registry.set(identifier, ControllerClass); }
  start() { this.connectTree(document); }
  connectTree(root) { root.querySelectorAll('[data-controller]').forEach((element) => this.connectElement(element)); }
  connectElement(element) { const ids = (element.getAttribute('data-controller') || '').trim().split(/\s+/).filter(Boolean); ids.forEach((identifier) => { const ControllerClass = this.registry.get(identifier); if (!ControllerClass) { console.error(`Cortex Stimulus: controller \"${identifier}\" not found`); return; } const targetsByName = this.collectTargets(identifier, element); const instance = new ControllerClass({ element, identifier, targetsByName }); this.instances.push(instance); if (typeof instance.connect === 'function') instance.connect(); this.bindActions(identifier, instance, element); }); }
  collectTargets(identifier, element) { const map = {}; element.querySelectorAll(`[data-${identifier}-target]`).forEach((node) => { const names = (node.getAttribute(`data-${identifier}-target`) || '').split(/\s+/).filter(Boolean); names.forEach((name) => { map[name] = map[name] || []; map[name].push(node); }); }); return map; }
  bindActions(identifier, instance, root) { const nodes = [root, ...root.querySelectorAll('[data-action]')]; nodes.forEach((node) => { const actions = (node.getAttribute('data-action') || '').split(/\s+/).filter(Boolean); actions.forEach((action) => { const m = action.match(/^([^->]+)->([^#]+)#(.+)$/); if (!m) { console.warn(`Cortex Stimulus: invalid action \"${action}\"`); return; } const eventName = m[1]; const actionController = m[2]; const methodName = m[3]; if (actionController !== identifier) return; if (typeof instance[methodName] !== 'function') { console.warn(`Cortex Stimulus: missing method ${identifier}#${methodName}`); return; } node.addEventListener(eventName, (event) => instance[methodName](event)); }); }); }
}
