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
