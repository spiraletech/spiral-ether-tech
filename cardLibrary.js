(function initCardLibrary(global) {
  const cardLibrary = {
    Idle: {
      id: "idle",
      name: "Idle",
      type: "stance",
      rarity: "common",
      color: "#94a3b8",
      slot: "base",
      tags: ["baseline", "movement"],
      icon: "🜁",
      description: "Baseline idle sway",
      priority: 0,
      duration: Infinity,
      blend: "add",
      sample(t) {
        return {
          rightArm: Math.sin(t * 2.0) * 0.25,
          leftArm: Math.sin(t * 2.0 + Math.PI) * 0.18,
          rightLeg: Math.sin(t * 2.0 + Math.PI) * 0.12,
          leftLeg: Math.sin(t * 2.0) * 0.12,
          torsoTilt: Math.sin(t * 1.2) * 0.03
        };
      }
    },
    Strike: {
      id: "strike",
      name: "Strike",
      type: "attack",
      rarity: "rare",
      color: "#fda4af",
      slot: "active",
      tags: ["melee", "burst"],
      icon: "⚔️",
      description: "Aggressive attack override",
      priority: 2,
      duration: 0.8,
      blend: "override",
      sample(t) {
        return {
          rightArm: -0.8 + Math.sin(t * 8) * 0.15,
          leftArm: 0.35,
          rightLeg: 0.18,
          leftLeg: -0.18,
          torsoTilt: -0.08
        };
      }
    },
    Glitch: {
      id: "glitch",
      name: "Glitch",
      type: "signal",
      rarity: "epic",
      color: "#60a5fa",
      slot: "active",
      tags: ["distortion", "arcane"],
      icon: "✦",
      description: "Corrupted signal animation",
      priority: 1,
      duration: 1.25,
      blend: "add",
      sample(t) {
        return {
          rightArm: Math.sin(t * 10) * 0.9,
          leftArm: Math.cos(t * 8) * 0.7,
          rightLeg: Math.sin(t * 7) * 0.35,
          leftLeg: Math.cos(t * 7) * 0.35,
          torsoTilt: Math.sin(t * 9) * 0.12
        };
      }
    }
  };

  global.SpiralCardLibrary = { cardLibrary };
})(window);
