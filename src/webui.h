// Upstream SmartClock firmware by B. van Weerd, with SugarCube changes.
// Copyright (c) 2026 B. van Weerd. Upstream code in this file is MIT. See LICENSE.
// Copyright (c) 2026 Peter Štieber. SugarCube changes are PolyForm Noncommercial 1.0.0. See LICENSE-SUGARCUBE.
// Required Notice: Copyright (c) 2026 Peter Štieber. SugarCube changes may not be sold or used commercially.

#ifndef WEBUI_H
#define WEBUI_H

#include <Arduino.h>
#include <pgmspace.h>

const char index_html[] PROGMEM = R"rawliteral(

<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>SugarCube</title>
<script>
(() => {
    try {
        const savedTheme = localStorage.getItem("smartclock-ui-theme");
        const prefersDark = window.matchMedia && window.matchMedia("(prefers-color-scheme: dark)").matches;
        document.documentElement.dataset.uiTheme =
            savedTheme === "dark" || savedTheme === "light"
                ? savedTheme
                : (prefersDark ? "dark" : "light");
    } catch (error) {
        document.documentElement.dataset.uiTheme = "light";
    }
})();
</script>
<style>
:root {
    --bg: #eef2f9;
    --bg-top: #f8f9ff;
    --bg-glow: rgba(208, 226, 255, 0.95);
    --surface: #f8f9ff;
    --surface-dim: #eef2f8;
    --surface-high: rgba(245, 247, 252, 0.94);
    --surface-highest: #e7ebf3;
    --surface-tint: rgba(255, 255, 255, 0.72);
    --panel: rgba(248, 249, 255, 0.92);
    --panel-strong: #f2f5fb;
    --panel-layer: rgba(248, 249, 255, 0.92);
    --panel-border: rgba(108, 117, 135, 0.18);
    --chip-bg: #e8edf7;
    --line: #c4c9d4;
    --line-strong: #a4acba;
    --ink: #151b26;
    --muted: #566070;
    --accent: #0b57d0;
    --accent-strong: #0b57d0;
    --accent-soft: #d3e3fd;
    --accent-ring: rgba(11, 87, 208, 0.14);
    --accent-border: rgba(11, 87, 208, 0.18);
    --danger: #ba1a1a;
    --danger-soft: #ffdad6;
    --danger-border: rgba(186, 26, 26, 0.18);
    --text-on-accent: #ffffff;
    --shadow: 0 1px 2px rgba(18, 24, 33, 0.06), 0 18px 36px rgba(18, 24, 33, 0.08);
    --radius: 28px;
    --radius-m: 20px;
    --radius-s: 16px;
    --touch: 48px;
    --nav-width: 120px;
    --mono: "Roboto Mono", "IBM Plex Mono", "SFMono-Regular", "Menlo", monospace;
    --sans: "Roboto Flex", "Roboto", "Segoe UI Variable", "Noto Sans", sans-serif;
}
html {
    color-scheme: light;
}
html[data-ui-theme="dark"] {
    color-scheme: dark;
    --bg: #10131a;
    --bg-top: #181c25;
    --bg-glow: rgba(82, 148, 255, 0.12);
    --surface: #11131a;
    --surface-dim: #161922;
    --surface-high: rgba(31, 36, 47, 0.96);
    --surface-highest: #272d39;
    --surface-tint: rgba(22, 25, 34, 0.78);
    --panel: rgba(23, 27, 35, 0.94);
    --panel-strong: #20252f;
    --panel-layer: rgba(23, 27, 35, 0.94);
    --panel-border: rgba(195, 198, 208, 0.12);
    --chip-bg: #252b37;
    --line: #424956;
    --line-strong: #5b6474;
    --ink: #edf1fb;
    --muted: #adb6c7;
    --accent: #a8c7fa;
    --accent-strong: #d3e3fd;
    --accent-soft: #284777;
    --accent-ring: rgba(168, 199, 250, 0.18);
    --accent-border: rgba(168, 199, 250, 0.18);
    --danger: #ffb4ab;
    --danger-soft: #5c1d1d;
    --danger-border: rgba(255, 180, 171, 0.18);
    --shadow: 0 1px 2px rgba(0, 0, 0, 0.28), 0 20px 44px rgba(0, 0, 0, 0.36);
}
* {
    box-sizing: border-box;
}
html, body {
    margin: 0;
    min-height: 100%;
}
body {
    font-family: var(--sans);
    color: var(--ink);
    background:
        radial-gradient(circle at top left, var(--bg-glow), transparent 30%),
        radial-gradient(circle at bottom right, rgba(255, 255, 255, 0.22), transparent 24%),
        linear-gradient(180deg, var(--bg-top) 0%, var(--bg) 100%);
}
body.auth-open {
    overflow: hidden;
}
button,
select,
input,
textarea {
    font: inherit;
}
.app {
    max-width: 1480px;
    margin: 0 auto;
    padding: 24px 20px 36px;
}
.topbar {
    display: flex;
    align-items: center;
    justify-content: space-between;
    gap: 18px;
    position: sticky;
    top: 10px;
    z-index: 12;
    margin-bottom: 22px;
    padding: 14px 18px;
    border-radius: 30px;
    border: 1px solid var(--panel-border);
    background: var(--surface-tint);
    box-shadow: var(--shadow);
    backdrop-filter: blur(18px);
}
.brand {
    display: flex;
    align-items: center;
    gap: 16px;
    min-width: 0;
}
.brand-mark {
    flex-shrink: 0;
    width: 44px;
    height: 44px;
    border-radius: 16px;
    display: grid;
    place-items: center;
    background: var(--accent-soft);
    color: var(--accent-strong);
    box-shadow: inset 0 0 0 1px var(--accent-border);
}
.brand-mark svg {
    width: 24px;
    height: 24px;
    display: block;
}
.brand h1 {
    margin: 0;
    font-size: 1.28rem;
    font-weight: 600;
    letter-spacing: -0.03em;
}
.brand > div {
    min-width: 0;
}
.brand small {
    display: block;
    color: var(--muted);
    font-size: 0.88rem;
    margin-top: 2px;
    white-space: nowrap;
    overflow: hidden;
    text-overflow: ellipsis;
}
.top-actions {
    display: flex;
    align-items: center;
    justify-content: flex-end;
    gap: 10px;
    flex-wrap: wrap;
}
.auth-overlay {
    position: fixed;
    inset: 0;
    z-index: 20;
    display: none;
    align-items: center;
    justify-content: center;
    padding: 20px;
    background: rgba(13, 16, 24, 0.46);
    backdrop-filter: blur(18px);
}
.auth-overlay.active {
    display: flex;
}
.auth-card {
    width: min(100%, 420px);
    padding: 24px;
    border-radius: 32px;
    border: 1px solid var(--panel-border);
    background: var(--surface);
    box-shadow: var(--shadow);
}
.auth-card h2 {
    margin: 0;
    font-size: 1.4rem;
    letter-spacing: -0.03em;
}
.auth-card p {
    margin: 12px 0 0;
    color: var(--muted);
    font-size: 0.92rem;
    line-height: 1.55;
}
.auth-device {
    display: grid;
    gap: 6px;
    margin-bottom: 18px;
    padding: 16px 18px;
    border-radius: 24px;
    border: 1px solid var(--panel-border);
    background: var(--panel-strong);
}
.auth-device-label {
    color: var(--muted);
    font-size: 0.72rem;
    font-weight: 600;
    letter-spacing: 0.08em;
    text-transform: uppercase;
}
.auth-device-name {
    color: var(--ink);
    font-size: 1.05rem;
    font-weight: 600;
    letter-spacing: -0.02em;
}
.auth-device-host {
    color: var(--muted);
    font-family: var(--mono);
    font-size: 0.78rem;
    word-break: break-all;
}
.auth-user {
    display: inline-flex;
    align-items: center;
    min-height: 40px;
    margin-top: 16px;
    padding: 0 14px;
    border-radius: 999px;
    border: 1px solid var(--panel-border);
    background: var(--chip-bg);
    color: var(--ink);
    font-family: var(--mono);
    font-size: 0.82rem;
}
.auth-form {
    display: grid;
    gap: 14px;
    margin-top: 20px;
}
.auth-message {
    min-height: 1.2em;
    color: var(--muted);
    font-size: 0.85rem;
}
.auth-message.error {
    color: var(--danger);
}
.auth-support {
    display: grid;
    gap: 12px;
    margin-top: 18px;
}
.auth-note {
    padding: 14px 16px;
    border-radius: 22px;
    background: var(--panel-strong);
    color: var(--muted);
    font-size: 0.82rem;
    line-height: 1.5;
}
.auth-note strong {
    display: block;
    margin-bottom: 4px;
    color: var(--ink);
    font-size: 0.84rem;
}
.auth-note code {
    font-family: var(--mono);
}
.pill {
    display: inline-flex;
    align-items: center;
    justify-content: center;
    min-height: 38px;
    padding: 0 14px;
    border-radius: 999px;
    border: 1px solid transparent;
    background: var(--chip-bg);
    color: var(--ink);
    font-size: 0.84rem;
    font-weight: 600;
}
.pill.live,
.pill.saved {
    background: var(--accent-soft);
    color: var(--accent-strong);
}
.pill.pending {
    background: var(--surface-highest);
    color: var(--ink);
}
.pill.error {
    color: var(--danger);
    background: var(--danger-soft);
}
.shell {
    display: grid;
    grid-template-columns: var(--nav-width) minmax(0, 1fr);
    gap: 22px;
    align-items: start;
}
.workspace {
    display: grid;
    gap: 20px;
    min-width: 0;
}
.tabs {
    display: grid;
    gap: 10px;
    position: sticky;
    top: 112px;
    padding: 12px 8px;
    border-radius: 32px;
    border: 1px solid var(--panel-border);
    background: var(--panel);
    box-shadow: var(--shadow);
}
.tab {
    min-height: 76px;
    padding: 12px 8px;
    border: none;
    border-radius: 24px;
    background: transparent;
    color: var(--muted);
    cursor: pointer;
    font-weight: 600;
    display: grid;
    justify-items: center;
    align-content: center;
    gap: 8px;
    box-shadow: none;
}
.tab-icon {
    width: 36px;
    height: 36px;
    border-radius: 18px;
    display: grid;
    place-items: center;
    color: currentColor;
}
.tab-icon svg {
    width: 20px;
    height: 20px;
    stroke: currentColor;
    fill: none;
    stroke-width: 2;
    stroke-linecap: round;
    stroke-linejoin: round;
}
.tab-label {
    font-size: 0.76rem;
    letter-spacing: 0.01em;
    text-align: center;
    line-height: 1.15;
}
.tab.active {
    background: var(--chip-bg);
    color: var(--ink);
}
.tab.active .tab-icon {
    background: var(--accent-soft);
    color: var(--accent-strong);
}
.panel {
    min-width: 0;
    background: var(--panel-layer);
    border: 1px solid var(--panel-border);
    border-radius: var(--radius);
    box-shadow: var(--shadow);
    padding: 24px;
}
.display-section {
    display: grid;
    gap: 14px;
    padding: 18px;
    border-radius: 24px;
    border: 1px solid var(--panel-border);
    background: var(--panel-strong);
}
.display-section + .display-section {
    margin-top: 16px;
}
.display-section-header {
    display: flex;
    align-items: flex-start;
    justify-content: space-between;
    gap: 12px;
}
.display-section-title {
    margin: 0;
    font-size: 0.98rem;
    font-weight: 600;
    letter-spacing: -0.02em;
}
.display-section-copy {
    margin: 6px 0 0;
    color: var(--muted);
    font-size: 0.84rem;
    line-height: 1.45;
}
.switch-list {
    display: flex;
    flex-direction: column;
    align-items: flex-start;
    gap: 2px;
}
.panel-stack,
.content-grid,
.system-grid {
    display: grid;
    gap: 20px;
}
.content-grid,
.system-grid {
    grid-template-columns: minmax(0, 1.08fr) minmax(300px, 0.92fr);
}
.panel-header {
    display: flex;
    align-items: center;
    justify-content: space-between;
    gap: 12px;
    margin-bottom: 18px;
}
.panel-title {
    margin: 0;
    font-size: 1.08rem;
    font-weight: 600;
    letter-spacing: -0.03em;
}
.meta-inline {
    display: flex;
    gap: 8px;
    flex-wrap: wrap;
}
.meta-chip {
    padding: 7px 12px;
    border-radius: 999px;
    background: var(--chip-bg);
    border: 1px solid transparent;
    color: var(--muted);
    font-size: 0.76rem;
    font-family: var(--mono);
}
.grid {
    display: grid;
    gap: 16px;
}
.grid.two {
    grid-template-columns: repeat(2, minmax(0, 1fr));
}
.grid.three {
    grid-template-columns: repeat(3, minmax(0, 1fr));
}
.field {
    display: grid;
    gap: 8px;
}
.field label,
.section-label,
summary {
    font-size: 0.76rem;
    font-weight: 600;
    text-transform: uppercase;
    letter-spacing: 0.05em;
    color: var(--muted);
}
.field small,
.muted {
    color: var(--muted);
    font-size: 0.8rem;
    line-height: 1.45;
}
input[type="text"],
input[type="number"],
input[type="password"],
input[type="time"],
select,
textarea {
    width: 100%;
    min-height: 56px;
    padding: 16px;
    border-radius: 18px;
    border: 1px solid transparent;
    background: var(--panel-strong);
    color: var(--ink);
    transition: border-color 140ms ease, box-shadow 140ms ease, background 140ms ease;
}
input[type="text"]:focus,
input[type="number"]:focus,
input[type="password"]:focus,
input[type="time"]:focus,
select:focus,
textarea:focus {
    outline: none;
    border-color: var(--accent-border);
    box-shadow: 0 0 0 4px var(--accent-ring);
    background: var(--surface);
}
textarea {
    min-height: 120px;
    resize: vertical;
}
input[type="range"] {
    width: 100%;
    accent-color: var(--accent);
}
.slider-row {
    display: grid;
    grid-template-columns: 1fr auto;
    align-items: center;
    gap: 10px;
}
.value-badge {
    min-width: 54px;
    text-align: center;
    padding: 12px 14px;
    border-radius: 16px;
    border: 1px solid transparent;
    background: var(--panel-strong);
    font-family: var(--mono);
}
.theme-picker {
    display: grid;
    gap: 12px;
    grid-template-columns: repeat(auto-fit, minmax(150px, 1fr));
}
.theme-button {
    min-height: 92px;
    padding: 14px 16px;
    border-radius: 20px;
    border: 1px solid transparent;
    background: var(--panel-strong);
    color: var(--ink);
    cursor: pointer;
    font-weight: 600;
    display: grid;
    align-content: start;
    justify-items: start;
    gap: 12px;
}
.theme-button.active {
    color: var(--accent-strong);
    background: var(--accent-soft);
}
.theme-preview {
    display: flex;
    gap: 8px;
}
.theme-swatch {
    width: 18px;
    height: 18px;
    border-radius: 9px;
    border: 1px solid rgba(0, 0, 0, 0.12);
}
.theme-button-title {
    font-size: 0.92rem;
    font-weight: 600;
}
.color-grid {
    display: grid;
    gap: 12px;
    grid-template-columns: repeat(auto-fit, minmax(220px, 1fr));
}
.color-card {
    padding: 14px;
    border-radius: 22px;
    background: var(--panel-strong);
}
.color-row {
    display: grid;
    grid-template-columns: 56px 1fr;
    gap: 10px;
    align-items: center;
}
input[type="color"] {
    width: 56px;
    min-width: 56px;
    height: 56px;
    padding: 6px;
    border-radius: 18px;
    border: 1px solid transparent;
    background: var(--surface);
    cursor: pointer;
}
.toggle-grid {
    display: grid;
    gap: 12px;
    grid-template-columns: repeat(2, minmax(0, 1fr));
}
.toggle {
    display: flex;
    align-items: center;
    justify-content: space-between;
    gap: 12px;
    min-height: 64px;
    padding: 14px 16px;
    border-radius: 22px;
    border: 1px solid transparent;
    background: var(--panel-strong);
}
.toggle span {
    font-size: 0.92rem;
    font-weight: 600;
}
.toggle input[type="checkbox"] {
    appearance: none;
    -webkit-appearance: none;
    position: relative;
    width: 52px;
    min-width: 52px;
    height: 32px;
    border-radius: 999px;
    background: var(--line);
    cursor: pointer;
    transition: background 140ms ease;
}
.toggle input[type="checkbox"]::before {
    content: "";
    position: absolute;
    top: 4px;
    left: 4px;
    width: 24px;
    height: 24px;
    border-radius: 50%;
    background: var(--surface);
    box-shadow: 0 1px 2px rgba(0, 0, 0, 0.24);
    transition: transform 140ms ease, background 140ms ease;
}
.toggle input[type="checkbox"]:checked {
    background: var(--accent);
}
.toggle input[type="checkbox"]:checked::before {
    transform: translateX(20px);
    background: var(--text-on-accent);
}
.toggle.switch-row {
    width: auto;
    justify-content: flex-start;
    gap: 14px;
    min-height: 44px;
    padding: 2px 0;
    background: transparent;
    border-color: transparent;
}
.chip-grid {
    display: flex;
    flex-wrap: wrap;
    gap: 10px;
}
.chip-toggle {
    position: relative;
}
.chip-toggle input {
    position: absolute;
    opacity: 0;
    pointer-events: none;
}
.chip-toggle span {
    display: inline-flex;
    align-items: center;
    justify-content: center;
    min-height: 44px;
    padding: 0 14px;
    border-radius: 999px;
    border: 1px solid transparent;
    background: var(--panel-strong);
    color: var(--muted);
    cursor: pointer;
    font-weight: 600;
}
.chip-toggle input:checked + span {
    color: var(--accent-strong);
    background: var(--accent-soft);
}
.chip-toggle input:focus-visible + span {
    outline: 2px solid var(--accent);
    outline-offset: 3px;
}
details {
    border: 1px solid transparent;
    border-radius: 22px;
    background: var(--panel-strong);
    overflow: hidden;
}
details + details {
    margin-top: 10px;
}
summary {
    list-style: none;
    padding: 16px 18px;
    cursor: pointer;
}
summary::-webkit-details-marker {
    display: none;
}
.details-body {
    padding: 0 18px 18px;
}
.feed-stack,
.search-results {
    display: grid;
    gap: 10px;
}
.slot-card {
    padding: 18px;
    border-radius: 24px;
    border: 1px solid transparent;
    background: var(--panel-strong);
}
.search-result {
    text-align: left;
    justify-content: flex-start;
    min-height: 0;
    padding: 14px 16px;
    border-radius: 18px;
    border: 1px solid transparent;
    background: var(--panel-strong);
    color: var(--ink);
}
.search-result small {
    display: block;
    margin-top: 4px;
    color: var(--muted);
    font-weight: 500;
}
.hidden {
    display: none !important;
}
.world-row {
    grid-template-columns: auto 1fr 1fr;
}
.world-row input[type="checkbox"] {
    appearance: none;
    -webkit-appearance: none;
    width: 20px;
    height: 20px;
    margin: 0 4px 18px 0;
    border-radius: 6px;
    border: 2px solid var(--line-strong);
    background: transparent;
    cursor: pointer;
}
.world-row input[type="checkbox"]:checked {
    border-color: var(--accent);
    background: var(--accent);
    box-shadow: inset 0 0 0 3px var(--surface);
}
.button-row {
    display: flex;
    flex-wrap: wrap;
    gap: 12px;
}
.button,
button,
input[type="submit"] {
    min-height: var(--touch);
    padding: 0 18px;
    border: 1px solid transparent;
    border-radius: 999px;
    background: var(--accent);
    color: var(--text-on-accent);
    cursor: pointer;
    font-weight: 600;
    letter-spacing: 0.01em;
}
.ghost,
.button.secondary {
    background: var(--panel-strong);
    color: var(--ink);
    border: 1px solid var(--panel-border);
}
.danger {
    background: var(--danger);
    color: var(--text-on-accent);
}
.mode-switch {
    display: inline-flex;
    align-items: center;
    gap: 4px;
    padding: 4px;
    border-radius: 999px;
    border: 1px solid var(--panel-border);
    background: var(--panel-strong);
}
.mode-button {
    min-height: 36px;
    padding: 0 14px;
    border: 1px solid transparent;
    border-radius: 999px;
    background: transparent;
    color: var(--muted);
    box-shadow: none;
}
.mode-button.active {
    background: var(--accent-soft);
    color: var(--ink);
}
.mode-button:hover {
    background: var(--surface-highest);
}
.button-link {
    display: inline-flex;
    align-items: center;
    justify-content: center;
    min-height: var(--touch);
    padding: 0 18px;
    border-radius: 999px;
    border: 1px solid var(--panel-border);
    background: var(--panel-strong);
    color: var(--ink);
    text-decoration: none;
    font-weight: 600;
}
button:disabled,
input[type="submit"]:disabled,
.button-link[aria-disabled="true"] {
    opacity: 0.5;
    cursor: not-allowed;
    box-shadow: none;
}
.panel-note {
    margin-top: 12px;
    color: var(--muted);
    font-size: 0.8rem;
    line-height: 1.45;
    padding: 12px 14px;
    border-radius: 18px;
    background: var(--panel-strong);
}
.identity-preview {
    margin-top: 0;
    display: grid;
    gap: 10px;
}
.identity-line {
    display: flex;
    align-items: baseline;
    justify-content: space-between;
    gap: 12px;
}
.identity-line span {
    color: var(--muted);
}
.identity-line strong,
.identity-line code {
    min-width: 0;
    overflow-wrap: anywhere;
    text-align: right;
    color: var(--ink);
    font-size: 0.86rem;
}
.identity-line code {
    font-family: var(--mono);
    text-align: right;
    word-break: break-all;
}
.status-list {
    display: grid;
    gap: 10px;
}
.status-item {
    display: flex;
    align-items: center;
    justify-content: space-between;
    gap: 12px;
    min-height: 52px;
    padding: 12px 14px;
    border-radius: 18px;
    border: 1px solid transparent;
    background: var(--panel-strong);
}
.status-item span:last-child {
    color: var(--muted);
    font-family: var(--mono);
    font-size: 0.8rem;
}
.tab-panel {
    display: none;
}
.tab-panel.active {
    display: grid;
    gap: 20px;
}
@media (max-width: 1120px) {
    .app {
        padding-bottom: 112px;
    }
    .topbar {
        position: static;
    }
    .shell {
        grid-template-columns: 1fr;
    }
    .tabs {
        position: fixed;
        left: 16px;
        right: 16px;
        bottom: 16px;
        top: auto;
        z-index: 15;
        grid-template-columns: repeat(6, minmax(0, 1fr));
        gap: 6px;
        padding: 10px;
        border-radius: 28px;
        backdrop-filter: blur(18px);
    }
    .tab {
        min-height: 68px;
        padding: 10px 6px;
        gap: 6px;
    }
    .tab-icon {
        width: 32px;
        height: 32px;
    }
    .tab-label {
        font-size: 0.68rem;
        line-height: 1.05;
    }
}
@media (max-width: 980px) {
    .content-grid,
    .system-grid,
    .grid.two,
    .grid.three,
    .color-grid,
    .toggle-grid {
        grid-template-columns: 1fr;
    }
    .topbar {
        flex-direction: column;
        align-items: flex-start;
    }
    .top-actions {
        width: 100%;
        justify-content: flex-start;
    }
}
@media (max-width: 720px) {
    .app {
        padding: 16px 14px 106px;
    }
    .topbar {
        margin-bottom: 16px;
        padding: 14px;
        border-radius: 24px;
    }
    .brand {
        width: 100%;
    }
    .brand small {
        max-width: 100%;
    }
    .panel {
        padding: 18px;
        border-radius: 24px;
    }
    .toggle-grid {
        grid-template-columns: 1fr;
    }
    .button-row > * {
        flex: 1 1 140px;
    }
}
</style>
</head>
<body>
<div class="app" id="appRoot" inert>
    <header class="topbar">
        <div class="brand">
            <div class="brand-mark" aria-hidden="true">
                <svg viewBox="0 0 24 24">
                    <path fill="currentColor" opacity="0.45" d="M12 3.2 20.2 8 12 12.8 3.8 8Z"></path>
                    <path fill="currentColor" opacity="0.72" d="M3.8 8v8L12 20.8V12.8Z"></path>
                    <path fill="currentColor" d="M20.2 8v8L12 20.8V12.8Z"></path>
                </svg>
            </div>
            <div>
                <h1>SugarCube</h1>
                <small id="deviceMeta">Loading…</small>
            </div>
        </div>
        <div class="top-actions">
            <div class="mode-switch" role="group" aria-label="Interface theme">
                <button class="mode-button" type="button" data-ui-theme-button="light" onclick="setUiTheme('light')">Light</button>
                <button class="mode-button" type="button" data-ui-theme-button="dark" onclick="setUiTheme('dark')">Dark</button>
            </div>
            <span id="syncState" class="pill" role="status">Ready</span>
            <button id="saveDisplayButton" type="button" onclick="commitState()">Save changes</button>
            <button id="discardDisplayButton" class="ghost" type="button" onclick="discardState()">Discard</button>
            <button class="ghost" type="button" onclick="loadAllState()">Refresh</button>
            <button id="signOutButton" class="ghost hidden" type="button" onclick="logoutSession()">Sign out</button>
        </div>
    </header>

    <div class="shell">
    <nav class="tabs" aria-label="Sections">
        <button class="tab active" data-tab="display" type="button" aria-pressed="true">
            <span class="tab-icon" aria-hidden="true">
                <svg viewBox="0 0 24 24">
                    <path d="M5 7h14"></path>
                    <path d="M5 12h8"></path>
                    <path d="M5 17h14"></path>
                    <circle cx="17" cy="12" r="2"></circle>
                </svg>
            </span>
            <span class="tab-label">Display</span>
        </button>
        <button class="tab" data-tab="nightscout" type="button" aria-pressed="false">
            <span class="tab-icon" aria-hidden="true">
                <svg viewBox="0 0 24 24">
                    <path d="M12 3.5c3 4 5.5 6.8 5.5 10a5.5 5.5 0 0 1-11 0c0-3.2 2.5-6 5.5-10z"></path>
                </svg>
            </span>
            <span class="tab-label">Nightscout</span>
        </button>
        <button class="tab" data-tab="network" type="button" aria-pressed="false">
            <span class="tab-icon" aria-hidden="true">
                <svg viewBox="0 0 24 24">
                    <path d="M4 10.5a12 12 0 0 1 16 0"></path>
                    <path d="M7.5 14a7.5 7.5 0 0 1 9 0"></path>
                    <path d="M10.5 17.5a3.7 3.7 0 0 1 3 0"></path>
                    <circle cx="12" cy="20" r="1"></circle>
                </svg>
            </span>
            <span class="tab-label">Network</span>
        </button>
        <button class="tab" data-tab="system" type="button" aria-pressed="false">
            <span class="tab-icon" aria-hidden="true">
                <svg viewBox="0 0 24 24">
                    <circle cx="12" cy="12" r="3.2"></circle>
                    <path d="M12 4.5v2.1"></path>
                    <path d="M12 17.4v2.1"></path>
                    <path d="M19.5 12h-2.1"></path>
                    <path d="M6.6 12H4.5"></path>
                    <path d="M17.3 6.7 15.8 8.2"></path>
                    <path d="M8.2 15.8 6.7 17.3"></path>
                    <path d="M17.3 17.3 15.8 15.8"></path>
                    <path d="M8.2 8.2 6.7 6.7"></path>
                </svg>
            </span>
            <span class="tab-label">System</span>
        </button>
    </nav>

    <main class="workspace">
    <section id="panel-display" class="tab-panel active">
        <div class="panel-stack">
            <section class="panel">
                <div class="panel-header">
                    <h2 class="panel-title">Display</h2>
                    <div class="meta-inline">
                        <span class="meta-chip" id="firmwareMeta">--</span>
                        <span class="meta-chip" id="storageMeta">--</span>
                        <span class="meta-chip" id="memoryMeta">--</span>
                    </div>
                </div>

                <section class="display-section">
                    <div class="display-section-header">
                        <div>
                            <h3 class="display-section-title">Clock</h3>
                            <p class="display-section-copy">Timezone and the time shown on the glucose screen.</p>
                        </div>
                    </div>

                    <div class="field">
                        <label for="timezonePreset">Timezone</label>
                        <select data-live id="timezonePreset"></select>
                        <div id="timezoneCustomWrap" class="field hidden">
                            <input data-live type="number" id="timezoneCustom" placeholder="Offset in seconds">
                        </div>
                    </div>

                    <div class="switch-list">
                        <label class="toggle switch-row"><span>24-hour</span><input data-live type="checkbox" id="use24Hour"></label>
                        <label class="toggle switch-row"><span>Seconds</span><input data-live type="checkbox" id="showSeconds"></label>
                        <label class="toggle switch-row"><span>Footer IP</span><input data-live type="checkbox" id="showIp"></label>
                    </div>
                </section>

                <section class="display-section">
                    <div class="display-section-header">
                        <div>
                            <h3 class="display-section-title">Brightness</h3>
                            <p class="display-section-copy">Daytime backlight level.</p>
                        </div>
                    </div>

                    <div class="field">
                        <label for="brightness">Brightness</label>
                        <div class="slider-row">
                            <input data-live type="range" id="brightness" min="0" max="100" value="70">
                            <div class="value-badge" id="brightnessValue">70%</div>
                        </div>
                    </div>
                </section>

                <section class="display-section">
                    <div class="display-section-header">
                        <div>
                            <h3 class="display-section-title">Night Mode</h3>
                            <p class="display-section-copy">Optional schedule for a dimmer backlight.</p>
                        </div>
                    </div>

                    <div class="switch-list">
                        <label class="toggle switch-row"><span>Enable schedule</span><input data-live type="checkbox" id="nightModeEnabled"></label>
                    </div>

                    <div id="nightModeFields" class="panel-stack">
                        <div class="grid two">
                            <div class="field">
                                <label for="nightStart">Start</label>
                                <input data-live type="time" id="nightStart" step="60">
                            </div>
                            <div class="field">
                                <label for="nightEnd">End</label>
                                <input data-live type="time" id="nightEnd" step="60">
                            </div>
                        </div>

                        <div class="field">
                            <label for="nightBrightness">Night brightness</label>
                            <div class="slider-row">
                                <input data-live type="range" id="nightBrightness" min="1" max="100" value="14">
                                <div class="value-badge" id="nightBrightnessValue">14%</div>
                            </div>
                        </div>

                        <div class="panel-note">Night mode follows the device timezone and uses its own brightness percentage.</div>
                    </div>
                </section>
            </section>
        </div>
    </section>

    <section id="panel-nightscout" class="tab-panel">
        <div class="panel-stack">
            <section class="panel">
                <div class="panel-header">
                    <h2 class="panel-title">Nightscout</h2>
                    <div class="meta-inline">
                        <span class="meta-chip" id="nightscoutFeedBadge">--</span>
                        <span class="meta-chip" id="nightscoutReading">--</span>
                        <span class="meta-chip" id="nightscoutFeedStatus">--</span>
                    </div>
                </div>

                <div class="toggle-grid">
                    <label class="toggle"><span>Enable feed</span><input data-feed-live type="checkbox" id="nightscoutEnabled"></label>
                </div>

                <div id="nightscoutFields" class="panel-stack" style="margin-top: 14px;">
                    <div class="field">
                        <label for="nightscoutBaseUrl">Site URL</label>
                        <input data-feed-live type="text" id="nightscoutBaseUrl" placeholder="https://my-nightscout.example.com">
                    </div>
                    <div class="field">
                        <label for="nightscoutRefreshSeconds">Read every (seconds)</label>
                        <input data-feed-live type="number" id="nightscoutRefreshSeconds" min="15" max="3600" step="1" inputmode="numeric">
                        <small>How often the cube asks Nightscout. 15 to 3600 seconds. 30 is the usual pace.</small>
                    </div>

                    <div class="field">
                        <label for="nightscoutToken">Access token (optional)</label>
                        <input data-feed-live type="password" id="nightscoutToken" placeholder="e.g. display-1a2b3c4d5e6f7a8b">
                        <small>Needed only if your site is not public. Create a read-only subject in Nightscout Admin Tools and paste its access token here. Do not use your API secret.</small>
                    </div>

                    <div class="toggle-grid">
                        <label class="toggle"><span>Show mmol/L (off = mg/dL)</span><input data-feed-live type="checkbox" id="nightscoutUseMmol"></label>
                        <label class="toggle"><span>Show insulin on board (IOB)</span><input data-feed-live type="checkbox" id="nightscoutShowIob"></label>
                    </div>
                    <div class="panel-note">Urgent low, low, high, and urgent high follow the unit selected above.</div>

                    <div class="grid two">
                        <div class="field">
                            <label for="nightscoutUrgentLow">Urgent low (mg/dL)</label>
                            <input data-feed-live type="number" id="nightscoutUrgentLow" min="30" max="120">
                        </div>
                        <div class="field">
                            <label for="nightscoutLow">Low (mg/dL)</label>
                            <input data-feed-live type="number" id="nightscoutLow" min="40" max="200">
                        </div>
                    </div>
                    <div class="grid two">
                        <div class="field">
                            <label for="nightscoutHigh">High (mg/dL)</label>
                            <input data-feed-live type="number" id="nightscoutHigh" min="100" max="400">
                        </div>
                        <div class="field">
                            <label for="nightscoutUrgentHigh">Urgent high (mg/dL)</label>
                            <input data-feed-live type="number" id="nightscoutUrgentHigh" min="120" max="600">
                        </div>
                    </div>
                    <small>Colours on the display: green inside Low-High, orange outside, red beyond the urgent limits. For information only, not for treatment decisions.</small>
                </div>

                <div class="button-row" style="margin-top: 16px;">
                    <button id="nightscoutSyncButton" type="button" onclick="syncFeeds('nightscout')">Sync Nightscout</button>
                    <button id="saveNightscoutButton" type="button" class="ghost" onclick="commitFeedConfig()">Save feeds</button>
                    <button id="discardNightscoutButton" type="button" class="ghost" onclick="discardFeedConfig()">Discard</button>
                </div>
            </section>
        </div>
    </section>

    <section id="panel-network" class="tab-panel">
        <div class="panel">
            <div class="panel-header">
                <h2 class="panel-title">Network</h2>
            </div>
            <div class="grid two" style="margin-bottom: 14px;">
                <div class="field">
                    <label for="deviceName">Device name</label>
                    <input data-live type="text" id="deviceName" maxlength="31" placeholder="Desk clock">
                    <small>Shown on the login page and used for <code>.local</code> discovery after the next reconnect or reboot.</small>
                </div>
                <div class="panel-note identity-preview">
                    <div class="identity-line">
                        <span>Login page</span>
                        <strong id="deviceNamePreview">Loading…</strong>
                    </div>
                    <div class="identity-line">
                        <span>.local</span>
                        <code id="deviceHostPreview">Loading…</code>
                    </div>
                </div>
            </div>
            <div class="button-row">
                <button type="button" class="ghost" onclick="scanWiFi()">Scan</button>
                <button type="button" class="ghost" onclick="reconfigureWiFi()">Portal</button>
            </div>
            <div class="panel-note" id="scanStatus">Idle</div>
            <div class="grid two" style="margin-top: 14px;">
                <div class="field">
                    <label for="wifiNetwork">Network</label>
                    <select id="wifiNetwork"></select>
                </div>
                <div class="field">
                    <label for="wifiPassword">Password</label>
                    <input type="password" id="wifiPassword">
                </div>
            </div>
            <div class="button-row" style="margin-top: 14px;">
                <button type="button" onclick="connectWiFi()">Connect</button>
            </div>
        </div>
    </section>

    <section id="panel-system" class="tab-panel">
        <div class="system-grid">


            <section class="panel">
                <div class="panel-header">
                    <h2 class="panel-title">System</h2>
                </div>
                <div class="status-list">
                    <div class="status-item"><span>Firmware</span><span id="systemVersion">--</span></div>
                    <div class="status-item"><span>Storage</span><span id="systemStorage">--</span></div>
                    <div class="status-item"><span>RAM</span><span id="systemMemory">--</span></div>
                </div>
                <div class="button-row" style="margin-top: 14px;">
                    <button type="button" class="ghost" onclick="restartDevice()">Restart</button>
                    <a class="button-link" href="/update">Update</a>
                    <a class="button-link" href="/log">Logs</a>
                </div>
                <div class="field" style="margin-top: 18px;">
                    <label for="currentAdminPassword">Admin password</label>
                    <small>Protects the dashboard and OTA access.</small>
                </div>
                <div class="grid two" style="margin-top: 12px;">
                    <div class="field">
                        <label for="currentAdminPassword">Current</label>
                        <input type="password" id="currentAdminPassword" autocomplete="current-password">
                    </div>
                    <div class="field">
                        <label for="newAdminPassword">New</label>
                        <input type="password" id="newAdminPassword" autocomplete="new-password">
                    </div>
                </div>
                <div class="field" style="margin-top: 12px;">
                    <label for="confirmAdminPassword">Confirm</label>
                    <input type="password" id="confirmAdminPassword" autocomplete="new-password">
                </div>
                <div class="button-row" style="margin-top: 14px;">
                    <button type="button" class="ghost" onclick="changeAdminPassword()">Update password</button>
                </div>
                <div class="panel-note" id="securityStatus">Signed in as <code>admin</code>.</div>
                <div class="button-row" style="margin-top: 14px;">
                    <button type="button" class="danger" onclick="factoryReset()">Factory Reset</button>
                </div>
            </section>
        </div>
    </section>
    </main>
    </div>
