"use strict";

const STATUSES = [
  { key: "Pending", label: "To do", color: "var(--s-pending)" },
  { key: "In Progress", label: "In progress", color: "var(--s-progress)" },
  { key: "Blocked", label: "Blocked", color: "var(--s-blocked)" },
  { key: "Failed", label: "Failed", color: "var(--s-failed)" },
  { key: "Complete", label: "Complete", color: "var(--s-complete)" },
  { key: "Skipped", label: "Skipped", color: "var(--s-skipped)" },
  { key: "Cancelled", label: "Cancelled", color: "var(--s-cancelled)" },
];
const STATUS_COLOR = Object.fromEntries(STATUSES.map((s) => [s.key, s.color]));
const AVATAR_COLORS = ["#7b68ee", "#ff5fa2", "#48b1ff", "#22c55e", "#f59e0b", "#14b8a6", "#ef4444", "#8b5cf6"];

const state = {
  definitions: [],
  actions: [],
  projects: [],
  currentId: null,
  snap: null,
  view: "board",
  selectedTaskId: null,
};

const $ = (sel) => document.querySelector(sel);
function esc(s) {
  return String(s == null ? "" : s).replace(/[&<>"']/g, (c) => ({ "&": "&amp;", "<": "&lt;", ">": "&gt;", '"': "&quot;", "'": "&#39;" }[c]));
}
function initials(name) {
  return (name || "?").split(/\s+/).filter(Boolean).slice(0, 2).map((p) => p[0].toUpperCase()).join("");
}
function avatar(name) {
  let h = 0;
  for (const ch of name || "") h = (h * 31 + ch.charCodeAt(0)) >>> 0;
  return `<span class="avatar" style="background:${AVATAR_COLORS[h % AVATAR_COLORS.length]}">${esc(initials(name))}</span>`;
}
function statusPill(status) {
  return `<span class="status-pill" style="background:${STATUS_COLOR[status] || "#999"}">${esc(status)}</span>`;
}
function toast(message, isError) {
  const el = document.createElement("div");
  el.className = "toast" + (isError ? " error" : "");
  el.textContent = message;
  const box = $("#toasts");
  box.appendChild(el);
  while (box.children.length > 3) box.firstChild.remove();
  setTimeout(() => el.remove(), isError ? 6000 : 2200);
}
function parseValue(text) {
  const t = text.trim();
  if (t === "true") return true;
  if (t === "false") return false;
  if (t !== "" && !isNaN(Number(t))) return Number(t);
  return t;
}

// deadline format: "YYYY-MM-DD HH:MM", empty if none
const FINISHED = ["Complete", "Skipped", "Cancelled"];
function parseDeadline(text) {
  const m = /^(\d{4})-(\d{2})-(\d{2})(?: (\d{2}):(\d{2}))?$/.exec(text || "");
  if (!m || m[1] === "0000") return null;
  return new Date(+m[1], +m[2] - 1, +m[3], +(m[4] || 0), +(m[5] || 0));
}
function isOverdue(node) {
  const due = parseDeadline(node.deadline);
  return !!due && due.getTime() < Date.now() && !FINISHED.includes(node.status);
}
function dueChip(node) {
  if (!parseDeadline(node.deadline)) return "";
  const late = isOverdue(node);
  return `<span class="tag ${late ? "overdue" : "due"}" title="Deadline">${late ? "⚠ Overdue · " : "📅 "}${esc(node.deadline)}</span>`;
}

async function api(method, path, body) {
  const res = await fetch(path, {
    method,
    headers: { "Content-Type": "application/json" },
    body: body === undefined ? undefined : JSON.stringify(body),
  });
  const data = await res.json().catch(() => ({ error: "Bad response from server" }));
  if (res.status === 404 && /^No route for /.test(data.error || "")) {
    // stale binary after an update
    throw new Error("The engine is older than this page. Stop tividy_gui (Ctrl+C) and run `make run` again.");
  }
  if (!res.ok) throw new Error(data.error || res.statusText);
  return data;
}

async function mutate(method, path, body, successMessage) {
  try {
    const snap = await api(method, path, body);
    applySnapshot(snap);
    if (successMessage) toast(typeof successMessage === "function" ? successMessage(snap) : successMessage);
    await loadProjects();
    return snap;
  } catch (e) {
    toast(e.message, true);
    if (state.currentId) await refresh(); // pick up the REJECTED log entry
    return null;
  }
}

function flatten(project) {
  const tasks = [];
  const stages = [];
  const byId = {};
  (function walk(node, stage, depth) {
    byId[node.id] = node;
    for (const child of node.workItems || []) {
      if (child.type === "Project") {
        const info = { node: child, depth, done: 0, total: 0 };
        stages.push(info);
        byId[child.id] = child;
        walk(child, child, depth + 1);
      } else {
        tasks.push({ node: child, stage, depth });
        byId[child.id] = child;
      }
    }
  })(project, null, 0);
  for (const s of stages) {
    const leaves = tasks.filter((t) => t.stage === s.node);
    s.total = leaves.length;
    s.done = leaves.filter((t) => t.node.status === "Complete" || t.node.status === "Skipped").length;
  }
  return { tasks, stages, byId };
}

async function loadStatic() {
  state.definitions = await api("GET", "/api/definitions");
  state.actions = await api("GET", "/api/actions");
  renderDefinitions();
}
async function loadProjects() {
  state.projects = await api("GET", "/api/projects");
  renderProjects();
}
async function refresh() {
  if (!state.currentId) return;
  try {
    applySnapshot(await api("GET", `/api/projects/${state.currentId}`));
  } catch (e) {
    toast(e.message, true);
  }
}
function applySnapshot(snap) {
  state.snap = snap;
  state.currentId = snap.project.id;
  render();
}
async function openProject(id) {
  state.currentId = id;
  state.selectedTaskId = null;
  closeDrawer();
  await refresh();
  renderProjects();
}

function renderDefinitions() {
  const side = state.definitions.map((d) => `
    <button class="side-item" data-def="${esc(d.id)}" title="${esc(d.description)}">
      <span class="name">${esc(d.name)}</span>
      <div class="row"><span class="sub">▶ Start new instance</span><span class="industry">${esc(d.industry)}</span></div>
    </button>`).join("");
  $("#definitions").innerHTML = side;
  $("#welcomeDefinitions").innerHTML = state.definitions.map((d) => `
    <div class="def-card">
      <span class="industry ${esc(d.industry)}">${esc(d.industry)}</span>
      <h3>${esc(d.name)}</h3>
      <div class="muted">${esc(d.description)}</div>
      <div><button class="btn primary small" data-def="${esc(d.id)}">Start instance</button></div>
    </div>`).join("");
  document.querySelectorAll("[data-def]").forEach((b) =>
    b.addEventListener("click", () => startInstanceDialog(b.dataset.def)));
}

function renderProjects() {
  if (!state.projects.length) {
    $("#projects").innerHTML = `<div class="sub" style="padding:0 8px;opacity:.6">No projects yet</div>`;
    return;
  }
  $("#projects").innerHTML = state.projects.map((p) => {
    const pct = p.total ? Math.round((100 * p.done) / p.total) : 100;
    return `
      <button class="side-item ${p.id === state.currentId ? "active" : ""}" data-project="${esc(p.id)}">
        <div class="row"><span class="name">${esc(p.name)}</span><span class="sub">${esc(p.id)}</span></div>
        <div class="row"><span class="sub">${p.industry ? esc(p.industry) + " · " : ""}${p.done}/${p.total}${p.completed ? " ✓" : ""}</span></div>
        <div class="mini-progress"><div style="width:${pct}%"></div></div>
      </button>`;
  }).join("");
  document.querySelectorAll("[data-project]").forEach((b) =>
    b.addEventListener("click", () => openProject(b.dataset.project)));
}

function render() {
  const snap = state.snap;
  $("#welcome").hidden = !!snap;
  $("#projectView").hidden = !snap;
  if (!snap) return;
  const p = snap.project;
  const flat = flatten(p);
  const done = flat.tasks.filter((t) => t.node.status === "Complete" || t.node.status === "Skipped").length;
  const total = flat.tasks.length;
  const industry = (p.customFields || {}).industry;
  $("#crumbs").innerHTML = `${industry ? `<span class="industry ${esc(industry)}">${esc(industry)}</span>` : ""}
    <span>${esc(p.id)}</span>${p.customFields && p.customFields.clonedFrom ? `<span>· cloned from ${esc(p.customFields.clonedFrom)}</span>` : ""}
    ${p.completed && total ? `<span class="tag ready">✓ Completed</span>` : ""}`;
  $("#projectTitle").textContent = p.name;
  $("#progressFill").style.width = (total ? (100 * done) / total : 0) + "%";
  $("#progressText").textContent = `${done}/${total} tasks done`;
  $("#undoBtn").disabled = !snap.canUndo;
  $("#redoBtn").disabled = !snap.canRedo;
  document.querySelectorAll("#tabs button").forEach((b) => b.classList.toggle("active", b.dataset.view === state.view));

  $("#stages").innerHTML = flat.stages.map((s) => {
    const cls = s.node.completed ? "done" : s.node.ready ? "" : "waiting";
    return `<span class="stage-chip ${cls}" title="Stage (nested project)"><span class="dot"></span>${esc(s.node.name)}
      <span class="muted">${s.done}/${s.total}${s.node.ready ? "" : " · waiting"}</span></span>`;
  }).join("");

  const views = { board: renderBoard, list: renderList, deps: renderGraph, work: renderWork, history: renderHistory, json: renderJson };
  views[state.view](flat);
  if (state.selectedTaskId) renderDrawer(flat);
}

const ACTION_ORDER = ["start", "complete", "block", "fail", "cancel"];
const ACTION_LABELS = { start: "Start", complete: "Complete", block: "Block", fail: "Fail", cancel: "Cancel" };
function actionLabel(a) { return ACTION_LABELS[a] || a.charAt(0).toUpperCase() + a.slice(1); }
function orderedActions() {
  const rank = (a) => { const i = ACTION_ORDER.indexOf(a); return i < 0 ? ACTION_ORDER.length : i; };
  return state.actions.slice().sort((a, b) => rank(a) - rank(b));
}

function primaryActions(node) {
  const isDecision = node.type === "Decision";
  switch (node.status) {
    case "Pending": return [{ action: "start", label: "Start", disabled: !node.ready }];
    case "In Progress": return isDecision ? [{ decide: true, label: "Decide…" }] : [{ action: "complete", label: "Complete" }];
    case "Blocked": return [{ action: "start", label: "Resume" }];
    case "Failed": return [{ action: "start", label: "Retry", disabled: !node.ready }];
    case "Complete": return [{ action: "start", label: "Reopen" }];
    default: return [];
  }
}

function actionControl(n) {
  if (n.status === "Skipped" || n.status === "Cancelled") return `<span class="muted">${esc(n.status)}</span>`;
  const a = primaryActions(n)[0];
  const main = !a ? "" : a.decide
    ? `<button class="btn primary small" data-decide="${esc(n.id)}">${a.label}</button>`
    : `<button class="btn primary small" data-act="${a.action}" data-task="${esc(n.id)}" ${a.disabled ? "disabled title='Prerequisites not satisfied'" : ""}>${a.label}</button>`;
  return `<span class="split">${main}<span class="menu-wrap"><button class="btn primary small caret" data-state-menu="${esc(n.id)}" title="Change state">▾</button></span></span>`;
}

function depState(p) {
  if (p.status === "Failed") return "failed";
  if (p.status === "Complete" || p.status === "Skipped" || p.completed) return "done";
  return "";
}

function depChips(n, flat) {
  const pre = (n.dependsOn || []).map((id) => flat.byId[id]).filter(Boolean);
  if (!pre.length) return "";
  const chip = (p) => `<span class="dep ${depState(p)}" title="${esc(p.name)} · ${esc(p.status || (p.completed ? "Complete" : "Open"))}"><i style="background:${STATUS_COLOR[p.status] || "#999"}"></i>${esc(p.name)}</span>`;
  return `<div class="card-deps"><span class="deps-label">⛓ Waits for</span>${pre.slice(0, 3).map(chip).join("")}${pre.length > 3 ? `<span class="dep more" title="${esc(pre.slice(3).map((p) => p.name).join(", "))}">+${pre.length - 3}</span>` : ""}</div>`;
}

function waitingOn(node, byId) {
  return (node.dependsOn || []).map((id) => byId[id]).filter((d) => d && !(d.completed || d.status === "Complete" || d.status === "Skipped"));
}

function cardHtml(t, flat) {
  const n = t.node;
  const exec = n.executor ? n.executor.name : "";
  const blockers = waitingOn(n, flat.byId);
  const stageWaiting = t.stage && !t.stage.ready;
  const unlocks = flat.tasks.map((x) => x.node).filter((x) => (x.dependsOn || []).includes(n.id));
  let readyTag = "";
  if (n.status === "Pending") {
    readyTag = n.ready ? `<span class="tag ready">Ready</span>`
      : `<span class="tag waiting" title="Waiting for: ${esc(blockers.map((b) => b.name).join(", ") || (t.stage ? t.stage.name : ""))}">🔒 ${stageWaiting && !blockers.length ? "Stage waiting" : "Waiting on " + blockers.length}</span>`;
  }
  const upstream = (n.failedBecause || []).map((id) => (flat.byId[id] || {}).name || id);
  const failedTag = upstream.length ? `<span class="tag failed-up" title="Failed because of: ${esc(upstream.join(", "))}">⛓ Failed upstream: ${esc(upstream.join(", "))}</span>` : "";
  const unlocksTag = unlocks.length ? `<span class="tag" title="Waiting for this task: ${esc(unlocks.map((u) => u.name).join(", "))}">→ unlocks ${unlocks.length}</span>` : "";
  return `
    <div class="card ${state.selectedTaskId === n.id ? "selected" : ""}" data-card="${esc(n.id)}" style="border-left-color:${STATUS_COLOR[n.status]}">
      <div class="card-head">
        <div class="card-title">
          <div class="card-name">${n.type === "Decision" ? "◆ " : ""}${esc(n.name)}</div>
          <div class="card-id">${esc(n.id)}</div>
        </div>
        <div class="menu-wrap"><button class="btn small ghost" data-menu="${esc(n.id)}" title="Duplicate or delete">⋯</button></div>
      </div>
      <div class="card-desc">${n.description ? esc(n.description) : '<span class="muted">No description</span>'}</div>
      <div class="card-tags">
        ${n.type === "Decision" ? `<span class="tag decision">Decision</span>` : ""}
        ${n.selectedBranch ? `<span class="tag branch">→ ${esc(n.selectedBranch)}</span>` : ""}
        ${t.stage ? `<span class="tag">${esc(t.stage.name)}</span>` : ""}
        ${readyTag}
        ${failedTag}
        ${unlocksTag}
      </div>
      ${depChips(n, flat)}
      <div class="card-row">
        <span class="assignee">${exec ? avatar(exec) + `<span>${esc(exec)}</span>` : `<span class="muted">Unassigned</span>`}</span>
        <span class="card-actions">${actionControl(n)}</span>
      </div>
      <div class="card-deadline">${dueChip(n) || '<span class="muted">No deadline</span>'}</div>
    </div>`;
}

function renderBoard(flat) {
  const cols = STATUSES.filter((s) => s.key !== "Cancelled" || flat.tasks.some((t) => t.node.status === "Cancelled"));
  $("#view").innerHTML = `<div class="board">${cols.map((s) => {
    const items = flat.tasks.filter((t) => t.node.status === s.key);
    return `
      <div class="column${items.length || s.key === "Pending" ? "" : " empty"}">
        <div class="column-head"><span class="pill" style="background:${s.color}">${s.label}</span><span class="count">${items.length}</span></div>
        <div class="column-body">
          ${items.map((t) => cardHtml(t, flat)).join("") || `<div class="column-empty">No tasks</div>`}
          ${s.key === "Pending" ? `<button class="btn ghost small" id="colAddTask">+ Add task</button>` : ""}
        </div>
      </div>`;
  }).join("")}</div>`;
  const add = $("#colAddTask");
  if (add) add.addEventListener("click", addTaskDialog);
  wireCards();
}

function renderList(flat) {
  const rows = [];
  (function walk(node, depth) {
    for (const c of node.workItems || []) {
      if (c.type === "Project") {
        rows.push(`<tr class="stage-row"><td style="padding-left:${12 + depth * 22}px">▾ ${esc(c.name)}</td><td>${c.completed ? statusPill("Complete") : `<span class="muted">${c.ready ? "Open" : "Waiting"}</span>`}</td><td></td><td></td><td></td></tr>`);
        walk(c, depth + 1);
      } else {
        rows.push(`<tr class="clickable" data-card="${esc(c.id)}">
          <td style="padding-left:${12 + depth * 22}px">${c.type === "Decision" ? "◆ " : ""}${esc(c.name)} <span class="muted">${esc(c.id)}</span></td>
          <td>${statusPill(c.status)}</td>
          <td>${c.executor ? avatar(c.executor.name) + " " + esc(c.executor.name) : '<span class="muted">—</span>'}</td>
          <td>${c.ready ? "✓" : "🔒"}</td>
          <td>${c.deadline ? `<span class="${isOverdue(c) ? "overdue-text" : ""}">${esc(c.deadline)}</span>` : '<span class="muted">—</span>'}</td></tr>`);
      }
    }
  })(state.snap.project, 0);
  $("#view").innerHTML = `<table class="table"><thead><tr><th>Task</th><th>Status</th><th>Assignee</th><th>Ready</th><th>Deadline</th></tr></thead>
    <tbody>${rows.join("") || `<tr><td colspan="5" class="muted">No tasks yet</td></tr>`}</tbody></table>`;
  wireCards();
}

function renderWork(flat) {
  const work = state.snap.work;
  if (!work.length) {
    $("#view").innerHTML = `<div class="empty-state">🎉 Nothing is waiting for action in this project.</div>`;
    return;
  }
  $("#view").innerHTML = `<div class="board" style="flex-direction:column;align-items:stretch;max-width:760px">
    ${work.map((w) => cardHtml({ node: flat.byId[w.id], stage: findStage(flat, w.id) }, flat)).join("")}</div>`;
  wireCards();
}
function findStage(flat, id) {
  const t = flat.tasks.find((x) => x.node.id === id);
  return t ? t.stage : null;
}

function renderHistory() {
  const lines = state.snap.history.slice().reverse();
  if (!lines.length) {
    $("#view").innerHTML = `<div class="empty-state">No actions yet. Every action, undo and rejection is recorded here.</div>`;
    return;
  }
  $("#view").innerHTML = `<div class="timeline">${lines.map((l) => {
    // e.g. "[On 2026-11-01 14:25]: ExecuteCommand\nActor: P-2\nDetails:Start task T-1"
    const m = l.match(/^\[(?:On )?(.*?)\]:?\s*(\w+)\s*\nActor:\s*(.*?)\s*\nDetails:\s*([\s\S]*)$/);
    const time = m ? m[1] : "";
    const kind = m ? m[2].replace(/Command$/, "").toUpperCase() : "";
    const text = m ? m[4] : l;
    return `<div class="event"><span class="kind ${esc(kind)}">${esc(kind || "EVENT")}</span><span>${esc(text)}</span><span class="time" style="margin-left:auto">${esc(time)}</span></div>`;
  }).join("")}</div>`;
}

function renderJson() {
  $("#view").innerHTML = `<pre class="json">${esc(JSON.stringify(state.snap.project, null, 2))}</pre>`;
}

function wireCards(root) {
  root = root || $("#view");
  root.querySelectorAll("[data-act]").forEach((b) => b.addEventListener("click", (e) => {
    e.stopPropagation();
    runAction(b.dataset.task, b.dataset.act);
  }));
  root.querySelectorAll("[data-decide]").forEach((b) => b.addEventListener("click", (e) => {
    e.stopPropagation();
    decideDialog(b.dataset.decide);
  }));
  root.querySelectorAll("[data-menu]").forEach((b) => b.addEventListener("click", (e) => {
    e.stopPropagation();
    openMoreMenu(b, b.dataset.menu);
  }));
  root.querySelectorAll("[data-state-menu]").forEach((b) => b.addEventListener("click", (e) => {
    e.stopPropagation();
    openStateMenu(b, b.dataset.stateMenu);
  }));
  root.querySelectorAll("[data-card]").forEach((c) => c.addEventListener("click", () => {
    state.selectedTaskId = c.dataset.card;
    render();
  }));
}

function openMenu(button, items) {
  closeMenus();
  const menu = document.createElement("div");
  menu.className = "menu";
  items.forEach((it) => {
    const b = document.createElement("button");
    b.textContent = it.label;
    if (it.danger) b.classList.add("danger");
    Object.entries(it.attrs || {}).forEach(([k, v]) => b.setAttribute(k, v));
    b.addEventListener("click", (e) => { e.stopPropagation(); closeMenus(); it.run(); });
    menu.appendChild(b);
  });
  document.body.appendChild(menu);
  const r = button.getBoundingClientRect();
  const w = menu.offsetWidth;
  const h = menu.offsetHeight;
  let top = r.bottom + 4;
  if (top + h + 8 > window.innerHeight) top = r.top - h - 4; // flip up if no room
  menu.style.left = Math.max(8, Math.min(window.innerWidth - w - 8, r.right - w)) + "px";
  menu.style.top = Math.max(8, Math.min(window.innerHeight - h - 8, top)) + "px";
}
function closeMenus() { document.querySelectorAll(".menu").forEach((m) => m.remove()); }

function openStateMenu(button, taskId) {
  const node = flatten(state.snap.project).byId[taskId];
  const items = orderedActions().map((a) => ({ label: actionLabel(a), attrs: { "data-menu-act": a }, run: () => runAction(taskId, a) }));
  if (node.type === "Decision") items.unshift({ label: "Decide…", attrs: { "data-menu-decide": "1" }, run: () => decideDialog(taskId) });
  openMenu(button, items);
}

function openMoreMenu(button, taskId) {
  openMenu(button, [
    { label: "Duplicate", attrs: { "data-menu-duplicate": "1" }, run: () => duplicateTask(taskId) },
    { label: "Delete…", attrs: { "data-menu-delete": "1" }, danger: true, run: () => deleteDialog(taskId) },
  ]);
}

function duplicateTask(taskId) {
  const node = flatten(state.snap.project).byId[taskId];
  return mutate("POST", `/api/projects/${state.currentId}/tasks/${taskId}/duplicate`, {}, `Duplicated ${node ? node.name : taskId}`);
}

function deleteDialog(taskId) {
  const flat = flatten(state.snap.project);
  const n = flat.byId[taskId];
  if (!n) return;
  const waiting = flat.tasks.map((x) => x.node).filter((x) => (x.dependsOn || []).includes(taskId));
  modal(`<h2>Delete “${esc(n.name)}”?</h2>
    <p>${waiting.length ? `Tasks waiting for it will be released: ${esc(waiting.map((w) => w.name).join(", "))}.` : "Nothing is waiting for this task."} You can undo this.</p>
    <div class="actions"><button type="button" class="btn" data-close>Cancel</button><button type="button" class="btn danger" id="confirmDelete">Delete</button></div>`, () => {
    $("#confirmDelete").addEventListener("click", async () => {
      closeModal();
      await mutate("POST", `/api/projects/${state.currentId}/tasks/${taskId}/delete`, {}, `Deleted ${n.name}`);
    });
  });
}

function runAction(taskId, action) {
  const node = flatten(state.snap.project).byId[taskId];
  return mutate("POST", `/api/projects/${state.currentId}/tasks/${taskId}/actions`, { action },
    `${action} → ${node ? node.name : taskId}`);
}

function clip(text, max) { return text.length > max ? text.slice(0, max - 1) + "…" : text; }

function renderGraph(flat) {
  const tasks = flat.tasks.map((t) => t.node);
  if (!tasks.length) {
    $("#view").innerHTML = `<div class="empty-state">No tasks yet.</div>`;
    return;
  }
  const byId = {};
  tasks.forEach((t) => { byId[t.id] = t; });

  const level = {};
  const depth = (t) => {
    if (level[t.id] !== undefined) return level[t.id];
    level[t.id] = 0; // guards against cycles
    let l = 0;
    (t.dependsOn || []).forEach((id) => { if (byId[id]) l = Math.max(l, depth(byId[id]) + 1); });
    level[t.id] = l;
    return l;
  };
  tasks.forEach(depth);

  const columns = [];
  tasks.forEach((t) => { (columns[level[t.id]] = columns[level[t.id]] || []).push(t); });
  const W = 214, H = 62, GX = 100, GY = 22, PAD = 22;
  const tallest = Math.max.apply(null, columns.map((c) => (c ? c.length : 0)));
  const pos = {};
  columns.forEach((col, ci) => {
    if (!col) return;
    const offset = ((tallest - col.length) * (H + GY)) / 2;
    col.forEach((t, ri) => { pos[t.id] = { x: PAD + ci * (W + GX), y: PAD + offset + ri * (H + GY) }; });
  });
  const width = PAD * 2 + columns.length * W + (columns.length - 1) * GX;
  const height = PAD * 2 + tallest * H + (tallest - 1) * GY;

  const colorOf = (p) => (p.status === "Failed" ? "#ef4444" : p.status === "Complete" || p.status === "Skipped" ? "#22c55e"
    : p.status === "In Progress" ? "#3b82f6" : "#9aa0b5");
  const colors = ["#ef4444", "#22c55e", "#3b82f6", "#9aa0b5"];
  const defs = colors.map((c) => `<marker id="arrow${c.slice(1)}" viewBox="0 0 10 10" refX="9" refY="5" markerWidth="8" markerHeight="8" orient="auto-start-reverse"><path d="M0,0 L10,5 L0,10 z" fill="${c}"/></marker>`).join("");

  let edgeCount = 0;
  const edges = tasks.map((t) => (t.dependsOn || []).filter((id) => pos[id]).map((id) => {
    const p = byId[id];
    const a = pos[id];
    const b = pos[t.id];
    const x1 = a.x + W, y1 = a.y + H / 2, x2 = b.x, y2 = b.y + H / 2;
    const mid = (x1 + x2) / 2;
    const color = colorOf(p);
    const branch = p.branches ? Object.entries(p.branches).find(([, ids]) => ids.includes(t.id)) : null;
    edgeCount++;
    return `<g class="edge-group" data-edge="${esc(id)}>${esc(t.id)}">
      <path class="edge" d="M${x1},${y1} C${mid},${y1} ${mid},${y2} ${x2},${y2}" stroke="${color}" marker-end="url(#arrow${color.slice(1)})"><title>${esc(p.name)} → ${esc(t.name)}</title></path>
      ${branch ? `<text class="edge-label" x="${mid}" y="${(y1 + y2) / 2 - 5}" text-anchor="middle">${esc(branch[0])}</text>` : ""}
    </g>`;
  }).join("")).join("");

  const nodes = tasks.map((t) => {
    const { x, y } = pos[t.id];
    const who = t.executor ? ` · ${t.executor.name}` : "";
    return `<g class="gnode ${depState(t)} ${state.selectedTaskId === t.id ? "selected" : ""}" data-card="${esc(t.id)}" transform="translate(${x},${y})">
      <title>${esc(t.name)} (${esc(t.id)}) · ${esc(t.status)}</title>
      <rect class="box" width="${W}" height="${H}" rx="10"/>
      <rect class="stripe" width="6" height="${H}" rx="3" style="fill:${STATUS_COLOR[t.status] || "#999"}"/>
      <text class="gname" x="18" y="26">${t.type === "Decision" ? "◆ " : ""}${esc(clip(t.name, 24))}</text>
      <text class="gsub" x="18" y="46">${esc(clip(t.status + who, 32))}</text>
    </g>`;
  }).join("");

  $("#view").innerHTML = `<div class="graph-wrap">
      <div class="graph-note">Arrows run from a prerequisite to the task that waits for it. Green = done, red = failed, blue = in progress, grey = not finished. Click a task for details.
        <span class="muted">${tasks.length} tasks · ${edgeCount} dependencies</span></div>
      <div class="graph"><svg id="depGraph" width="${width}" height="${height}" viewBox="0 0 ${width} ${height}" xmlns="http://www.w3.org/2000/svg"><defs>${defs}</defs>${edges}${nodes}</svg></div>
    </div>`;
  wireCards();
}

function closeDrawer() {
  state.selectedTaskId = null;
  $("#drawer").classList.remove("open");
  document.querySelectorAll(".card.selected").forEach((c) => c.classList.remove("selected"));
}

function renderDrawer(flat) {
  const n = flat.byId[state.selectedTaskId];
  if (!n) return closeDrawer();
  const pid = state.currentId;
  const dependents = flat.tasks.map((t) => t.node).concat(flat.stages.map((s) => s.node))
    .filter((x) => (x.dependsOn || []).includes(n.id));
  const prereqs = (n.dependsOn || []).map((id) => flat.byId[id]).filter(Boolean);
  const candidates = Object.values(flat.byId).filter((x) => x.id !== n.id && x.id !== state.snap.project.id && !(n.dependsOn || []).includes(x.id));
  const fields = Object.entries(n.customFields || {});
  const dl = n.deadline ? n.deadline.split(" ") : ["", ""];
  const branches = n.branches ? Object.entries(n.branches) : [];

  $("#drawer").innerHTML = `
    <div class="drawer-head">
      <button class="drawer-close" id="drawerClose">×</button>
      <div class="crumbs"><span>${esc(n.id)}</span><span>· ${esc(n.type)}</span></div>
      <h2>${esc(n.name)}</h2>
      ${(n.failedBecause || []).length ? `<div class="notice danger">Failed because a prerequisite failed: ${n.failedBecause.map((id) => esc((flat.byId[id] || {}).name || id)).join(", ")}. It returns to To do when that task is retried.</div>` : ""}
      <div class="row-gap">${statusPill(n.status)} ${n.ready ? '<span class="tag ready">Ready</span>' : '<span class="tag waiting">🔒 Waiting</span>'}
        <span style="margin-left:auto" class="card-actions">${primaryActions(n).map((a) => a.decide
          ? `<button class="btn primary small" data-decide="${esc(n.id)}">${a.label}</button>`
          : `<button class="btn primary small" data-act="${a.action}" data-task="${esc(n.id)}" ${a.disabled ? "disabled" : ""}>${a.label}</button>`).join("")}</span></div>
    </div>
    <div class="drawer-body">
      <div>
        <div class="field-label">Details</div>
        <div class="kv">
          <div>Description</div><div>${esc(n.description) || '<span class="muted">—</span>'}</div>
          <div>Assignee</div><div>${n.executor ? avatar(n.executor.name) + " " + esc(n.executor.name) + ` <span class="muted">(${esc(n.executor.role)})</span>` : '<span class="muted">Unassigned</span>'}</div>
          <div>Deadline</div><div>${n.deadline ? dueChip(n) : '<span class="muted">—</span>'}</div>
          ${fields.map(([k, v]) => `<div>${esc(k)}</div><div>${esc(v)}</div>`).join("")}
        </div>
      </div>

      ${branches.length ? `<div><div class="field-label">Branches ${n.selectedBranch ? `· selected <b>${esc(n.selectedBranch)}</b>` : ""}</div>
        <div class="links">${branches.map(([label, ids]) => `<div class="link-item"><b>${esc(label)}</b><span>${ids.map((id) => esc(flat.byId[id] ? flat.byId[id].name : id)).join(", ")}</span></div>`).join("")}</div></div>` : ""}

      <div>
        <div class="field-label">Waits for (prerequisites)</div>
        <div class="links">${prereqs.map((p) => `<div class="link-item"><span>${esc(p.name)} <span class="muted">${esc(p.id)}</span></span>${p.type === "Project" ? (p.completed ? statusPill("Complete") : '<span class="tag">Stage</span>') : statusPill(p.status)}</div>`).join("") || '<span class="muted">Nothing — no prerequisites</span>'}</div>
        <div class="row-gap" style="margin-top:8px">
          <select class="input" id="depSelect"><option value="">Add a prerequisite…</option>
            ${candidates.map((c) => `<option value="${esc(c.id)}">${esc(c.name)} (${esc(c.id)})</option>`).join("")}</select>
          <button class="btn small" id="depAdd">Link</button>
        </div>
      </div>

      <div>
        <div class="field-label">Unlocks (dependents)</div>
        <div class="links">${dependents.map((d) => `<div class="link-item"><span>${esc(d.name)} <span class="muted">${esc(d.id)}</span></span>${d.type === "Project" ? '<span class="tag">Stage</span>' : statusPill(d.status)}</div>`).join("") || '<span class="muted">Nothing depends on this task</span>'}</div>
      </div>

      <div>
        <div class="field-label">Assign</div>
        <div class="form-grid">
          <input class="input" id="asId" placeholder="Person id (e.g. N7)">
          <input class="input" id="asRole" placeholder="Role">
          <input class="input full" id="asName" placeholder="Full name">
          <div class="full"><button class="btn small" id="asSave">Assign</button></div>
        </div>
      </div>

      <div>
        <div class="field-label">Edit (undoable, all-or-nothing)</div>
        <div class="form-grid">
          <input class="input full" id="edName" value="${esc(n.name)}" placeholder="Name">
          <textarea class="input full" id="edDesc" rows="2" placeholder="Description">${esc(n.description)}</textarea>
          <input class="input" type="date" id="edDate" value="${esc(dl[0])}">
          <input class="input" type="time" id="edTime" value="${esc(dl[1] || "")}">
          <div class="full"><button type="button" class="btn small ghost" id="edClearDate" ${n.deadline ? "" : "disabled"}>Clear deadline</button></div>
          <input class="input" id="edKey" placeholder="Custom field">
          <input class="input" id="edVal" placeholder="Value">
          <div class="full"><button class="btn primary small" id="edSave">Save changes</button></div>
        </div>
      </div>
    </div>`;
  $("#drawer").classList.add("open");
  $("#drawerClose").addEventListener("click", closeDrawer);
  wireCards($("#drawer"));

  $("#depAdd").addEventListener("click", () => {
    const pre = $("#depSelect").value;
    if (!pre) return toast("Choose a prerequisite first", true);
    mutate("POST", `/api/projects/${pid}/dependencies`, { dependent: n.id, prerequisite: pre }, "Dependency added");
  });
  $("#asSave").addEventListener("click", () => mutate("POST", `/api/projects/${pid}/tasks/${n.id}/assign`,
    { id: $("#asId").value.trim(), name: $("#asName").value.trim(), role: $("#asRole").value.trim() }, "Assignee updated"));
  let clearDeadline = false;
  $("#edClearDate").addEventListener("click", () => {
    $("#edDate").value = "";
    $("#edTime").value = "";
    clearDeadline = true;
  });
  ["edDate", "edTime"].forEach((id) => $("#" + id).addEventListener("input", () => { clearDeadline = false; }));
  $("#edSave").addEventListener("click", () => {
    const payload = {};
    if ($("#edName").value.trim() !== n.name) payload.name = $("#edName").value.trim();
    if ($("#edDesc").value !== n.description) payload.description = $("#edDesc").value;
    const date = $("#edDate").value;
    if (clearDeadline) {
      if (n.deadline) payload.deadline = null;
    } else if (date) {
      const [y, m, d] = date.split("-").map(Number);
      const [hh, mm] = ($("#edTime").value || "00:00").split(":").map(Number);
      const deadline = { year: y, month: m, day: d, hour: hh, min: mm };
      const formatted = `${date} ${String(hh).padStart(2, "0")}:${String(mm).padStart(2, "0")}`;
      if (formatted !== n.deadline) payload.deadline = deadline;
    } else if ($("#edTime").value) {
      return toast("Choose a date as well as a time for the deadline", true);
    }
    if ($("#edKey").value.trim()) payload.customFields = { [$("#edKey").value.trim()]: $("#edVal").value };
    if (!Object.keys(payload).length) return toast("Nothing changed");
    mutate("POST", `/api/projects/${pid}/tasks/${n.id}/edit`, payload, "Task updated");
  });
}

function modal(html, onOpen) {
  $("#modal").innerHTML = html;
  $("#modalBackdrop").hidden = false;
  const first = $("#modal").querySelector("input, select, textarea");
  if (first) first.focus();
  $("#modal").querySelectorAll("[data-close]").forEach((b) => b.addEventListener("click", closeModal));
  if (onOpen) onOpen();
}
function closeModal() { $("#modalBackdrop").hidden = true; }

function formModal(title, text, fields, submitLabel, onSubmit) {
  modal(`<h2>${esc(title)}</h2><p>${esc(text)}</p>
    <form id="mForm" class="kv-rows">
      ${fields.map((f) => `<input class="input" name="${f.name}" placeholder="${esc(f.placeholder)}" value="${esc(f.value || "")}">`).join("")}
      <div class="actions"><button type="button" class="btn" data-close>Cancel</button><button class="btn primary">${esc(submitLabel)}</button></div>
    </form>`, () => {
    $("#mForm").addEventListener("submit", async (e) => {
      e.preventDefault();
      const values = Object.fromEntries(new FormData(e.target).entries());
      closeModal();
      await onSubmit(values);
    });
  });
}

function startInstanceDialog(defId) {
  const def = state.definitions.find((d) => d.id === defId);
  const count = state.projects.filter((p) => p.industry === def.industry).length + 1;
  const suggestion = `${def.name} ${count}`;
  formModal(`Start: ${def.name}`, "Creates an independent running instance from this definition.",
    [{ name: "name", placeholder: "Instance name", value: suggestion }], "Start", async (v) => {
      const snap = await mutate("POST", "/api/projects", { definitionId: defId, name: v.name }, `Started ${v.name}`);
      if (snap) { state.view = "board"; render(); renderProjects(); }
    });
}

function newProjectDialog() {
  formModal("New empty project", "Build your own workflow by adding tasks and dependencies.",
    [{ name: "name", placeholder: "Project name" }, { name: "description", placeholder: "Description (optional)" }], "Create",
    (v) => mutate("POST", "/api/projects", { name: v.name, description: v.description }, `Created ${v.name}`));
}

function addTaskDialog() {
  formModal("Add task", "Adds a task to this project (undoable).",
    [{ name: "name", placeholder: "Task name" }, { name: "description", placeholder: "Description (optional)" }], "Add",
    (v) => mutate("POST", `/api/projects/${state.currentId}/tasks`, { name: v.name, description: v.description }, `Added ${v.name}`));
}

function cloneDialog() {
  const p = state.snap.project;
  formModal("Clone project", "Prototype: makes an independent copy with fresh ids, Pending tasks and remapped dependencies.",
    [{ name: "name", placeholder: "Name of the copy", value: p.name + " (copy)" }], "Clone",
    (v) => mutate("POST", `/api/projects/${state.currentId}/clone`, { name: v.name }, `Cloned into ${v.name}`));
}

function decideDialog(taskId) {
  const flat = flatten(state.snap.project);
  const n = flat.byId[taskId];
  const hint = (n.customFields || {}).inputs || "";
  const keys = hint ? hint.split(",").map((s) => s.trim().split(/\s+/)[0]).filter(Boolean) : [""];
  const branches = Object.entries(n.branches || {});
  const row = (k) => `<div class="row-gap"><input class="input" data-k placeholder="field" value="${esc(k)}"><input class="input" data-v placeholder="value"></div>`;
  modal(`<h2>◆ ${esc(n.name)}</h2>
    <p>The routing strategy picks a branch from these inputs${hint ? `: <b>${esc(hint)}</b>` : ""}.</p>
    <div class="branch-list">${branches.map(([l, ids]) => `<span class="tag branch">${esc(l)} → ${ids.map((id) => esc(flat.byId[id] ? flat.byId[id].name : id)).join(", ")}</span>`).join("")}</div>
    <form id="dForm" class="kv-rows">
      <div id="dRows" class="kv-rows">${keys.map(row).join("")}</div>
      <div><button type="button" class="btn small ghost" id="dMore">+ field</button></div>
      <div class="actions"><button type="button" class="btn" data-close>Cancel</button><button class="btn primary">Route</button></div>
    </form>`, () => {
    $("#dMore").addEventListener("click", () => $("#dRows").insertAdjacentHTML("beforeend", row("")));
    $("#dForm").addEventListener("submit", async (e) => {
      e.preventDefault();
      const context = {};
      $("#dRows").querySelectorAll(".row-gap").forEach((r) => {
        const k = r.querySelector("[data-k]").value.trim();
        const v = r.querySelector("[data-v]").value;
        if (k && v.trim() !== "") context[k] = parseValue(v);
      });
      closeModal();
      await mutate("POST", `/api/projects/${state.currentId}/tasks/${taskId}/decide`, { context }, (s) => `Routed to “${s.branch}”`);
    });
  });
}

function downloadText(filename, text) {
  const url = URL.createObjectURL(new Blob([text], { type: "application/json" }));
  const a = document.createElement("a");
  a.href = url;
  a.download = filename;
  document.body.appendChild(a);
  a.click();
  a.remove();
  setTimeout(() => URL.revokeObjectURL(url), 1000);
}

async function exportProject() {
  if (!state.currentId) return;
  try {
    const data = await api("GET", `/api/projects/${state.currentId}/export`);
    const slug = (data.project.name || "project").toLowerCase().replace(/[^a-z0-9]+/g, "-").replace(/^-+|-+$/g, "") || "project";
    downloadText(`${slug}.tividy.json`, JSON.stringify(data, null, 2));
    toast(`Exported ${data.project.name}`);
  } catch (e) {
    toast(e.message, true);
  }
}

async function importFromFile(file) {
  if (!file) return;
  let data;
  try {
    data = JSON.parse(await file.text());
  } catch (e) {
    toast(`${file.name} is not valid JSON`, true);
    return;
  }
  const snap = await mutate("POST", "/api/projects/import", data, (s) => `Imported ${s.project.name} (progress starts at To do)`);
  if (snap) {
    state.view = "board";
    closeDrawer();
    render();
    renderProjects();
  }
}

$("#newProjectBtn").addEventListener("click", newProjectDialog);
$("#exportBtn").addEventListener("click", exportProject);
$("#importBtn").addEventListener("click", () => $("#importFile").click());
$("#importFile").addEventListener("change", async (e) => {
  const file = e.target.files[0];
  e.target.value = "";
  await importFromFile(file);
});
$("#addTaskBtn").addEventListener("click", addTaskDialog);
$("#cloneBtn").addEventListener("click", cloneDialog);
$("#undoBtn").addEventListener("click", () => mutate("POST", `/api/projects/${state.currentId}/undo`, {}, "Undone"));
$("#redoBtn").addEventListener("click", () => mutate("POST", `/api/projects/${state.currentId}/redo`, {}, "Redone"));
document.querySelectorAll("#tabs button").forEach((b) => b.addEventListener("click", () => { state.view = b.dataset.view; render(); }));
$("#modalBackdrop").addEventListener("click", (e) => { if (e.target.id === "modalBackdrop") closeModal(); });
document.addEventListener("click", closeMenus);
document.addEventListener("keydown", (e) => {
  if (e.key === "Escape") { closeModal(); closeDrawer(); closeMenus(); }
  const typing = /INPUT|TEXTAREA|SELECT/.test(document.activeElement.tagName);
  if (!state.currentId || typing || !(e.ctrlKey || e.metaKey)) return;
  if (e.key === "z" && state.snap.canUndo) { e.preventDefault(); $("#undoBtn").click(); }
  if (e.key === "y" && state.snap.canRedo) { e.preventDefault(); $("#redoBtn").click(); }
});
window.addEventListener("focus", refresh);

(async function init() {
  try {
    await loadStatic();
    await loadProjects();
    render();
  } catch (e) {
    toast("Cannot reach the TIVIDY engine: " + e.message, true);
  }
})();
