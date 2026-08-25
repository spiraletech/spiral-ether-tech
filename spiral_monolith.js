/* Spiral OS Mainframe Monolith (generated from modular sources) */

/* ===== engine.js ===== */
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

/* ===== skeleton.js ===== */
(function initSkeleton(global) {
  class Skeleton {
    constructor(canvas) {
      this.canvas = canvas;
      this.x = canvas.width / 2;
      this.y = canvas.height / 2 + 30;
      this.headRadius = 20;
      this.torsoLength = 72;
      this.armLength = 48;
      this.forearmLength = 42;
      this.legLength = 56;
      this.shinLength = 52;
      this.pose = this.getNeutralPose();
      this.sockets = {};
      this.updateSockets();
    }

    getNeutralPose() {
      return { rightArm: 0, leftArm: 0, rightLeg: 0, leftLeg: 0, torsoTilt: 0 };
    }

    toWorld(localX, localY) {
      const cos = Math.cos(this.pose.torsoTilt);
      const sin = Math.sin(this.pose.torsoTilt);
      return {
        x: this.x + localX * cos - localY * sin,
        y: this.y + localX * sin + localY * cos
      };
    }

    updateSockets() {
      const shoulderY = -78;
      const rightElbowX = Math.cos(this.pose.rightArm) * this.armLength;
      const rightElbowY = shoulderY + Math.sin(this.pose.rightArm) * this.armLength;
      const rightHandX = rightElbowX + Math.cos(this.pose.rightArm + 0.25) * this.forearmLength;
      const rightHandY = rightElbowY + Math.sin(this.pose.rightArm + 0.25) * this.forearmLength;

      const leftArmAngle = Math.PI + this.pose.leftArm;
      const leftElbowX = Math.cos(leftArmAngle) * this.armLength;
      const leftElbowY = shoulderY + Math.sin(leftArmAngle) * this.armLength;
      const leftHandX = leftElbowX + Math.cos(leftArmAngle - 0.25) * this.forearmLength;
      const leftHandY = leftElbowY + Math.sin(leftArmAngle - 0.25) * this.forearmLength;

      this.sockets = {
        head: this.toWorld(0, -110),
        chest: this.toWorld(0, -56),
        back: this.toWorld(-14, -52),
        rightHand: this.toWorld(rightHandX, rightHandY),
        leftHand: this.toWorld(leftHandX, leftHandY)
      };
    }

    draw(ctx, pulse, activeCardNames) {
      ctx.save();
      ctx.translate(this.x, this.y);
      ctx.rotate(this.pose.torsoTilt);

      const pulseGlow = pulse * 18;
      const hasGlitch = activeCardNames.includes("Glitch");
      ctx.shadowBlur = 16 + pulseGlow;
      ctx.shadowColor = hasGlitch ? "#60a5fa" : "#a78bfa";

      ctx.strokeStyle = "#e2e8f0";
      ctx.fillStyle = "#e2e8f0";
      ctx.lineWidth = 5;
      ctx.lineCap = "round";

      ctx.beginPath();
      ctx.arc(0, -110, this.headRadius, 0, Math.PI * 2);
      ctx.stroke();

      ctx.beginPath();
      ctx.moveTo(0, -90);
      ctx.lineTo(0, -18);
      ctx.stroke();

      this.drawJoint(ctx, 0, -90);
      this.drawJoint(ctx, 0, -18);

      const shoulderY = -78;
      const hipY = -18;
      this.drawJoint(ctx, 0, shoulderY);

      const rightElbow = this.drawLimb(ctx, 0, shoulderY, this.armLength, this.pose.rightArm);
      this.drawLimb(ctx, rightElbow.x, rightElbow.y, this.forearmLength, this.pose.rightArm + 0.25);

      const leftElbow = this.drawLimb(ctx, 0, shoulderY, this.armLength, Math.PI + this.pose.leftArm);
      this.drawLimb(ctx, leftElbow.x, leftElbow.y, this.forearmLength, Math.PI + this.pose.leftArm - 0.25);

      const rightKnee = this.drawLimb(ctx, 0, hipY, this.legLength, Math.PI / 2 + this.pose.rightLeg);
      this.drawLimb(ctx, rightKnee.x, rightKnee.y, this.shinLength, Math.PI / 2 + this.pose.rightLeg + 0.08);

      const leftKnee = this.drawLimb(ctx, 0, hipY, this.legLength, Math.PI / 2 + this.pose.leftLeg);
      this.drawLimb(ctx, leftKnee.x, leftKnee.y, this.shinLength, Math.PI / 2 + this.pose.leftLeg - 0.08);

      ctx.restore();
    }

    drawJoint(ctx, x, y, r = 4) {
      ctx.beginPath();
      ctx.arc(x, y, r, 0, Math.PI * 2);
      ctx.fill();
    }

    drawLimb(ctx, x, y, length, angle) {
      const endX = x + Math.cos(angle) * length;
      const endY = y + Math.sin(angle) * length;
      ctx.beginPath();
      ctx.moveTo(x, y);
      ctx.lineTo(endX, endY);
      ctx.stroke();
      this.drawJoint(ctx, endX, endY, 4);
      return { x: endX, y: endY };
    }
  }

  global.SpiralSkeleton = { Skeleton };
})(window);

