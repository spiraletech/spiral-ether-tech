# Spiral OS Mainframe Report

This report includes the **single-file monolith runtime** for mainframe intake.

## Monolith artifact

- `spiral_monolith.js` — combined version of:
  - `engine.js`
  - `skeleton.js`
  - `cardLibrary.js`
  - `cards.js`
  - `hud.js`
  - `attachments.js`
  - `input.js`
  - `fx.js`
  - `main.js`

## Purpose

Use this file when you need one payload to review, archive, or share with Spiral OS GPT/mainframe pipelines.

The modular source remains the primary architecture; the monolith is the combined report build and is regenerated from modular files.