</div>

<div id="authOverlay" class="auth-overlay active" role="dialog" aria-modal="true" aria-labelledby="authTitle">
    <div class="auth-card">
        <div class="auth-device">
            <div class="auth-device-label">Device</div>
            <div class="auth-device-name" id="authDeviceName">Loading…</div>
            <div class="auth-device-host" id="authDeviceHost">Loading…</div>
        </div>
        <h2 id="authTitle">Admin login</h2>
        <p id="authHint">Sign in to manage the device.</p>
        <div class="auth-user">User: <span id="authUsernameLabel" style="margin-left: 6px;">admin</span></div>
        <form id="authForm" class="auth-form">
            <div class="field">
                <label for="authPassword">Password</label>
                <input type="password" id="authPassword" autocomplete="current-password" autofocus>
            </div>
            <div class="button-row">
                <button type="submit">Sign in</button>
                <button id="authRevealButton" type="button" class="ghost hidden" onclick="revealPasswordOnDevice()">Show password on device</button>
            </div>
            <div id="authMessage" class="auth-message" role="alert"></div>
        </form>
        <div class="auth-support">
            <div class="auth-note" id="authSetupNote"></div>
            <div class="auth-note" id="authRecoveryNote"></div>
        </div>
    </div>
</div>

<script>
const timezoneOptions = [
    { value: -43200, label: "UTC-12:00" },
    { value: -39600, label: "UTC-11:00" },
    { value: -36000, label: "UTC-10:00" },
    { value: -34200, label: "UTC-09:30" },
    { value: -32400, label: "UTC-09:00" },
    { value: -28800, label: "UTC-08:00" },
    { value: -25200, label: "UTC-07:00" },
    { value: -21600, label: "UTC-06:00" },
    { value: -18000, label: "UTC-05:00" },
    { value: -14400, label: "UTC-04:00" },
    { value: -12600, label: "UTC-03:30" },
    { value: -10800, label: "UTC-03:00" },
    { value: -7200, label: "UTC-02:00" },
    { value: -3600, label: "UTC-01:00" },
    { value: 0, label: "UTC+00:00" },
    { value: 3600, label: "UTC+01:00" },
    { value: 7200, label: "UTC+02:00" },
    { value: 10800, label: "UTC+03:00" },
    { value: 12600, label: "UTC+03:30" },
    { value: 14400, label: "UTC+04:00" },
    { value: 16200, label: "UTC+04:30" },
    { value: 18000, label: "UTC+05:00" },
    { value: 19800, label: "UTC+05:30" },
    { value: 20700, label: "UTC+05:45" },
    { value: 21600, label: "UTC+06:00" },
    { value: 23400, label: "UTC+06:30" },
    { value: 25200, label: "UTC+07:00" },
    { value: 28800, label: "UTC+08:00" },
    { value: 31500, label: "UTC+08:45" },
    { value: 32400, label: "UTC+09:00" },
    { value: 34200, label: "UTC+09:30" },
    { value: 36000, label: "UTC+10:00" },
    { value: 37800, label: "UTC+10:30" },
    { value: 39600, label: "UTC+11:00" },
    { value: 43200, label: "UTC+12:00" },
    { value: 45900, label: "UTC+12:45" },
    { value: 46800, label: "UTC+13:00" },
    { value: 50400, label: "UTC+14:00" }
];

