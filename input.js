(function initInput(global) {
  class InputManager {
    constructor({ onPulse, onSetIdle, onToggleStrike, onToggleGlitch }) {
      this.onPulse = onPulse;
      this.onSetIdle = onSetIdle;
      this.onToggleStrike = onToggleStrike;
      this.onToggleGlitch = onToggleGlitch;
    }

    bind() {
      window.addEventListener("keydown", (event) => {
        if (event.key === "1") {
          this.onSetIdle();
          this.onPulse();
        }
        if (event.key === "2") {
          this.onToggleStrike();
          this.onPulse();
        }
        if (event.key === "3") {
          this.onToggleGlitch();
          this.onPulse();
        }
      });
    }
  }

  global.SpiralInput = { InputManager };
})(window);
