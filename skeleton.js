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