const themeLabels = /*DEVICE_THEME_PRESETS*/ [];

const defaultCustomTheme = {
    background: themeLabels[0].tones[0],
    surface: themeLabels[0].tones[1],
    accent: themeLabels[0].tones[2],
    text: themeLabels[0].text
};

const defaultNightCustomTheme = {
    background: "#050B17",
    surface: "#101A28",
    accent: "#A8C7FA",
    text: "#F3F6FC"
};

const dashboardPageControls = [
    { key: "clock", id: "pageClock" },
    { key: "nightscout", id: "pageNightscout" },
    { key: "weather", id: "pageWeather" }
];

let hydrating = false;
let dirty = false;
let editGeneration = 0;
let feedEditGeneration = 0;
let operationQueue = Promise.resolve();

function queueOperation(operation) {
    const pending = operationQueue.then(operation);
    operationQueue = pending.catch(() => {});
    return pending;
}
let liveTimer = 0;
let livePromise = Promise.resolve();
let feedDirty = false;
let feedLiveTimer = 0;
let feedPromise = Promise.resolve();
let authenticated = false;
let authProvisionedThisBoot = false;
let authCanRevealPassword = false;
const uiThemeStorageKey = "smartclock-ui-theme";
const deviceIdentity = {
    deviceName: "SugarCube",
    defaultDeviceName: "SugarCube",
    hostName: "sugarcube"
};

function currentUiTheme() {
    return document.documentElement.dataset.uiTheme === "dark" ? "dark" : "light";
}

function refreshUiThemeButtons() {
    const activeTheme = currentUiTheme();
    document.querySelectorAll("[data-ui-theme-button]").forEach((button) => {
        const isActive = button.dataset.uiThemeButton === activeTheme;
        button.classList.toggle("active", isActive);
        button.setAttribute("aria-pressed", isActive ? "true" : "false");
    });
}

function setUiTheme(theme) {
    const nextTheme = theme === "dark" ? "dark" : "light";
    document.documentElement.dataset.uiTheme = nextTheme;
    refreshUiThemeButtons();

    try {
        localStorage.setItem(uiThemeStorageKey, nextTheme);
    } catch (error) {
        console.warn("Theme preference was not persisted.", error);
    }
}

function normalizeDeviceName(value) {
    return String(value ?? "")
        .replace(/\./g, " ")
        .replace(/[^A-Za-z0-9 _-]+/g, "")
        .replace(/\s+/g, " ")
        .trim()
        .slice(0, 31);
}