/* ===== cardLibrary.js ===== */
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

/* ===== cards.js ===== */
(function initCards(global) {
  const cards = global.SpiralCardLibrary.cardLibrary;

  function createCardRuntime() {
    return {
      baseCard: "Idle",
      activeCards: [],
      maxCards: 2,
      baseTime: 0
    };
  }

  function activateCard(runtime, name) {
    if (!cards[name] || name === runtime.baseCard) return;
    runtime.activeCards = runtime.activeCards.filter((entry) => entry.name !== name);
    runtime.activeCards.push({ name, elapsed: 0 });
    runtime.activeCards.sort((a, b) => cards[a.name].priority - cards[b.name].priority);
    runtime.activeCards = runtime.activeCards.slice(-runtime.maxCards);
  }

  function tickCards(runtime, dt) {
    runtime.baseTime += dt;
    runtime.activeCards = runtime.activeCards
      .map((entry) => ({ name: entry.name, elapsed: entry.elapsed + dt }))
      .filter((entry) => entry.elapsed <= cards[entry.name].duration)
      .sort((a, b) => cards[a.name].priority - cards[b.name].priority);
  }

  function applyBlend(pose, sample, blend, strength) {
    for (const key of Object.keys(sample)) {
      if (blend === "override") {
        pose[key] = pose[key] * (1 - strength) + sample[key] * strength;
      } else {
        pose[key] += sample[key] * strength;
      }
    }
  }

  function applyCards(runtime, pose) {
    const base = cards[runtime.baseCard].sample(runtime.baseTime);
    applyBlend(pose, base, cards[runtime.baseCard].blend, 1);

    for (const entry of runtime.activeCards) {
      const card = cards[entry.name];
      const strength = Math.max(0, 1 - entry.elapsed / card.duration);
      const sample = card.sample(entry.elapsed);
      applyBlend(pose, sample, card.blend, strength);
    }
  }

  global.SpiralCards = { cards, createCardRuntime, activateCard, tickCards, applyCards };
})(window);

