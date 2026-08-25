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
