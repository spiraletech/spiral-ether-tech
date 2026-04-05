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