/* ===== hud.js ===== */
(function initHud(global) {
  function generateReceipt() {
    const code = Math.floor(Math.random() * 900000 + 100000);
    return {
      id: code,
      barcode: String(code)
        .split("")
        .map((d) => "|".repeat(Number(d) % 4 + 1))
        .join(" ")
    };
  }

  class Hud {
    constructor(root, cards, attachmentIndex, onSelectCard, onPulse, onEquip) {
      this.root = root;
      this.cards = cards;
      this.attachmentIndex = attachmentIndex;
      this.onSelectCard = onSelectCard;
      this.onPulse = onPulse;
      this.onEquip = onEquip;
      this.receipt = generateReceipt();
      this.selectedZone = Intl.DateTimeFormat().resolvedOptions().timeZone || "UTC";
      this.boot();
    }

    buildEquipmentOptions(socket) {
      return this.attachmentIndex[socket]
        .map((item) => `<option value="${item.id}">${item.label}</option>`)
        .join("");
    }

    boot() {
      const options = Object.keys(this.cards)
        .map((name) => `<option value="${name}">${name}</option>`)
        .join("");

      this.root.innerHTML = `
        <div><strong>SPIRAL OS // SKELETON PROTOTYPE</strong></div>
        <div class="clock" id="clock">00:00:00</div>
        <div><small id="zoneNow"></small></div>
        <label for="timezoneSelect">Timezone</label><br />
        <select id="timezoneSelect">
          ${["UTC", "America/New_York", "America/Chicago", "America/Los_Angeles", "Europe/London", "Asia/Tokyo"]
            .map((z) => `<option value="${z}" ${z === this.selectedZone ? "selected" : ""}>${z}</option>`)
            .join("")}
        </select>
        <br />
        <label for="cardSelect">Active Card</label><br />
        <select id="cardSelect">${options}</select>
        <br />
        <label for="rightHandSelect">Right Hand</label><br />
        <select id="rightHandSelect">${this.buildEquipmentOptions("rightHand")}</select>
        <br />
        <label for="backSelect">Back Slot</label><br />
        <select id="backSelect">${this.buildEquipmentOptions("back")}</select>
        <br />
        <button id="triggerBtn">Trigger Pulse</button>
        <div class="receipt">
          <div><strong>STATUS</strong></div>
          <div id="statusText">System booted.</div>
          <div style="margin-top:6px;"><code id="cardDesc">${this.cards.Idle.description}</code></div>
          <div style="margin-top:6px;" id="receiptId">RECEIPT ID: ${this.receipt.id}</div>
          <div class="barcode" id="barcode">${this.receipt.barcode}</div>
        </div>
      `;

      this.clockNode = this.root.querySelector("#clock");
      this.zoneNode = this.root.querySelector("#zoneNow");
      this.statusNode = this.root.querySelector("#statusText");
      this.cardDescNode = this.root.querySelector("#cardDesc");
      this.receiptIdNode = this.root.querySelector("#receiptId");
      this.barcodeNode = this.root.querySelector("#barcode");

      this.root.querySelector("#cardSelect").addEventListener("change", (e) => {
        const cardName = e.target.value;
        this.onSelectCard(cardName);
        this.statusNode.textContent = `${cardName} loaded.`;
        this.cardDescNode.textContent = this.cards[cardName].description;
      });

      this.root.querySelector("#rightHandSelect").addEventListener("change", (e) => {
        this.onEquip("rightHand", e.target.value);
      });

      this.root.querySelector("#backSelect").addEventListener("change", (e) => {
        this.onEquip("back", e.target.value);
      });

      this.root.querySelector("#triggerBtn").addEventListener("click", () => {
        this.onPulse();
        this.receipt = generateReceipt();
        this.receiptIdNode.textContent = `RECEIPT ID: ${this.receipt.id}`;
        this.barcodeNode.textContent = this.receipt.barcode;
        this.statusNode.textContent = "Pulse triggered.";
      });

      this.root.querySelector("#timezoneSelect").addEventListener("change", (e) => {
        this.selectedZone = e.target.value;
      });
    }

    updateRuntime(runtimeSeconds, activeCards) {
      const hh = String(Math.floor(runtimeSeconds / 3600)).padStart(2, "0");
      const mm = String(Math.floor((runtimeSeconds % 3600) / 60)).padStart(2, "0");
      const ss = String(Math.floor(runtimeSeconds % 60)).padStart(2, "0");
      this.clockNode.textContent = `${hh}:${mm}:${ss}`;
      const now = new Intl.DateTimeFormat("en-US", {
        timeZone: this.selectedZone,
        hour: "2-digit",
        minute: "2-digit",
        second: "2-digit",
        year: "numeric",
        month: "2-digit",
        day: "2-digit",
        hour12: false
      }).format(new Date());
      this.zoneNode.textContent = `TZ ${this.selectedZone}: ${now}`;
      this.statusNode.textContent = `Blend stack: ${activeCards.join(" + ")}`;
    }
  }

  global.SpiralHud = { Hud };
})(window);

