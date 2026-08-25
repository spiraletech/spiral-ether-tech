(function initEngine(global) {
  class Engine {
    constructor(update, render) {
      this.update = update;
      this.render = render;
      this.last = 0;
    }

    start() {
      requestAnimationFrame((ts) => this.loop(ts));
    }

    loop(timestamp) {
      const dt = (timestamp - this.last) / 1000 || 0;
      this.last = timestamp;
      this.update(dt);
      this.render();
      requestAnimationFrame((ts) => this.loop(ts));
    }
  }

  global.SpiralEngine = { Engine };
})(window);