function defaultDeviceNameLabel() {
    return normalizeDeviceName(deviceIdentity.defaultDeviceName) || "SugarCube";
}

function buildHostnameFromDeviceName(value) {
    const effectiveName = normalizeDeviceName(value) || defaultDeviceNameLabel();
    const slug = effectiveName
        .toLowerCase()
        .replace(/[^a-z0-9 _-]+/g, "")
        .replace(/[\s_]+/g, "-")
        .replace(/-+/g, "-")
        .replace(/^-+|-+$/g, "");

    let host = slug.startsWith("smartclock-") ? slug.slice("smartclock-".length) : slug;
    if (host === "smartclock") host = "";
    if (!host || host === "sugarcube") {
        return "sugarcube";
    }
    return host.slice(0, 31);
}

function updateDeviceMeta() {
    const deviceName = normalizeDeviceName(deviceIdentity.deviceName) || defaultDeviceNameLabel();
    const hostName = String(deviceIdentity.hostName || buildHostnameFromDeviceName(deviceName));
    const hostLabel = `${hostName}.local`;

    document.getElementById("deviceMeta").textContent = `${deviceName} · ${hostLabel}${deviceIdentity.rebootRequired ? " · Restart to apply new address" : ""}`;
    document.getElementById("authDeviceName").textContent = deviceName;
    document.getElementById("authDeviceHost").textContent = hostLabel;
}

function setDeviceIdentity(payload = {}) {
    const nextDefaultDeviceName = normalizeDeviceName(payload.defaultDeviceName) || deviceIdentity.defaultDeviceName || "SugarCube";
    const nextDeviceName = normalizeDeviceName(payload.deviceName) || nextDefaultDeviceName;
    const nextHostName = String(payload.hostName || buildHostnameFromDeviceName(nextDeviceName));

    deviceIdentity.defaultDeviceName = nextDefaultDeviceName;
    deviceIdentity.deviceName = nextDeviceName;
    deviceIdentity.hostName = nextHostName;
    deviceIdentity.rebootRequired = Boolean(payload.rebootRequired);
    updateDeviceMeta();
    updateDeviceIdentityPreview();
}

function updateDeviceIdentityPreview() {
    const input = document.getElementById("deviceName");
    const previewName = normalizeDeviceName(input ? input.value : "") ||
        normalizeDeviceName(deviceIdentity.deviceName) ||
        defaultDeviceNameLabel();
    const previewHost = buildHostnameFromDeviceName(previewName);

    document.getElementById("deviceNamePreview").textContent = previewName;
    document.getElementById("deviceHostPreview").textContent = `${previewHost}.local`;
}

function setSyncState(label, tone = "") {
    const element = document.getElementById("syncState");
    element.textContent = label;
    element.className = `pill ${tone}`.trim();
}

function setAuthMessage(message = "", tone = "") {
    const element = document.getElementById("authMessage");
    element.textContent = message;
    element.className = `auth-message ${tone}`.trim();
}

function updateAuthGuidance() {
    const revealButton = document.getElementById("authRevealButton");
    const setupNote = document.getElementById("authSetupNote");
    const recoveryNote = document.getElementById("authRecoveryNote");

    revealButton.classList.toggle("hidden", !authCanRevealPassword);

    if (authCanRevealPassword) {
        document.getElementById("authHint").textContent = authProvisionedThisBoot
            ? "A generated setup password was created on this boot. If you missed it, show it on the device screen."
            : "This device is still using its generated setup password. You can show it on the device screen.";
        setupNote.innerHTML = "<strong>First install or reset</strong>Connect to <code>SugarCube-Setup</code>, open <code>192.168.4.1</code>, then tap <code>Show password on device</code> if you missed the setup password.";
        recoveryNote.innerHTML = "<strong>Security model</strong>The reveal action only works while the generated setup password is still active. After you replace it, the device keeps only a secure hash.";
        return;
    }

    document.getElementById("authHint").textContent = "Sign in to manage the device.";
    setupNote.innerHTML = "<strong>First install or reset</strong>Connect to <code>SugarCube-Setup</code> and open <code>192.168.4.1</code>. New devices provisioned by this firmware can reveal the generated setup password from this page.";
    recoveryNote.innerHTML = "<strong>Forgot a password?</strong>Custom or older passwords cannot be displayed because only a secure hash is stored. Use 5 quick power cycles to factory reset, then sign in with the new generated setup password.";
}

function setAuthenticatedState(nextState) {
    authenticated = Boolean(nextState);
    document.getElementById("appRoot").inert = !authenticated;
    document.body.classList.toggle("auth-open", !authenticated);
    document.getElementById("authOverlay").classList.toggle("active", !authenticated);
    document.getElementById("signOutButton").classList.toggle("hidden", !authenticated);
    updateActionButtons();
}

function showAuthOverlay(message = "", tone = "") {
    setAuthenticatedState(false);
    setAuthMessage(message, tone);
    document.getElementById("authPassword").value = "";
    document.getElementById("authPassword").focus();
}

function hideAuthOverlay() {
    setAuthenticatedState(true);
    setAuthMessage("");
    document.querySelector(".tab.active").focus();
}

function trapAuthFocus(event) {
    if (authenticated || event.key !== "Tab") return;
    const controls = Array.from(document.getElementById("authOverlay")
        .querySelectorAll("input:not(:disabled), button:not(:disabled)"))
        .filter((control) => control.offsetParent !== null);
    const first = controls[0];
    const last = controls[controls.length - 1];
    if (!first) return;
    if (event.shiftKey && document.activeElement === first) {
        event.preventDefault();
        last.focus();
    } else if (!event.shiftKey && document.activeElement === last) {
        event.preventDefault();
        first.focus();
    }
}

function handleUnauthorized(message = "Sign in to continue.") {
    hydrating = false;
    showAuthOverlay(message, "error");
    setSyncState("Locked", "error");
    updateActionButtons();
}

async function request(url, options = {}) {
    const response = await fetch(url, options);
    if (response.status === 401 && url !== "/auth/login" && url !== "/auth/status") {
        handleUnauthorized("Session expired. Sign in again.");
    }
    return response;
}

async function postJson(url, body) {
    return request(url, {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify(body)
    });
}

async function refreshAuthStatus() {
    const response = await fetch("/auth/status");
    if (!response.ok) {
        throw new Error("Auth status failed");
    }

    const authState = await response.json();
    setDeviceIdentity(authState);
    authProvisionedThisBoot = Boolean(authState.provisionedThisBoot);
    authCanRevealPassword = Boolean(authState.canRevealPassword);
    document.getElementById("authUsernameLabel").textContent = authState.username || "admin";
    document.getElementById("securityStatus").textContent = authState.authenticated
        ? (authCanRevealPassword
            ? `Signed in as ${authState.username || "admin"}. Setup password reveal stays available until you replace it.`
            : `Signed in as ${authState.username || "admin"}.`)
        : (authCanRevealPassword
            ? `Admin user: ${authState.username || "admin"}. Setup password reveal is available from the login screen.`
            : `Admin user: ${authState.username || "admin"}.`);
    updateAuthGuidance();

    if (authState.authenticated) {
        hideAuthOverlay();
    } else {
        showAuthOverlay(authCanRevealPassword ? "Use the reveal button if you need the setup password." : "");
    }
}

function updateActionButtons() {
    const locked = hydrating || !authenticated;
    document.getElementById("saveDisplayButton").disabled = locked || !dirty;
    document.getElementById("discardDisplayButton").disabled = locked || !dirty;
    document.getElementById("saveNightscoutButton").disabled = locked || !feedDirty;
    document.getElementById("discardNightscoutButton").disabled = locked || !feedDirty;
}

function formatBytes(bytes) {
    if (!Number.isFinite(bytes) || bytes < 0) {
        return "--";
    }
    if (bytes < 1024) {
        return `${bytes} B`;
    }
    if (bytes < 1024 * 1024) {
        return `${(bytes / 1024).toFixed(1)} KB`;
    }
    return `${(bytes / (1024 * 1024)).toFixed(2)} MB`;
}

function formatAge(ageSec) {
    if (!Number.isFinite(ageSec) || ageSec < 0) {
        return "--";
    }
    if (ageSec < 15) {
        return "Just now";
    }
    if (ageSec < 3600) {
        return `${Math.max(1, Math.round(ageSec / 60))}m ago`;
    }
    return `${Math.round(ageSec / 3600)}h ago`;
}

function sourceLabel(source) {
    if (source === "open-meteo") {
        return "Open-Meteo";
    }
    return "Disabled";
}

function normalizeWeatherSourceForUI(source) {
    return source === "open-meteo" ? "open-meteo" : "disabled";
}