/* ===== attachments.js ===== */
(function initAttachments(global) {
  const attachmentIndex = {
    head: [
      {
        id: "none",
        label: "None",
        draw() {}
      },
      {
        id: "halo",
        label: "Halo",
        draw(ctx, point) {
          ctx.strokeStyle = "rgba(226,232,240,0.45)";
          ctx.beginPath();
          ctx.arc(point.x, point.y - 14, 14, 0, Math.PI * 2);
          ctx.stroke();
        }
      }
    ],
    rightHand: [
      {
        id: "none",
        label: "None",
        draw() {}
      },
      {
        id: "ether_card",
        label: "Ether Card",
        draw(ctx, point, state) {
          const hasGlitch = state.activeCards.includes("Glitch");
          ctx.fillStyle = hasGlitch ? "#60a5fa" : "#a78bfa";
          ctx.fillRect(point.x - 8, point.y - 14, 16, 24);
        }
      },
      {
        id: "ether_blade",
        label: "Ether Blade",
        draw(ctx, point) {
          ctx.fillStyle = "#cbd5e1";
          ctx.fillRect(point.x - 2, point.y - 30, 4, 32);
          ctx.fillStyle = "#93c5fd";
          ctx.fillRect(point.x - 6, point.y - 34, 12, 8);
        }
      }
    ],
    chest: [
      {
        id: "none",
        label: "None",
        draw() {}
      },
      {
        id: "chest_orb",
        label: "Chest Orb",
        draw(ctx, point) {
          ctx.fillStyle = "rgba(147, 197, 253, 0.85)";
          ctx.beginPath();
          ctx.arc(point.x, point.y, 5, 0, Math.PI * 2);
          ctx.fill();
        }
      }
    ],
    back: [
      {
        id: "none",
        label: "None",
        draw() {}
      },
      {
        id: "back_core",
        label: "Back Core",
        draw(ctx, point) {
          ctx.fillStyle = "rgba(148, 163, 184, 0.55)";
          ctx.fillRect(point.x - 3, point.y - 3, 6, 6);
        }
      },
      {
        id: "back_pack",
        label: "Back Pack",
        draw(ctx, point) {
          ctx.fillStyle = "rgba(71, 85, 105, 0.9)";
          ctx.fillRect(point.x - 8, point.y - 10, 16, 20);
        }
      }
    ]
  };

  function createAttachmentRuntime() {
    return {
      equipped: {
        head: "halo",
        rightHand: "ether_card",
        chest: "chest_orb",
        back: "back_core"
      }
    };
  }

  function equipAttachment(runtime, socket, itemId) {
    if (!attachmentIndex[socket]) return;
    const exists = attachmentIndex[socket].some((item) => item.id === itemId);
    if (!exists) return;
    runtime.equipped[socket] = itemId;
  }

  function drawAttachments(ctx, sockets, state, runtime) {
    ctx.save();

    Object.entries(runtime.equipped).forEach(([socket, itemId]) => {
      const point = sockets[socket];
      if (!point) return;
      const item = attachmentIndex[socket].find((entry) => entry.id === itemId);
      if (!item) return;
      item.draw(ctx, point, state);
    });

    ctx.restore();
  }

  global.SpiralAttachments = {
    attachmentIndex,
    createAttachmentRuntime,
    equipAttachment,
    drawAttachments
  };
})(window);

/* ===== input.js ===== */
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

/* ===== fx.js ===== */
(function initFx(global) {
  class FxLayer {
    constructor() {
      this.time = 0;
      this.particles = [];
    }

    update(dt, state) {
      this.time += dt;

      if (state.activeCards.includes("Glitch") && Math.random() < 0.35) {
        this.particles.push({
          x: Math.random() * state.canvas.width,
          y: Math.random() * state.canvas.height,
          life: 0.22,
          color: "rgba(96, 165, 250, 0.45)"
        });
      }

      if (state.equipped.rightHand === "ether_blade" && Math.random() < 0.25 && state.sockets.rightHand) {
        this.particles.push({
          x: state.sockets.rightHand.x,
          y: state.sockets.rightHand.y,
          life: 0.18,
          color: "rgba(186, 230, 253, 0.7)"
        });
      }

      this.particles = this.particles
        .map((p) => ({ ...p, y: p.y - dt * 26, life: p.life - dt }))
        .filter((p) => p.life > 0);
    }

    draw(ctx, state) {
      const { canvas, activeCards, equipped, sockets } = state;

      if (activeCards.includes("Glitch")) {
        for (let i = 0; i < 6; i++) {
          const y = Math.random() * canvas.height;
          const h = Math.random() * 8 + 2;
          ctx.fillStyle = `rgba(96, 165, 250, ${Math.random() * 0.08})`;
          ctx.fillRect(0, y, canvas.width, h);
        }
      }

      if (equipped.head === "halo" && sockets.head) {
        ctx.strokeStyle = `rgba(147, 197, 253, ${0.25 + Math.sin(this.time * 4) * 0.15})`;
        ctx.beginPath();
        ctx.arc(sockets.head.x, sockets.head.y - 14, 18 + Math.sin(this.time * 5) * 2, 0, Math.PI * 2);
        ctx.stroke();
      }

      if (equipped.chest === "chest_orb" && sockets.chest) {
        const glow = 6 + Math.sin(this.time * 6) * 2;
        ctx.fillStyle = "rgba(125, 211, 252, 0.35)";
        ctx.beginPath();
        ctx.arc(sockets.chest.x, sockets.chest.y, glow, 0, Math.PI * 2);
        ctx.fill();
      }

      this.particles.forEach((particle) => {
        ctx.fillStyle = particle.color;
        ctx.beginPath();
        ctx.arc(particle.x, particle.y, 2.2, 0, Math.PI * 2);
        ctx.fill();
      });
    }
  }

  global.SpiralFx = { FxLayer };
})(window);

