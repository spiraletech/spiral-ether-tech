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
