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