function normalizeHexColor(value, fallback) {
    const base = (value || "").trim().replace(/^#/, "").toUpperCase();
    if (/^[0-9A-F]{6}$/.test(base)) {
        return `#${base}`;
    }
    return fallback;
}

function readNumber(id, fallback = 0) {
    const value = parseFloat(document.getElementById(id).value);
    return Number.isFinite(value) ? value : fallback;
}

function readInt(id, fallback = 0) {
    const value = parseInt(document.getElementById(id).value, 10);
    return Number.isFinite(value) ? value : fallback;
}

function setValue(id, value) {
    document.getElementById(id).value = value ?? "";
}

function setChecked(id, value) {
    document.getElementById(id).checked = Boolean(value);
}

function ensurePageSelection(preferredId = "pageClock") {
    const anyEnabled = dashboardPageControls.some(({ id }) => document.getElementById(id).checked);
    if (anyEnabled) {
        return;
    }

    const fallbackId = preferredId || "pageClock";
    setChecked(fallbackId, true);
}

function applyEnabledPages(pages = {}) {
    const hasConfiguredPages = dashboardPageControls.some(({ key }) =>
        Object.prototype.hasOwnProperty.call(pages, key)
    );
    dashboardPageControls.forEach(({ key, id }) => {
        const enabled = hasConfiguredPages ? Boolean(pages[key]) : true;
        setChecked(id, enabled);
    });
    ensurePageSelection();
}

function readEnabledPages() {
    const pages = {};
    dashboardPageControls.forEach(({ key, id }) => {
        pages[key] = document.getElementById(id).checked;
    });
    ensurePageSelection();
    pages.clock = document.getElementById("pageClock").checked;
    return pages;
}

function defaultThemeColors(prefix = "custom") {
    return prefix === "nightCustom" ? defaultNightCustomTheme : defaultCustomTheme;
}

function themeColorControlIds(name, prefix = "custom") {
    const controlPrefix = `${prefix}${name.charAt(0).toUpperCase() + name.slice(1)}`;
    return {
        picker: `${controlPrefix}Picker`,
        hex: `${controlPrefix}Hex`
    };
}

function setThemeColorControl(name, value, prefix = "custom") {
    const fallback = defaultThemeColors(prefix)[name];
    const normalized = normalizeHexColor(value, fallback);
    const ids = themeColorControlIds(name, prefix);
    setValue(ids.hex, normalized);
    setValue(ids.picker, normalized);
}

function getThemeColorValue(name, prefix = "custom") {
    const defaults = defaultThemeColors(prefix);
    const ids = themeColorControlIds(name, prefix);
    const pickerValue = normalizeHexColor(document.getElementById(ids.picker).value, defaults[name]);
    return normalizeHexColor(document.getElementById(ids.hex).value, pickerValue);
}

function bindThemeColorInputs(prefix = "custom") {
    const defaults = defaultThemeColors(prefix);
    ["accent", "background", "surface", "text"].forEach((name) => {
        const ids = themeColorControlIds(name, prefix);
        const picker = document.getElementById(ids.picker);
        const hex = document.getElementById(ids.hex);

        picker.addEventListener("input", () => {
            hex.value = normalizeHexColor(picker.value, defaults[name]);
        });

        hex.addEventListener("input", () => {
            const normalized = normalizeHexColor(hex.value, "");
            if (normalized) {
                picker.value = normalized;
            }
        });

        hex.addEventListener("blur", () => {
            const normalized = getThemeColorValue(name, prefix);
            hex.value = normalized;
            picker.value = normalized;
        });
    });
}

function formatMinutesOfDay(totalMinutes) {
    const safeMinutes = Number.isFinite(totalMinutes) ? totalMinutes : 0;
    const normalized = ((safeMinutes % 1440) + 1440) % 1440;
    const hours = String(Math.floor(normalized / 60)).padStart(2, "0");
    const minutes = String(normalized % 60).padStart(2, "0");
    return `${hours}:${minutes}`;
}

function parseTimeOfDay(value, fallbackMinutes = 0) {
    const match = /^(\d{2}):(\d{2})$/.exec(String(value || "").trim());
    if (!match) {
        return fallbackMinutes;
    }

    const hours = parseInt(match[1], 10);
    const minutes = parseInt(match[2], 10);
    if (!Number.isFinite(hours) || !Number.isFinite(minutes) || hours > 23 || minutes > 59) {
        return fallbackMinutes;
    }

    return (hours * 60) + minutes;
}

function updateCustomThemeUI() {
    const enabled = document.getElementById("customThemeEnabled").checked;
    const fields = document.getElementById("customThemeFields");
    fields.classList.toggle("hidden", !enabled);
    fields.querySelectorAll("input").forEach((input) => {
        input.disabled = !enabled;
    });
}

function populateTimezoneOptions() {
    const timezoneSelect = document.getElementById("timezonePreset");
    timezoneSelect.innerHTML = "";

    timezoneOptions.forEach((optionData) => {
        const option = document.createElement("option");
        option.value = String(optionData.value);
        option.textContent = optionData.label;
        timezoneSelect.appendChild(option);
    });

    const customOption = document.createElement("option");
    customOption.value = "custom";
    customOption.textContent = "Custom";
    timezoneSelect.appendChild(customOption);
}

function populateThemeButtons(containerId, inputId) {
    const container = document.getElementById(containerId);
    container.innerHTML = "";

    themeLabels.forEach((themeData) => {
        const button = document.createElement("button");
        button.type = "button";
        button.className = "theme-button";
        button.dataset.targetInput = inputId;
        button.dataset.theme = String(themeData.value);
        button.title = `${themeData.family} · ${themeData.mode === "dark" ? "Dark" : "Light"}`;

        const preview = document.createElement("span");
        preview.className = "theme-preview";
        preview.setAttribute("aria-hidden", "true");
        (themeData.tones || []).forEach((tone) => {
            const swatch = document.createElement("span");
            swatch.className = "theme-swatch";
            swatch.style.background = tone;
            preview.appendChild(swatch);
        });

        const label = document.createElement("span");
        label.className = "theme-button-title";
        label.textContent = themeData.label;

        const description = document.createElement("span");
        description.className = "muted";
        description.textContent = `${themeData.family} · ${themeData.mode === "dark" ? "Dark" : "Light"}`;
        button.appendChild(preview);
        button.appendChild(label);
        button.appendChild(description);
        button.addEventListener("click", () => {
            document.getElementById(inputId).value = String(themeData.value);
            if (inputId === "nightThemePreset") {
                document.getElementById("nightCustomThemeEnabled").checked = false;
                updateNightThemeUI();
            } else {
                document.getElementById("customThemeEnabled").checked = false;
                updateCustomThemeUI();
            }
            updateThemeButtons(containerId, inputId);
            scheduleLiveSync(90);
        });
        container.appendChild(button);
    });
}

function updateThemeButtons(containerId, inputId) {
    const activeTheme = String(document.getElementById(inputId).value || "0");
    document.querySelectorAll(`#${containerId} .theme-button`).forEach((button) => {
        button.classList.toggle("active", button.dataset.theme === activeTheme);
        button.setAttribute("aria-pressed", button.dataset.theme === activeTheme ? "true" : "false");
    });
}

function handleTimezonePresetChange() {
    const preset = document.getElementById("timezonePreset").value;
    const customWrap = document.getElementById("timezoneCustomWrap");
    if (preset === "custom") {
        customWrap.classList.remove("hidden");
    } else {
        customWrap.classList.add("hidden");
        document.getElementById("timezoneCustom").value = preset;
    }
}

function applyTimezoneToUI(gmtOffset) {
    const preset = document.getElementById("timezonePreset");
    const custom = document.getElementById("timezoneCustom");
    const match = timezoneOptions.find((optionData) => optionData.value === gmtOffset);

    if (match) {
        preset.value = String(gmtOffset);
        custom.value = String(gmtOffset);
    } else {
        preset.value = "custom";
        custom.value = String(gmtOffset);
    }

    handleTimezonePresetChange();
}

function getTimezoneFromUI() {
    const preset = document.getElementById("timezonePreset").value;
    if (preset === "custom") {
        const customValue = parseInt(document.getElementById("timezoneCustom").value, 10);
        return Number.isFinite(customValue) ? customValue : null;
    }

    const presetValue = parseInt(preset, 10);
    return Number.isFinite(presetValue) ? presetValue : null;
}

function updateBrightnessLabel() {
    document.getElementById("brightnessValue").textContent = `${readInt("brightness", 70)}%`;
}

function updateNightBrightnessLabel() {
    const value = readInt("nightBrightness", 14);
    document.getElementById("nightBrightnessValue").textContent = `${Math.max(1, value)}%`;
}

function updateNightThemeUI() {
    const nightEnabled = document.getElementById("nightModeEnabled").checked;
    const themeEnabled = nightEnabled && document.getElementById("nightThemeEnabled").checked;
    const customEnabled = themeEnabled && document.getElementById("nightCustomThemeEnabled").checked;
    const presetWrap = document.getElementById("nightThemePresetWrap");
    const customToggle = document.getElementById("nightCustomThemeToggle");
    const customFields = document.getElementById("nightCustomThemeFields");

    presetWrap.classList.toggle("hidden", !themeEnabled);
    customToggle.classList.toggle("hidden", !themeEnabled);
    customFields.classList.toggle("hidden", !customEnabled);

    presetWrap.querySelectorAll("button, input").forEach((element) => {
        element.disabled = !themeEnabled;
    });
    customToggle.querySelectorAll("input").forEach((element) => {
        element.disabled = !themeEnabled;
    });
    customFields.querySelectorAll("input").forEach((element) => {
        element.disabled = !customEnabled;
    });
}

function updateNightModeUI() {
    const enabled = document.getElementById("nightModeEnabled").checked;
    const fields = document.getElementById("nightModeFields");

    fields.classList.toggle("hidden", !enabled);
    fields.querySelectorAll("input, button").forEach((element) => {
        element.disabled = !enabled;
    });

    updateNightBrightnessLabel();
}

function applyAppState(appState) {
    setDeviceIdentity(appState);
    setValue("deviceName", appState.deviceName ?? appState.defaultDeviceName ?? "");
    setValue("brightness", appState.brt ?? 70);
    applyTimezoneToUI(typeof appState.gmtOffset === "number" ? appState.gmtOffset : 0);
    updateBrightnessLabel();
    updateDeviceIdentityPreview();
}

let retainedTheme = {
    theme: 0,
    customThemeEnabled: false,
    customTheme: null,
    nightThemeEnabled: false,
    nightTheme: 0,
    nightCustomThemeEnabled: false,
    nightCustomTheme: null
};

function applyDashboardState(dashboardState) {
    const config = dashboardState.config || {};
    const customTheme = config.customTheme || {};
    const nightMode = config.nightMode || {};
    const nightCustomTheme = nightMode.customTheme || {};
    retainedTheme = {
        theme: config.theme ?? 0,
        customThemeEnabled: Boolean(config.customThemeEnabled),
        customTheme: {
            background: customTheme.background || defaultCustomTheme.background,
            surface: customTheme.surface || defaultCustomTheme.surface,
            accent: customTheme.accent || defaultCustomTheme.accent,
            text: customTheme.text || defaultCustomTheme.text
        },
        nightThemeEnabled: Boolean(nightMode.themeEnabled),
        nightTheme: nightMode.theme ?? 0,
        nightCustomThemeEnabled: Boolean(nightMode.customThemeEnabled),
        nightCustomTheme: {
            background: nightCustomTheme.background || defaultNightCustomTheme.background,
            surface: nightCustomTheme.surface || defaultNightCustomTheme.surface,
            accent: nightCustomTheme.accent || defaultNightCustomTheme.accent,
            text: nightCustomTheme.text || defaultNightCustomTheme.text
        }
    };

    setChecked("nightModeEnabled", nightMode.enabled);
    setValue("nightStart", formatMinutesOfDay(nightMode.startMinutes ?? (22 * 60)));
    setValue("nightEnd", formatMinutesOfDay(nightMode.endMinutes ?? (7 * 60)));
    setValue("nightBrightness", Math.max(1, nightMode.brightness ?? 14));
    updateNightModeUI();

    setChecked("use24Hour", config.use24Hour);
    setChecked("showSeconds", config.showSeconds);
    setChecked("showIp", config.showIp);
}

function updateWeatherModeUI() {
    const source = normalizeWeatherSourceForUI(document.getElementById("weatherSource").value);
    const weatherSource = document.getElementById("weatherSource");
    const autoFields = document.getElementById("weatherAutoFields");
    weatherSource.value = source;
    autoFields.classList.toggle("hidden", source !== "open-meteo");
    document.getElementById("weatherUseFahrenheit").disabled = source !== "open-meteo";
    document.getElementById("weatherFeedBadge").textContent = sourceLabel(source);

    autoFields.querySelectorAll("input, button").forEach((element) => {
        element.disabled = source !== "open-meteo";
    });

    if (source !== "open-meteo") {
        document.getElementById("weatherResults").innerHTML = "";
        document.getElementById("weatherSearchStatus").textContent = "Weather feed disabled.";
    } else if (!document.getElementById("weatherSearchStatus").textContent.trim() ||
               document.getElementById("weatherSearchStatus").textContent === "Weather feed disabled.") {
        document.getElementById("weatherSearchStatus").textContent = "Search a city or set coordinates directly.";
    }
}

function updateNightscoutModeUI() {
    const enabled = document.getElementById("nightscoutEnabled").checked;
    const fields = document.getElementById("nightscoutFields");
    fields.classList.toggle("hidden", !enabled);
    fields.querySelectorAll("input, select, button").forEach((element) => {
        element.disabled = !enabled;
    });
    document.getElementById("nightscoutSyncButton").disabled = !enabled;
    const host = document.getElementById("nightscoutBaseUrl").value.trim()
        .replace(/^https?:\/\//, "").split("/")[0];
    document.getElementById("nightscoutFeedBadge").textContent = enabled ? (host || "Not set") : "Disabled";
}

function feedStatusText(source, status, fallbackText) {
    if (source === "disabled") {
        return "Disabled";
    }
    if (status?.syncing) {
        return "Syncing";
    }
    if (status?.lastError) {
        return status.lastError.length > 24 ? `${status.lastError.slice(0, 23)}...` : status.lastError;
    }
    if (status?.hasData) {
        return `${status.stale ? "Stale · " : ""}${formatAge(status.lastSuccessAgeSec)}`;
    }
    return fallbackText;
}

function applyFeedState(feedState) {
    const config = feedState.config || {};
    const status = feedState.status || {};

    const nightscout = config.nightscout || {};
    const nightscoutStatus = status.nightscout || {};
    setChecked("nightscoutEnabled", nightscout.enabled);
    setValue("nightscoutBaseUrl", nightscout.baseUrl || "");
    setValue("nightscoutToken", nightscout.token || "");
    setChecked("nightscoutUseMmol", nightscout.useMmol);
    setChecked("nightscoutShowIob", nightscout.showIob ?? true);
    setValue("nightscoutRefreshSeconds", nightscout.refreshSeconds ?? 30);
    thresholdUnitIsMmol = false;
    setValue("nightscoutUrgentLow", nightscout.urgentLowMgdl ?? 54);
    setValue("nightscoutLow", nightscout.lowMgdl ?? 70);
    setValue("nightscoutHigh", nightscout.highMgdl ?? 180);
    setValue("nightscoutUrgentHigh", nightscout.urgentHighMgdl ?? 250);
    applyThresholdUnit(Boolean(nightscout.useMmol));
    updateNightscoutModeUI();
    document.getElementById("nightscoutFeedStatus").textContent = feedStatusText(
        nightscout.enabled ? "nightscout" : "disabled",
        nightscoutStatus,
        "Waiting"
    );
    document.getElementById("nightscoutReading").textContent = nightscout.enabled && nightscoutStatus.hasData
        ? `${nightscout.useMmol ? (nightscoutStatus.sgv / 18.0182).toFixed(1) : nightscoutStatus.sgv} ${nightscoutStatus.direction || ""}`.trim()
        : "--";
}

function buildSettingsPayload() {
    return {
        deviceName: document.getElementById("deviceName").value.trim(),
        brightness: readInt("brightness", 70),
        gmtOffset: getTimezoneFromUI()
    };
}

function buildConfigPayload() {
    const nightMode = {
        enabled: document.getElementById("nightModeEnabled").checked,
        startMinutes: parseTimeOfDay(document.getElementById("nightStart").value, 22 * 60),
        endMinutes: parseTimeOfDay(document.getElementById("nightEnd").value, 7 * 60),
        brightness: Math.max(1, readInt("nightBrightness", 14)),
        themeEnabled: retainedTheme.nightThemeEnabled,
        theme: retainedTheme.nightTheme,
        customThemeEnabled: retainedTheme.nightCustomThemeEnabled,
        customTheme: retainedTheme.nightCustomTheme || defaultNightCustomTheme
    };

    return {
        theme: retainedTheme.theme,
        customThemeEnabled: retainedTheme.customThemeEnabled,
        customTheme: retainedTheme.customTheme || defaultCustomTheme,
        nightMode,
        pages: { clock: false, nightscout: true, weather: false },
        rotationEnabled: false,
        rotationIntervalSec: 10,
        use24Hour: document.getElementById("use24Hour").checked,
        showSeconds: document.getElementById("showSeconds").checked,
        showIp: document.getElementById("showIp").checked
    };
}

const nightscoutThresholds = [
    { id: "nightscoutUrgentLow", label: "Urgent low", mgMin: 30, mgMax: 120, fallback: 54 },
    { id: "nightscoutLow", label: "Low", mgMin: 40, mgMax: 200, fallback: 70 },
    { id: "nightscoutHigh", label: "High", mgMin: 100, mgMax: 400, fallback: 180 },
    { id: "nightscoutUrgentHigh", label: "Urgent high", mgMin: 120, mgMax: 600, fallback: 250 }
];
const mgdlPerMmol = 18.0182;
let thresholdUnitIsMmol = false;

function mgdlToMmol(mgdl) {
    return Math.round((mgdl / mgdlPerMmol) * 10) / 10;
}

function mmolToMgdl(mmol) {
    return Math.round(mmol * mgdlPerMmol);
}

function readThresholdMgdl(field) {
    const value = parseFloat(document.getElementById(field.id).value);
    if (!Number.isFinite(value)) return field.fallback;
    return thresholdUnitIsMmol ? mmolToMgdl(value) : Math.round(value);
}

function applyThresholdUnit(useMmol) {
    nightscoutThresholds.forEach((field) => {
        const input = document.getElementById(field.id);
        const label = document.querySelector(`label[for="${field.id}"]`);
        const current = parseFloat(input.value);
        if (useMmol && !thresholdUnitIsMmol) {
            const mgdl = Number.isFinite(current) ? current : field.fallback;
            input.value = mgdlToMmol(mgdl).toFixed(1);
        } else if (!useMmol && thresholdUnitIsMmol) {
            const mmol = Number.isFinite(current) ? current : mgdlToMmol(field.fallback);
            input.value = String(mmolToMgdl(mmol));
        } else if (!Number.isFinite(current)) {
            input.value = useMmol ? mgdlToMmol(field.fallback).toFixed(1) : String(field.fallback);
        }
        input.step = useMmol ? "0.1" : "1";
        input.min = useMmol ? mgdlToMmol(field.mgMin) : field.mgMin;
        input.max = useMmol ? mgdlToMmol(field.mgMax) : field.mgMax;
        if (label) label.textContent = `${field.label} (${useMmol ? "mmol/L" : "mg/dL"})`;
    });
    thresholdUnitIsMmol = useMmol;
}

function readRefreshSeconds() {
    const value = parseInt(document.getElementById("nightscoutRefreshSeconds").value, 10);
    if (!Number.isFinite(value)) return 30;
    return Math.min(3600, Math.max(15, value));
}

function buildFeedPayload() {
    return {
        nightscout: {
            enabled: document.getElementById("nightscoutEnabled").checked,
            baseUrl: document.getElementById("nightscoutBaseUrl").value.trim(),
            token: document.getElementById("nightscoutToken").value.trim(),
            useMmol: document.getElementById("nightscoutUseMmol").checked,
            showIob: document.getElementById("nightscoutShowIob").checked,
            refreshSeconds: readRefreshSeconds(),
            urgentLowMgdl: readThresholdMgdl(nightscoutThresholds[0]),
            lowMgdl: readThresholdMgdl(nightscoutThresholds[1]),
            highMgdl: readThresholdMgdl(nightscoutThresholds[2]),
            urgentHighMgdl: readThresholdMgdl(nightscoutThresholds[3])
        }
    };
}

function buildLivePayload() {
    return {
        settings: buildSettingsPayload(),
        config: buildConfigPayload()
    };
}

function scheduleFeedPreview(delay = 180) {
    if (!authenticated) {
        return;
    }

    feedDirty = true;
    feedEditGeneration += 1;
    setSyncState("Feeds pending", "pending");
    window.clearTimeout(feedLiveTimer);
    updateActionButtons();

    feedLiveTimer = window.setTimeout(() => {
        feedLiveTimer = 0;
        const generation = feedEditGeneration;
        feedPromise = doFeedPreview();
        feedPromise.catch((error) => {
            console.error(error);
            if (authenticated && generation === feedEditGeneration) setSyncState("Feed preview failed", "error");
        });
    }, delay);
}

function scheduleLiveSync(delay = 180) {
    if (!authenticated) {
        return;
    }

    dirty = true;
    editGeneration += 1;
    setSyncState("Pending", "pending");
    window.clearTimeout(liveTimer);
    updateActionButtons();

    liveTimer = window.setTimeout(() => {
        liveTimer = 0;
        const generation = editGeneration;
        livePromise = doLiveSync();
        livePromise.catch((error) => {
            console.error(error);
            if (authenticated && generation === editGeneration) setSyncState("Preview failed", "error");
        });
    }, delay);
}

async function doLiveSync() {
    return queueOperation(async () => {
        const generation = editGeneration;
        const timezone = getTimezoneFromUI();
        if (timezone === null) {
            setSyncState("Timezone", "error");
            throw new Error("Invalid timezone");
        }

        const payload = buildLivePayload();
        setSyncState("Preview", "pending");

        const response = await postJson("/dashboard/live", payload);

        if (!response.ok) {
            throw new Error(await response.text());
        }

        setSyncState(editGeneration === generation ? "Display draft" : "Pending",
                     editGeneration === generation ? "live" : "pending");
    });
}

async function doFeedPreview() {
    return queueOperation(async () => {
        const generation = feedEditGeneration;
        const payload = buildFeedPayload();
        setSyncState("Feeds preview", "pending");

        const response = await postFeedConfig("/feeds/live", payload);

        if (!response.ok) {
            throw new Error(await response.text());
        }

        setSyncState(feedEditGeneration === generation ? "Feeds draft" : "Feeds pending",
                     feedEditGeneration === generation ? "live" : "pending");
    });
}

async function flushLiveSync() {
    if (liveTimer) {
        window.clearTimeout(liveTimer);
        liveTimer = 0;
        livePromise = doLiveSync();
    }

    return livePromise.catch((error) => {
        if (!authenticated) throw error;
        livePromise = doLiveSync();
        return livePromise;
    });
}

async function flushFeedPreview() {
    if (feedLiveTimer) {
        window.clearTimeout(feedLiveTimer);
        feedLiveTimer = 0;
        feedPromise = doFeedPreview();
    }

    return feedPromise.catch((error) => {
        if (!authenticated) throw error;
        feedPromise = doFeedPreview();
        return feedPromise;
    });
}

async function commitState() {
    if (hydrating || !dirty) {
        return;
    }

    try {
        await flushLiveSync();
        await queueOperation(async () => {
            setSyncState("Saving", "pending");

            const generation = editGeneration;
            const payload = buildLivePayload();
            const response = await postJson("/dashboard/save", payload);
            if (!response.ok) {
                throw new Error(await response.text());
            }

            dirty = editGeneration !== generation;
            const appResponse = await request("/app.json");
            if (appResponse.ok) setDeviceIdentity(await appResponse.json());
            updateActionButtons();
            setSyncState(dirty ? "Pending" : "Saved", dirty ? "pending" : "saved");
        });
    } catch (error) {
        console.error(error);
        setSyncState("Save failed", "error");
    }
}

async function commitFeedConfig() {
    if (hydrating || !feedDirty) {
        return;
    }

    try {
        await flushFeedPreview();
        await queueOperation(async () => {
            setSyncState("Saving feeds", "pending");

            const generation = feedEditGeneration;
            const payload = buildFeedPayload();
            const response = await postFeedConfig("/feeds/save", payload);
            if (!response.ok) {
                throw new Error(await response.text());
            }

            feedDirty = feedEditGeneration !== generation;
            updateActionButtons();
            setSyncState(feedDirty ? "Feeds pending" : "Saved", feedDirty ? "pending" : "saved");
            if (!feedDirty) await loadFeedStateOnly();
        });
    } catch (error) {
        console.error(error);
        setSyncState("Feed save failed", "error");
    }
}

async function discardState() {
    if (hydrating || !dirty) {
        return;
    }

    window.clearTimeout(liveTimer);
    liveTimer = 0;

    const generation = editGeneration;
    try {
        await queueOperation(async () => {
            const response = await request("/dashboard/discard", { method: "POST" });
            if (!response.ok) {
                throw new Error(await response.text());
            }

            dirty = editGeneration !== generation;
            updateActionButtons();
            if (!dirty) await loadAllState();
            setSyncState("Discarded", "saved");
        });
    } catch (error) {
        console.error(error);
        setSyncState("Discard failed", "error");
    }
}

async function discardFeedConfig() {
    if (hydrating || !feedDirty) {
        return;
    }

    window.clearTimeout(feedLiveTimer);
    feedLiveTimer = 0;

    const generation = feedEditGeneration;
    try {
        await queueOperation(async () => {
            const response = await request("/feeds/discard", { method: "POST" });
            if (!response.ok) {
                throw new Error(await response.text());
            }

            feedDirty = feedEditGeneration !== generation;
            updateActionButtons();
            if (!feedDirty) await loadFeedStateOnly();
            setSyncState("Discarded", "saved");
        });
    } catch (error) {
        console.error(error);
        setSyncState("Discard failed", "error");
    }
}

async function resetDashboardDefaults() {
    if (!confirm("Reset layout and widget content?")) {
        return;
    }

    window.clearTimeout(liveTimer);
    liveTimer = 0;
    const generation = editGeneration;

    try {
        const response = await queueOperation(() => request("/dashboard/reset", { method: "POST" }));
        if (!response.ok) {
            throw new Error(await response.text());
        }
        dirty = editGeneration !== generation;
        updateActionButtons();
        if (!dirty) await loadAllState();
        setSyncState(dirty ? "Pending" : "Saved", dirty ? "pending" : "saved");
    } catch (error) {
        console.error(error);
        setSyncState("Reset failed", "error");
    }
}

async function resetFeedDefaults() {
    if (!confirm("Reset feed providers and live source settings?")) {
        return;
    }

    window.clearTimeout(feedLiveTimer);
    feedLiveTimer = 0;
    const generation = feedEditGeneration;

    try {
        const response = await queueOperation(() => request("/feeds/reset", { method: "POST" }));
        if (!response.ok) {
            throw new Error(await response.text());
        }
        feedDirty = feedEditGeneration !== generation;
        updateActionButtons();
        if (!feedDirty) await loadFeedStateOnly();
        setSyncState(feedDirty ? "Feeds pending" : "Saved", feedDirty ? "pending" : "saved");
    } catch (error) {
        console.error(error);
        setSyncState("Feed reset failed", "error");
    }
}

function renderWeatherResults(results) {
    const container = document.getElementById("weatherResults");
    container.innerHTML = "";

    if (!Array.isArray(results) || results.length === 0) {
        document.getElementById("weatherSearchStatus").textContent = "No places found.";
        return;
    }

    results.forEach((result) => {
        const button = document.createElement("button");
        button.type = "button";
        button.className = "search-result";
        const label = document.createElement("div");
        label.textContent = result.label || result.name || "Result";
        const meta = document.createElement("small");
        meta.textContent = `${result.timezone || "No timezone"} · ${result.latitude}, ${result.longitude}`;
        button.appendChild(label);
        button.appendChild(meta);
        button.addEventListener("click", () => {
            setValue("weatherQuery", result.name || "");
            setValue("weatherLabel", result.label || result.name || "");
            setValue("weatherLatitude", result.latitude ?? "");
            setValue("weatherLongitude", result.longitude ?? "");
            document.getElementById("weatherSearchStatus").textContent = `${result.label || result.name} selected.`;
            scheduleFeedPreview(40);
            syncFeeds("weather");
        });
        container.appendChild(button);
    });
}

async function waitQueuedResponse(response, pollUrl) {
    while (response.status === 202) {
        await new Promise((resolve) => window.setTimeout(resolve, 300));
        response = await request(pollUrl);
    }
    return response;
}

async function postFeedConfig(url, payload) {
    let response = await postJson(url, payload);
    if (response.status === 409) {
        const idle = await waitQueuedResponse(await request("/feeds/job"), "/feeds/job");
        if (!idle.ok) throw new Error(await idle.text());
        response = await postJson(url, payload);
    }
    return response;
}

async function searchWeatherLocations() {
    const query = document.getElementById("weatherQuery").value.trim();
    if (query.length < 2) {
        document.getElementById("weatherSearchStatus").textContent = "Enter at least 2 characters.";
        setSyncState("Search query", "error");
        return;
    }

    document.getElementById("weatherSearchStatus").textContent = "Searching...";
    document.getElementById("weatherResults").innerHTML = "";

    try {
        const response = await waitQueuedResponse(
            await request(`/feeds/search?query=${encodeURIComponent(query)}`), "/feeds/search");
        if (!response.ok) {
            const message = (await response.text()).trim();
            throw new Error(message || "Search failed");
        }

        const payload = await response.json();
        renderWeatherResults(payload.results || []);
        setSyncState("Search ready", "saved");
    } catch (error) {
        console.error(error);
        const message = error instanceof Error && error.message ? error.message : "Search failed";
        document.getElementById("weatherSearchStatus").textContent = message;
        setSyncState("Search failed", "error");
    }
}

async function syncFeeds(scope = "all") {
    try {
        await flushFeedPreview();
        setSyncState("Syncing feeds", "pending");
        const generation = feedEditGeneration;
        const response = await waitQueuedResponse(
            await request(`/feeds/sync?scope=${encodeURIComponent(scope)}`, { method: "POST" }), "/feeds/job");
        if (!response.ok) {
            const message = (await response.text()).trim();
            throw new Error(message || "Feed sync failed");
        }
        const feedState = await response.json();
        if (generation === feedEditGeneration) {
            applyFeedState(feedState);
            feedDirty = Boolean(feedState.meta?.hasDraft);
        }
        if (feedState.meta?.lastSyncSuccess === false) throw new Error(feedState.meta.lastSyncError || "Feed sync failed");
        updateActionButtons();
        setSyncState(feedDirty ? "Feeds synced, draft open" : "Feeds synced", feedDirty ? "live" : "saved");
    } catch (error) {
        console.error(error);
        try {
            await loadFeedStateOnly();
        } catch (loadError) {
            console.error(loadError);
        }
        const message = error instanceof Error && error.message ? error.message : "Sync failed";
        if (scope === "weather") {
            document.getElementById("weatherSearchStatus").textContent = message;
        }
        setSyncState(message.length <= 24 ? message : "Sync failed", "error");
    }
}







function reconfigureWiFi() {
    if (!confirm("Clear saved WiFi and restart into AP mode?")) {
        return;
    }

    request("/reconfigurewifi", { method: "POST" })
        .then((response) => response.text().then((message) => ({ ok: response.ok, message })))
        .then(({ ok, message }) => {
            if (!ok) {
                throw new Error(message || "Portal failed");
            }
            setSyncState("Restarting AP", "pending");
        })
        .catch((error) => {
            console.error(error);
            setSyncState("Portal failed", "error");
        });
}

function restartDevice() {
    if (!confirm("Restart SugarCube?")) {
        return;
    }

    request("/restart", { method: "POST" })
        .then((response) => response.text().then((message) => ({ ok: response.ok, message })))
        .then(({ ok, message }) => {
            if (!ok) {
                throw new Error(message || "Restart failed");
            }
            setSyncState("Restarting", "pending");
        })
        .catch((error) => {
            console.error(error);
            setSyncState("Restart failed", "error");
        });
}

function factoryReset() {
    if (!confirm("Factory reset the device?")) {
        return;
    }

    request("/factoryreset", { method: "POST" })
        .then((response) => response.text().then((message) => ({ ok: response.ok, message })))
        .then(({ ok, message }) => {
            if (!ok) {
                throw new Error(message || "Reset failed");
            }
            setSyncState("Resetting", "pending");
        })
        .catch((error) => {
            console.error(error);
            setSyncState("Reset failed", "error");
        });
}

function scanWiFi() {
    const scanStatus = document.getElementById("scanStatus");
    scanStatus.textContent = "Scanning…";

    request("/scan?start=1")
        .then((response) => waitQueuedResponse(response, "/scan"))
        .then(async (response) => {
            if (!response.ok) {
                const message = (await response.text()).trim();
                throw new Error(message || "Scan failed");
            }
            return response.json();
        })
        .then((networks) => {
            const select = document.getElementById("wifiNetwork");
            select.innerHTML = '<option value="">Select…</option>';

            if (!Array.isArray(networks) || networks.length === 0) {
                scanStatus.textContent = "No networks";
                return;
            }

            networks
                .sort((left, right) => right.rssi - left.rssi)
                .forEach((network) => {
                    if (!network.ssid || !network.ssid.trim()) {
                        return;
                    }

                    const option = document.createElement("option");
                    option.value = network.ssid;
                    option.textContent = `${network.ssid} (${network.rssi} dBm)`;
                    select.appendChild(option);
                });

            scanStatus.textContent = `${networks.length} found`;
        })
        .catch((error) => {
            console.error(error);
            const message = error instanceof Error && error.message ? error.message : "Scan failed";
            scanStatus.textContent = message.length <= 28 ? message : "Scan failed";
        });
}

function connectWiFi() {
    const ssid = document.getElementById("wifiNetwork").value;
    const password = document.getElementById("wifiPassword").value;

    if (!ssid) {
        setSyncState("Pick network", "error");
        return;
    }

    if (!confirm(`Connect to ${ssid}?`)) {
        return;
    }

    postJson("/connect", { ssid, password })
        .then((response) => response.text().then((message) => ({ ok: response.ok, message })))
        .then(({ ok, message }) => {
            if (!ok) {
                throw new Error(message || "Connect failed");
            }
            setSyncState("Connecting", "pending");
        })
        .then(() => {
            document.getElementById("wifiPassword").value = "";
        })
        .catch((error) => {
            console.error(error);
            setSyncState("Connect failed", "error");
        });
}

async function logoutSession() {
    try {
        await request("/auth/logout", { method: "POST" });
    } catch (error) {
        console.error(error);
    } finally {
        handleUnauthorized("Signed out.");
    }
}

async function revealPasswordOnDevice() {
    if (!authCanRevealPassword) {
        setAuthMessage("Custom or older passwords cannot be displayed.", "error");
        return;
    }

    const button = document.getElementById("authRevealButton");
    button.disabled = true;

    try {
        setAuthMessage("Showing password on device...");
        const response = await request("/auth/reveal", { method: "POST" });
        const message = await response.text();
        if (!response.ok) {
            throw new Error(message || "Reveal failed");
        }

        setAuthMessage(message || "Password shown on device for 15 seconds.");
    } catch (error) {
        console.error(error);
        setAuthMessage(error.message || "Reveal failed.", "error");
    } finally {
        button.disabled = false;
    }
}

async function changeAdminPassword() {
    const currentPassword = document.getElementById("currentAdminPassword").value;
    const newPassword = document.getElementById("newAdminPassword").value;
    const confirmPassword = document.getElementById("confirmAdminPassword").value;

    if (!newPassword || newPassword !== confirmPassword) {
        document.getElementById("securityStatus").textContent = "New password confirmation does not match.";
        setSyncState("Password mismatch", "error");
        return;
    }

    try {
        setSyncState("Updating password", "pending");
        const response = await postJson("/auth/password", {
            currentPassword,
            newPassword
        });
        const message = await response.text();
        if (!response.ok) {
            throw new Error(message || "Password update failed");
        }

        document.getElementById("currentAdminPassword").value = "";
        document.getElementById("newAdminPassword").value = "";
        document.getElementById("confirmAdminPassword").value = "";
        await refreshAuthStatus();
        document.getElementById("securityStatus").textContent = "Password updated. Device restarting; sign in after it reconnects.";
        showAuthOverlay("Password updated. Device restarting; sign in after it reconnects.");
        setSyncState("Password updated", "saved");
    } catch (error) {
        console.error(error);
        document.getElementById("securityStatus").textContent = error.message || "Password update failed.";
        setSyncState("Password failed", "error");
    }
}

async function submitLogin(event) {
    event.preventDefault();

    const password = document.getElementById("authPassword").value;
    if (!password) {
        setAuthMessage("Enter the admin password.", "error");
        return;
    }

    try {
        setAuthMessage("Signing in...");
        const response = await postJson("/auth/login", {
            username: document.getElementById("authUsernameLabel").textContent || "admin",
            password
        });
        const message = await response.text();
        if (!response.ok) {
            throw new Error(message || "Sign-in failed");
        }

        hideAuthOverlay();
        await refreshAuthStatus();
        await loadAllState();
    } catch (error) {
        console.error(error);
        showAuthOverlay(error.message || "Sign-in failed.", "error");
    }
}

function switchTab(tabId) {
    const nextTab = ["display", "nightscout", "network", "system"].includes(tabId)
        ? tabId
        : "display";

    document.querySelectorAll(".tab").forEach((button) => {
        const active = button.dataset.tab === nextTab;
        button.classList.toggle("active", active);
        button.setAttribute("aria-pressed", active ? "true" : "false");
    });
    document.querySelectorAll(".tab-panel").forEach((panel) => {
        const active = panel.id === `panel-${nextTab}`;
        panel.classList.toggle("active", active);
        panel.hidden = !active;
    });

    if (window.location.hash !== `#${nextTab}`) {
        history.replaceState(null, "", `#${nextTab}`);
    }
}

async function loadFeedStateOnly() {
    if (!authenticated) {
        updateActionButtons();
        return;
    }

    const generation = feedEditGeneration;
    const preserveEdits = feedDirty;

    const response = await request("/feeds.json");
    if (response.status === 202) return;
    if (!response.ok) {
        throw new Error("Feed state failed");
    }

    const feedState = await response.json();
    if (!preserveEdits && generation === feedEditGeneration) {
        applyFeedState(feedState);
        feedDirty = Boolean(feedState.meta?.hasDraft);
    }
    updateActionButtons();
}

async function loadAllState() {
    if (!authenticated) {
        handleUnauthorized(authCanRevealPassword ? "Use the reveal button or sign in to continue." : "Sign in to continue.");
        return;
    }

    const generation = editGeneration;
    const feedGeneration = feedEditGeneration;
    const preserveEdits = dirty;
    const preserveFeedEdits = feedDirty;
    hydrating = true;
    setSyncState("Loading", "pending");

    try {
        const [appResponse, dashboardResponse, feedsResponse, versionResponse, spaceResponse] = await Promise.all([
            request("/app.json"),
            request("/dashboard.json"),
            request("/feeds.json"),
            request("/version.json"),
            request("/space.json")
        ]);

        if (dashboardResponse.status === 202 || feedsResponse.status === 202) {
            setSyncState("Feed sync in progress", "pending");
            window.setTimeout(() => loadAllState(), 1000);
            return;
        }

        if (!appResponse.ok || !dashboardResponse.ok || !feedsResponse.ok || !versionResponse.ok || !spaceResponse.ok) {
            throw new Error("State request failed");
        }

        const appState = await appResponse.json();
        const dashboardState = await dashboardResponse.json();
        const feedsState = await feedsResponse.json();
        const versionState = await versionResponse.json();
        const spaceState = await spaceResponse.json();

        if (!preserveEdits && generation === editGeneration) {
            applyAppState(appState);
            applyDashboardState(dashboardState);
            dirty = Boolean(appState.displayDraft || dashboardState.meta?.hasDraft);
        }
        if (!preserveFeedEdits && feedGeneration === feedEditGeneration) {
            applyFeedState(feedsState);
            feedDirty = Boolean(feedsState.meta?.hasDraft);
        }

        const firmwareVersion = versionState.version || "Unknown";
        const storageText = `Storage ${formatBytes(spaceState.free)} free`;
        const memoryText = `RAM ${formatBytes(spaceState.ramFree)} free`;

        document.getElementById("firmwareMeta").textContent = firmwareVersion;
        document.getElementById("storageMeta").textContent = storageText;
        document.getElementById("memoryMeta").textContent = memoryText;
        document.getElementById("systemVersion").textContent = firmwareVersion;
        document.getElementById("systemStorage").textContent = `${formatBytes(spaceState.total)} / ${formatBytes(spaceState.free)} free`;
        document.getElementById("systemMemory").textContent = `${formatBytes(spaceState.ramFree)} free · largest ${formatBytes(spaceState.ramLargest)}`;

        if (dirty && feedDirty) {
            setSyncState("Display + feeds draft", "live");
        } else if (dirty) {
            setSyncState("Display draft", "live");
        } else if (feedDirty) {
            setSyncState("Feeds draft", "live");
        } else if (appState.networkBusy) {
            setSyncState("Network task", "pending");
        } else {
            setSyncState("Ready");
        }
    } catch (error) {
        console.error(error);
        if (authenticated) {
            setSyncState("Load failed", "error");
        }
    } finally {
        hydrating = false;
        updateActionButtons();
    }
}

document.addEventListener("keydown", trapAuthFocus);

document.addEventListener("input", (event) => {
    if (event.target.matches("[data-feed-live]")) {
        if (event.target.id === "nightscoutBaseUrl") {
            updateNightscoutModeUI();
        }
        if (event.target.tagName === "TEXTAREA" || event.target.type === "text" || event.target.type === "password") {
            scheduleFeedPreview(260);
            return;
        }

        scheduleFeedPreview(150);
        return;
    }

    if (event.target.matches("[data-live]")) {
        if (event.target.id === "deviceName") {
            updateDeviceIdentityPreview();
            scheduleLiveSync(260);
            return;
        }

        if (event.target.id === "brightness") {
            updateBrightnessLabel();
            scheduleLiveSync(70);
            return;
        }

        if (event.target.id === "nightBrightness") {
            updateNightBrightnessLabel();
            scheduleLiveSync(70);
            return;
        }

        if (event.target.id === "timezoneCustom") {
            scheduleLiveSync(180);
            return;
        }

        if (event.target.tagName === "TEXTAREA" || event.target.type === "text") {
            scheduleLiveSync(260);
            return;
        }

        scheduleLiveSync(150);
    }
});

document.addEventListener("change", (event) => {
    if (event.target.id === "timezonePreset") {
        handleTimezonePresetChange();
        scheduleLiveSync(100);
        return;
    }

    if (event.target.id === "nightModeEnabled") {
        updateNightModeUI();
        scheduleLiveSync(90);
        return;
    }

    if (event.target.matches("[data-page-toggle]")) {
        ensurePageSelection(event.target.id);
        scheduleLiveSync(90);
        return;
    }

    if (event.target.id === "nightscoutUseMmol") {
        applyThresholdUnit(event.target.checked);
        updateNightscoutModeUI();
        scheduleFeedPreview(80);
        return;
    }

    if (event.target.id === "nightscoutEnabled") {
        updateNightscoutModeUI();
        scheduleFeedPreview(80);
        return;
    }

    if (event.target.matches("[data-feed-live]")) {
        scheduleFeedPreview(100);
        return;
    }

    if (event.target.matches("[data-live]")) {
        scheduleLiveSync(100);
    }
});

document.querySelectorAll(".tab").forEach((button) => {
    button.addEventListener("click", () => switchTab(button.dataset.tab));
});

window.addEventListener("beforeunload", (event) => {
    if (!dirty && !feedDirty) {
        return;
    }

    event.preventDefault();
    event.returnValue = "";
});

window.addEventListener("hashchange", () => {
    switchTab((window.location.hash || "#display").slice(1));
});

window.addEventListener("load", () => {
    refreshUiThemeButtons();
    populateTimezoneOptions();
    handleTimezonePresetChange();
    updateNightModeUI();
    updateDeviceMeta();
    updateDeviceIdentityPreview();
    switchTab((window.location.hash || "#display").slice(1));
    updateActionButtons();
    document.getElementById("authForm").addEventListener("submit", submitLogin);
    refreshAuthStatus()
        .then(() => {
            if (authenticated) {
                return loadAllState();
            }
            setSyncState("Locked", "error");
            return undefined;
        })
        .catch((error) => {
            console.error(error);
            showAuthOverlay("Unable to reach the device.", "error");
            setSyncState("Auth failed", "error");
        });
});
</script>
</body>
</html>

)rawliteral";

const char ota_html[] PROGMEM = R"rawliteral(

<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>SugarCube OTA</title>
<script>
(() => {
    try {
        const savedTheme = localStorage.getItem("smartclock-ui-theme");
        const prefersDark = window.matchMedia && window.matchMedia("(prefers-color-scheme: dark)").matches;
        document.documentElement.dataset.uiTheme =
            savedTheme === "dark" || savedTheme === "light"
                ? savedTheme
                : (prefersDark ? "dark" : "light");
    } catch (error) {
        document.documentElement.dataset.uiTheme = "light";
    }
})();
</script>
<style>
:root {
    --bg: #eef2f9;
    --bg-top: #f8f9ff;
    --bg-glow: rgba(208, 226, 255, 0.95);
    --surface: #f8f9ff;
    --surface-high: rgba(245, 247, 252, 0.94);
    --surface-strong: #f2f5fb;
    --surface-tint: rgba(255, 255, 255, 0.72);
    --panel-border: rgba(108, 117, 135, 0.18);
    --chip-bg: #e8edf7;
    --line: #c4c9d4;
    --line-strong: #a4acba;
    --ink: #151b26;
    --muted: #566070;
    --accent: #0b57d0;
    --accent-strong: #0b57d0;
    --accent-soft: #d3e3fd;
    --accent-ring: rgba(11, 87, 208, 0.14);
    --accent-border: rgba(11, 87, 208, 0.18);
    --danger: #ba1a1a;
    --danger-soft: #ffdad6;
    --danger-border: rgba(186, 26, 26, 0.18);
    --success: #0f9d58;
    --success-soft: rgba(15, 157, 88, 0.14);
    --success-border: rgba(15, 157, 88, 0.2);
    --text-on-accent: #ffffff;
    --shadow: 0 1px 2px rgba(18, 24, 33, 0.06), 0 18px 36px rgba(18, 24, 33, 0.08);
    --radius: 28px;
    --radius-m: 20px;
    --radius-s: 16px;
    --touch: 48px;
    --mono: "Roboto Mono", "IBM Plex Mono", "SFMono-Regular", "Menlo", monospace;
    --sans: "Roboto Flex", "Roboto", "Segoe UI Variable", "Noto Sans", sans-serif;
}
html {
    color-scheme: light;
}
html[data-ui-theme="dark"] {
    color-scheme: dark;
    --bg: #10131a;
    --bg-top: #181c25;
    --bg-glow: rgba(82, 148, 255, 0.12);
    --surface: #11131a;
    --surface-high: rgba(31, 36, 47, 0.96);
    --surface-strong: #20252f;
    --surface-tint: rgba(22, 25, 34, 0.78);
    --panel-border: rgba(195, 198, 208, 0.12);
    --chip-bg: #252b37;
    --line: #424956;
    --line-strong: #5b6474;
    --ink: #edf1fb;
    --muted: #adb6c7;
    --accent: #a8c7fa;
    --accent-strong: #d3e3fd;
    --accent-soft: #284777;
    --accent-ring: rgba(168, 199, 250, 0.18);
    --accent-border: rgba(168, 199, 250, 0.18);
    --danger: #ffb4ab;
    --danger-soft: rgba(255, 180, 171, 0.14);
    --danger-border: rgba(255, 180, 171, 0.18);
    --success: #7bdba6;
    --success-soft: rgba(123, 219, 166, 0.16);
    --success-border: rgba(123, 219, 166, 0.2);
    --shadow: 0 1px 2px rgba(0, 0, 0, 0.28), 0 20px 44px rgba(0, 0, 0, 0.36);
}
* {
    box-sizing: border-box;
}
html, body {
    margin: 0;
    min-height: 100%;
}
body {
    font-family: var(--sans);
    color: var(--ink);
    background:
        radial-gradient(circle at top left, var(--bg-glow), transparent 30%),
        radial-gradient(circle at bottom right, rgba(255, 255, 255, 0.22), transparent 24%),
        linear-gradient(180deg, var(--bg-top) 0%, var(--bg) 100%);
}
a,
button,
input {
    font: inherit;
}
.app {
    max-width: 1080px;
    margin: 0 auto;
    padding: 24px 20px 40px;
}
.topbar {
    display: flex;
    align-items: center;
    justify-content: space-between;
    gap: 18px;
    margin-bottom: 22px;
    padding: 14px 18px;
    border-radius: 30px;
    border: 1px solid var(--panel-border);
    background: var(--surface-tint);
    box-shadow: var(--shadow);
    backdrop-filter: blur(18px);
}
.brand {
    display: flex;
    align-items: center;
    gap: 16px;
    min-width: 0;
}
.brand-mark {
    width: 44px;
    height: 44px;
    border-radius: 16px;
    display: grid;
    place-items: center;
    background: var(--accent-soft);
    color: var(--accent-strong);
    box-shadow: inset 0 0 0 1px var(--accent-border);
}
.brand-mark svg {
    width: 24px;
    height: 24px;
    display: block;
}
.brand h1 {
    margin: 0;
    font-size: 1.28rem;
    font-weight: 600;
    letter-spacing: -0.03em;
}
.brand small {
    display: block;
    color: var(--muted);
    font-size: 0.88rem;
    margin-top: 2px;
    white-space: nowrap;
    overflow: hidden;
    text-overflow: ellipsis;
}
.top-actions {
    display: flex;
    align-items: center;
    justify-content: flex-end;
    gap: 10px;
    flex-wrap: wrap;
}
.mode-switch {
    display: inline-flex;
    align-items: center;
    gap: 4px;
    padding: 4px;
    border-radius: 999px;
    border: 1px solid var(--panel-border);
    background: var(--surface-strong);
}
.mode-button {
    min-height: 36px;
    padding: 0 14px;
    border: 1px solid transparent;
    border-radius: 999px;
    background: transparent;
    color: var(--muted);
    box-shadow: none;
    cursor: pointer;
}
.mode-button.active {
    background: var(--accent-soft);
    color: var(--ink);
}
.mode-button:hover {
    background: var(--chip-bg);
}
.button-link,
button {
    min-height: var(--touch);
    padding: 0 18px;
    border: 1px solid transparent;
    border-radius: 999px;
    background: var(--accent);
    color: var(--text-on-accent);
    cursor: pointer;
    font-weight: 600;
    letter-spacing: 0.01em;
    text-decoration: none;
    display: inline-flex;
    align-items: center;
    justify-content: center;
}
.ghost {
    background: var(--surface-strong);
    color: var(--ink);
    border-color: var(--panel-border);
}
button:disabled,
input:disabled,
.button-link[aria-disabled="true"] {
    opacity: 0.55;
    cursor: not-allowed;
}
.pill {
    display: inline-flex;
    align-items: center;
    min-height: 36px;
    padding: 0 14px;
    border-radius: 999px;
    background: var(--chip-bg);
    color: var(--muted);
    font-weight: 600;
    font-size: 0.88rem;
}
.pill.pending {
    background: var(--accent-soft);
    color: var(--accent-strong);
}
.pill.saved {
    background: var(--success-soft);
    color: var(--success);
}
.pill.error {
    background: var(--danger-soft);
    color: var(--danger);
}
.layout {
    width: 100%;
}
.panel {
    box-sizing: border-box;
    padding: 24px;
    border-radius: 32px;
    border: 1px solid var(--panel-border);
    background: var(--surface-high);
    box-shadow: var(--shadow);
}
.hero-title {
    margin: 0 0 10px;
    font-size: clamp(1.8rem, 4vw, 2.5rem);
    line-height: 1.05;
    letter-spacing: -0.04em;
}
.hero-copy,
.panel-note,
.status-detail,
.field-help {
    color: var(--muted);
    line-height: 1.6;
}
.panel-note,
.status-card {
    margin-top: 18px;
    padding: 16px 18px;
    border-radius: 24px;
    border: 1px solid transparent;
    background: var(--surface-strong);
}
.status-card.saved {
    border-color: var(--success-border);
    background: var(--success-soft);
}
.status-card.error {
    border-color: var(--danger-border);
    background: var(--danger-soft);
}
.status-card.pending {
    border-color: var(--accent-border);
    background: var(--accent-ring);
}
.status-row {
    display: flex;
    align-items: center;
    justify-content: space-between;
    gap: 12px;
    flex-wrap: wrap;
}
.status-detail {
    margin: 12px 0 0;
    font-size: 0.92rem;
}
.form-stack {
    display: grid;
    gap: 18px;
}
.field {
    display: grid;
    gap: 10px;
}
.field label {
    font-weight: 600;
}
input[type="file"] {
    width: 100%;
    min-height: 60px;
    padding: 12px 14px;
    border-radius: 20px;
    border: 1px solid var(--panel-border);
    background: var(--surface-strong);
    color: var(--ink);
}
input[type="file"]::file-selector-button {
    margin-right: 14px;
    min-height: 36px;
    padding: 0 16px;
    border: 1px solid transparent;
    border-radius: 999px;
    background: var(--accent-soft);
    color: var(--accent-strong);
    font-weight: 600;
    cursor: pointer;
}
.file-meta {
    min-height: 1.4em;
    color: var(--muted);
    font-size: 0.9rem;
}
.progress-block {
    display: grid;
    gap: 10px;
}
.progress-header {
    display: flex;
    align-items: center;
    justify-content: space-between;
    gap: 12px;
    font-weight: 600;
}
.progress-value {
    color: var(--muted);
    font-family: var(--mono);
    font-size: 0.92rem;
    font-variant-numeric: tabular-nums;
}
.progress-track {
    height: 12px;
    border-radius: 999px;
    overflow: hidden;
    background: var(--chip-bg);
}
.progress-fill {
    width: 0;
    height: 100%;
    border-radius: inherit;
    background: linear-gradient(90deg, var(--accent), var(--accent-strong));
    transition: width 180ms ease;
}
.button-row {
    display: flex;
    flex-wrap: wrap;
    gap: 12px;
}
.guide-grid {
    display: grid;
    gap: 12px;
    margin-top: 18px;
}
.guide-card {
    padding: 16px 18px;
    border-radius: 22px;
    background: var(--surface-strong);
}
.guide-card strong {
    display: block;
    margin-bottom: 6px;
    font-size: 0.95rem;
}
.guide-card p {
    margin: 0;
    color: var(--muted);
    line-height: 1.55;
    font-size: 0.9rem;
}
@media (max-width: 900px) {
    .topbar {
        flex-direction: column;
        align-items: flex-start;
    }
    .top-actions {
        width: 100%;
        justify-content: flex-start;
    }
}
@media (max-width: 640px) {
    .app {
        padding: 16px 14px 28px;
    }
    .topbar,
    .panel {
        border-radius: 24px;
        padding: 18px;
    }
    .button-row > * {
        flex: 1 1 180px;
    }
}
</style>
</head>
<body>
<div class="app">
    <header class="topbar">
        <div class="brand">
            <div class="brand-mark" aria-hidden="true">
                <svg viewBox="0 0 24 24">
                    <path fill="currentColor" opacity="0.45" d="M12 3.2 20.2 8 12 12.8 3.8 8Z"></path>
                    <path fill="currentColor" opacity="0.72" d="M3.8 8v8L12 20.8V12.8Z"></path>
                    <path fill="currentColor" d="M20.2 8v8L12 20.8V12.8Z"></path>
                </svg>
            </div>
            <div>
                <h1>SugarCube</h1>
                <small id="deviceMeta">Loading firmware metadata…</small>
            </div>
        </div>
        <div class="top-actions">
            <div class="mode-switch" role="group" aria-label="Interface theme">
                <button class="mode-button" type="button" data-ui-theme-button="light" onclick="setUiTheme('light')">Light</button>
                <button class="mode-button" type="button" data-ui-theme-button="dark" onclick="setUiTheme('dark')">Dark</button>
            </div>
            <a class="button-link ghost" href="/">Back to dashboard</a>
        </div>
    </header>

    <main class="layout">
        <section class="panel">
            <h2 class="hero-title">SugarCube OTA</h2>
            <p class="hero-copy">Upload a new firmware image over Wi-Fi.</p>

            <div id="statusCard" class="status-card">
                <div class="status-row">
                    <span id="statusPill" class="pill">Ready</span>
                    <span id="progressText" class="progress-value">0%</span>
                </div>
                <p id="statusDetail" class="status-detail">Choose a firmware image to start the upload.</p>
            </div>

            <form id="otaForm" class="form-stack">
                <div class="field">
                    <label for="firmwareFile">Firmware image</label>
                    <input id="firmwareFile" name="update" type="file" accept=".bin,application/octet-stream">
                    <div id="selectedFile" class="file-meta">No file selected.</div>
                    <div class="field-help">Recommended file: <code>firmware.bin</code></div>
                </div>

                <div class="progress-block">
                    <div class="progress-header">
                        <span>Upload progress</span>
                        <span id="progressLabel">0%</span>
                    </div>
                    <div class="progress-track" aria-hidden="true">
                        <div id="progressFill" class="progress-fill"></div>
                    </div>
                </div>

                <div class="button-row">
                    <button id="uploadButton" type="submit">Upload firmware</button>
                    <a class="button-link ghost" href="/">Cancel</a>
                </div>
            </form>

            <div class="panel-note">
                If the upload fails, keep this page open and try again after the device is fully back online.
            </div>
        </section>
    </main>
</div>

<script>
const uiThemeStorageKey = "smartclock-ui-theme";
let uploadInFlight = false;

function currentUiTheme() {
    return document.documentElement.dataset.uiTheme === "dark" ? "dark" : "light";
}

function refreshUiThemeButtons() {
    const activeTheme = currentUiTheme();
    document.querySelectorAll("[data-ui-theme-button]").forEach((button) => {
        const isActive = button.dataset.uiThemeButton === activeTheme;
        button.classList.toggle("active", isActive);
        button.setAttribute("aria-pressed", isActive ? "true" : "false");
    });
}

function setUiTheme(theme) {
    const nextTheme = theme === "dark" ? "dark" : "light";
    document.documentElement.dataset.uiTheme = nextTheme;
    refreshUiThemeButtons();

    try {
        localStorage.setItem(uiThemeStorageKey, nextTheme);
    } catch (error) {
        console.warn("Theme preference was not persisted.", error);
    }
}

function formatBytes(bytes) {
    if (!Number.isFinite(bytes) || bytes <= 0) {
        return "--";
    }
    if (bytes < 1024) {
        return `${bytes} B`;
    }
    if (bytes < 1024 * 1024) {
        return `${(bytes / 1024).toFixed(1)} KB`;
    }
    return `${(bytes / (1024 * 1024)).toFixed(2)} MB`;
}

function setUploadBusy(busy) {
    uploadInFlight = Boolean(busy);
    document.getElementById("firmwareFile").disabled = uploadInFlight;
    document.getElementById("uploadButton").disabled = uploadInFlight;
}

function updateProgress(percent) {
    const safePercent = Math.max(0, Math.min(100, Math.round(percent)));
    document.getElementById("progressFill").style.width = `${safePercent}%`;
    document.getElementById("progressLabel").textContent = `${safePercent}%`;
    document.getElementById("progressText").textContent = `${safePercent}%`;
}

function setStatus(label, tone = "", detail = "") {
    const pill = document.getElementById("statusPill");
    const card = document.getElementById("statusCard");
    pill.textContent = label;
    pill.className = `pill ${tone}`.trim();
    card.className = `status-card ${tone}`.trim();
    document.getElementById("statusDetail").textContent = detail;
}

function updateSelectedFile() {
    const input = document.getElementById("firmwareFile");
    const file = input.files && input.files[0];
    document.getElementById("selectedFile").textContent = file
        ? `${file.name} · ${formatBytes(file.size)}`
        : "No file selected.";
}

async function loadDeviceMeta() {
    const meta = document.getElementById("deviceMeta");

    try {
        const response = await fetch("/version.json");
        if (!response.ok) {
            throw new Error("Version request failed");
        }

        const payload = await response.json();
        if (payload.deviceName && payload.hostName) {
            meta.textContent = `${payload.deviceName} · ${payload.hostName}.local`;
            return;
        }

        meta.textContent = payload.version ? `${payload.version} · OTA ready` : "OTA ready";
    } catch (error) {
        console.error(error);
        meta.textContent = "Firmware metadata unavailable";
    }
}

function uploadFirmware(file) {
    return new Promise((resolve, reject) => {
        const xhr = new XMLHttpRequest();
        xhr.open("POST", "/update");
        xhr.timeout = 180000;

        xhr.upload.addEventListener("progress", (event) => {
            if (!event.lengthComputable) {
                return;
            }

            updateProgress((event.loaded / event.total) * 100);
        });

        xhr.addEventListener("load", () => {
            const responseText = (xhr.responseText || "").trim();
            if (xhr.status >= 200 && xhr.status < 300) {
                resolve(responseText || "OK");
                return;
            }

            reject(new Error(responseText || `Upload failed (HTTP ${xhr.status})`));
        });

        xhr.addEventListener("error", () => {
            reject(new Error("Network error during firmware upload"));
        });

        xhr.addEventListener("timeout", () => {
            reject(new Error("Upload timed out"));
        });

        const formData = new FormData();
        formData.append("update", file, file.name);
        xhr.send(formData);
    });
}

document.getElementById("firmwareFile").addEventListener("change", updateSelectedFile);

document.getElementById("otaForm").addEventListener("submit", async (event) => {
    event.preventDefault();

    if (uploadInFlight) {
        return;
    }

    const input = document.getElementById("firmwareFile");
    const file = input.files && input.files[0];
    if (!file) {
        setStatus("Pick firmware", "error", "Choose a .bin firmware image first.");
        updateProgress(0);
        return;
    }

    setUploadBusy(true);
    updateProgress(0);
    setStatus("Uploading", "pending", "Firmware upload started. Keep this page open until the device confirms the reboot.");

    try {
        const result = await uploadFirmware(file);
        updateProgress(100);
        setStatus("Rebooting", "saved", result === "OK - Rebooting..."
            ? "Firmware uploaded. The device is restarting now."
            : result);
    } catch (error) {
        console.error(error);
        setStatus("Upload failed", "error", error.message || "Firmware upload failed.");
        setUploadBusy(false);
        return;
    }

    window.setTimeout(() => {
        setUploadBusy(false);
    }, 12000);
});

window.addEventListener("load", () => {
    refreshUiThemeButtons();
    updateSelectedFile();
    loadDeviceMeta();
});
</script>
</body>
</html>

)rawliteral";

#endif