/* ===== main.js ===== */
(function bootSpiralApp(global) {
  const { Engine } = global.SpiralEngine;
  const { Skeleton } = global.SpiralSkeleton;
  const { cards, createCardRuntime, activateCard, tickCards, applyCards } = global.SpiralCards;
  const { Hud } = global.SpiralHud;
  const { attachmentIndex, createAttachmentRuntime, equipAttachment, drawAttachments } = global.SpiralAttachments;
  const { InputManager } = global.SpiralInput;
  const { FxLayer } = global.SpiralFx;

  const canvas = document.getElementById("viewport");
  const ctx = canvas.getContext("2d");
  const hudNode = document.getElementById("hud");

  let elapsed = 0;
  let pulse = 0;

  const skeleton = new Skeleton(canvas);
  const cardRuntime = createCardRuntime();
  const attachmentRuntime = createAttachmentRuntime();
  const fxLayer = new FxLayer();

  function toggleCard(name) {
    const idx = cardRuntime.activeCards.findIndex((entry) => entry.name === name);
    if (idx >= 0) {
      cardRuntime.activeCards.splice(idx, 1);
      return;
    }
    activateCard(cardRuntime, name);
  }

  const hud = new Hud(
    hudNode,
    cards,
    attachmentIndex,
    (cardName) => {
      if (cardName === cardRuntime.baseCard || cardName === "Idle") {
        cardRuntime.activeCards = [];
        return;
      }
      activateCard(cardRuntime, cardName);
    },
    () => {
      pulse = 1;
    },
    (socket, itemId) => {
      equipAttachment(attachmentRuntime, socket, itemId);
    }
  );

  const input = new InputManager({
    onPulse: () => {
      pulse = 1;
    },
    onSetIdle: () => {
      cardRuntime.activeCards = [];
    },
    onToggleStrike: () => {
      toggleCard("Strike");
    },
    onToggleGlitch: () => {
      toggleCard("Glitch");
    }
  });
  input.bind();

  function drawGrid() {
    ctx.save();
    ctx.strokeStyle = "rgba(148, 163, 184, 0.08)";
    ctx.lineWidth = 1;
    for (let x = 0; x < canvas.width; x += 32) {
      ctx.beginPath();
      ctx.moveTo(x, 0);
      ctx.lineTo(x, canvas.height);
      ctx.stroke();
    }
    for (let y = 0; y < canvas.height; y += 32) {
      ctx.beginPath();
      ctx.moveTo(0, y);
      ctx.lineTo(canvas.width, y);
      ctx.stroke();
    }
    ctx.restore();
  }

  function drawFooter(activeNames) {
    ctx.save();
    ctx.fillStyle = "rgba(226,232,240,0.75)";
    ctx.font = "12px Inter, system-ui, sans-serif";
    const active = activeNames.length ? activeNames.join(" + ") : "None";
    const equipped = Object.entries(attachmentRuntime.equipped)
      .map(([socket, item]) => `${socket}:${item}`)
      .join(" | ");
    ctx.fillText(`BASE: ${cardRuntime.baseCard} | ACTIVE: ${active}`, 18, canvas.height - 34);
    ctx.fillText(`EQUIPPED: ${equipped}`, 18, canvas.height - 18);
    ctx.restore();
  }

  function update(dt) {
    elapsed += dt;
    pulse *= 0.94;

    const pose = skeleton.getNeutralPose();
    tickCards(cardRuntime, dt);
    applyCards(cardRuntime, pose);
    skeleton.pose = pose;
    skeleton.updateSockets();

    const activeNames = cardRuntime.activeCards.map((entry) => entry.name);

    fxLayer.update(dt, {
      canvas,
      sockets: skeleton.sockets,
      activeCards: activeNames,
      equipped: attachmentRuntime.equipped
    });

    hud.updateRuntime(elapsed, [cardRuntime.baseCard, ...activeNames]);
  }

  function render() {
    const activeNames = cardRuntime.activeCards.map((entry) => entry.name);

    ctx.clearRect(0, 0, canvas.width, canvas.height);
    drawGrid();
    skeleton.draw(ctx, pulse, [cardRuntime.baseCard, ...activeNames]);
    drawAttachments(ctx, skeleton.sockets, { activeCards: activeNames }, attachmentRuntime);
    fxLayer.draw(ctx, {
      canvas,
      sockets: skeleton.sockets,
      activeCards: activeNames,
      equipped: attachmentRuntime.equipped
    });
    drawFooter(activeNames);
  }

  const engine = new Engine(update, render);
  engine.start();
})(window);

