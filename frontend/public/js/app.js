const API_BASE = "/api/v1";

const state = {
  token: localStorage.getItem("healthiq_token") || null,
  user: JSON.parse(localStorage.getItem("healthiq_user") || "null"),
  ws: null,
  chart: null,
  bmiChart: null,
  records: [],
  editingId: null,
  chartRange: localStorage.getItem("healthiq_chart_range") || "90",
};

const $ = (id) => document.getElementById(id);

/* ---------------- Utilidades ---------------- */

function cssVar(name) {
  return getComputedStyle(document.documentElement).getPropertyValue(name).trim();
}

async function api(path, options = {}) {
  const headers = { "Content-Type": "application/json", ...(options.headers || {}) };
  if (state.token) headers["Authorization"] = `Bearer ${state.token}`;
  const res = await fetch(`${API_BASE}${path}`, { ...options, headers });
  let data = {};
  try { data = await res.json(); } catch (_) {}
  if (!res.ok) {
    const err = new Error(data.message || "Error de la API");
    err.status = res.status;
    throw err;
  }
  return data;
}

function toast(message) {
  const el = $("toast");
  el.textContent = message;
  el.classList.remove("hidden");
  clearTimeout(el._t);
  el._t = setTimeout(() => el.classList.add("hidden"), 4000);
}

function fmt(n, d = 1) {
  return n === null || n === undefined || isNaN(n) ? "--" : Number(n).toFixed(d);
}

function fmtDate(s) {
  return String(s || "").replace("T", " ").slice(0, 16);
}

function addDaysFmt(dateStr, days) {
  const m = String(dateStr).match(/(\d{4})-(\d{2})-(\d{2})/);
  if (!m) return "";
  const d = new Date(Date.UTC(+m[1], +m[2] - 1, +m[3]) + days * 86400000);
  const pad = (n) => String(n).padStart(2, "0");
  return `${d.getUTCFullYear()}-${pad(d.getUTCMonth() + 1)}-${pad(d.getUTCDate())} 00:00`;
}

// Regresion lineal de minimos cuadrados sobre una serie: devuelve los valores
// ajustados de la recta y = m*x + b en cada punto del arreglo.
function linearFit(ys) {
  const n = ys.length;
  let sx = 0, sy = 0, sxx = 0, sxy = 0;
  for (let i = 0; i < n; i++) {
    sx += i;
    sy += ys[i];
    sxx += i * i;
    sxy += i * ys[i];
  }
  const denom = n * sxx - sx * sx;
  if (Math.abs(denom) < 1e-12) return ys.map(() => sy / n);
  const m = (n * sxy - sx * sy) / denom;
  const b = (sy - m * sx) / n;
  return ys.map((_, i) => m * i + b);
}

function escapeHtml(s) {
  const d = document.createElement("div");
  d.textContent = String(s ?? "");
  return d.innerHTML;
}

/* ---------------- Autenticacion ---------------- */

function setAuth(token, user) {
  state.token = token;
  state.user = user;
  localStorage.setItem("healthiq_token", token);
  localStorage.setItem("healthiq_user", JSON.stringify(user));
  $("user-label").textContent = user.name;
  $("chip-name").textContent = user.name;
  $("user-avatar").textContent = (user.name || "?").trim().charAt(0).toUpperCase();
}

function showView(name) {
  $("auth-view").classList.toggle("hidden", name !== "auth");
  $("dash-view").classList.toggle("hidden", name !== "dash");
}

function initAuth() {
  document.querySelectorAll(".tab").forEach((btn) => {
    btn.addEventListener("click", () => {
      document.querySelectorAll(".tab").forEach((b) => b.classList.remove("active"));
      btn.classList.add("active");
      const isRegister = btn.dataset.tab === "register";
      $("fld-name").style.display = isRegister ? "block" : "none";
      $("auth-submit").textContent = isRegister ? "Crear cuenta" : "Ingresar";
      $("auth-error").textContent = "";
    });
  });

  $("auth-form").addEventListener("submit", async (e) => {
    e.preventDefault();
    const isRegister = document.querySelector(".tab.active").dataset.tab === "register";
    const payload = {
      email: $("email").value.trim(),
      password: $("password").value,
    };
    if (isRegister) payload.name = $("name").value.trim();

    const btn = $("auth-submit");
    btn.disabled = true;
    btn.textContent = "Procesando...";
    $("auth-error").textContent = "";
    try {
      const data = await api(isRegister ? "/auth/register" : "/auth/login", {
        method: "POST",
        body: JSON.stringify(payload),
      });
      setAuth(data.token, data.user);
      $("auth-form").reset();
      await enterApp();
    } catch (err) {
      $("auth-error").textContent = err.message;
    } finally {
      btn.disabled = false;
      btn.textContent = isRegister ? "Crear cuenta" : "Ingresar";
    }
  });
}

/* ---------------- Dashboard ---------------- */

function makeChart(canvasId, lineColor, fillColor) {
  const ctx = $(canvasId).getContext("2d");
  return new Chart(ctx, {
    type: "line",
    data: { labels: [], datasets: [] },
    options: {
      responsive: true,
      maintainAspectRatio: false,
      animation: { duration: 500 },
      interaction: { mode: "index", intersect: false },
      plugins: {
        legend: { labels: { color: cssVar("--muted") || "#8b9bc4" } },
        tooltip: {
          backgroundColor: cssVar("--card-2") || "#151f38",
          titleColor: cssVar("--text") || "#e8eefc",
          bodyColor: cssVar("--text") || "#e8eefc",
        },
      },
      scales: {
        x: { ticks: { color: cssVar("--muted") }, grid: { color: cssVar("--border") } },
        y: { ticks: { color: cssVar("--muted") }, grid: { color: cssVar("--border") } },
      },
    },
  });
}

function initCharts() {
  if (state.chart) return;
  state.chart = makeChart("trend-chart", cssVar("--accent") || "#38bdf8",
    "rgba(56,189,248,.15)");
  state.bmiChart = makeChart("bmi-chart", cssVar("--green") || "#34d399",
    "rgba(52,211,153,.15)");
  const sel = $("chart-range");
  if (sel) {
    sel.value = state.chartRange;
    sel.addEventListener("change", () => {
      state.chartRange = sel.value;
      localStorage.setItem("healthiq_chart_range", state.chartRange);
      loadDashboard().catch(() => {});
    });
  }
}

function resetCharts() {
  if (state.chart) { state.chart.destroy(); state.chart = null; }
  if (state.bmiChart) { state.bmiChart.destroy(); state.bmiChart = null; }
  initCharts();
}

function renderMetrics(data) {
  const a = data.analysis || {};
  const pred = a.prediction || {};
  const stats = data.stats || {};

  $("m-bmi").textContent = fmt(a.bmi);
  $("m-bmi-cat").textContent = a.category || (a.available ? "" : "Sin datos");
  $("m-weight").textContent = fmt(stats.latestWeight);
  $("m-trend").textContent = (pred.trendKgPerMonth ?? 0) > 0
    ? `+${fmt(pred.trendKgPerMonth)}`
    : fmt(pred.trendKgPerMonth);
  $("m-trend-sub").textContent = `kg/mes · confianza ${pred.confidence || "baja"}`;
  $("m-bmi30").textContent = fmt(pred.bmi30d);
}

function renderChart(data) {
  const all = (data.history || []).slice().reverse(); // ascendente por fecha
  const maxN = state.chartRange === "all" ? all.length : Math.min(parseInt(state.chartRange, 10) || 90, all.length);
  const history = all.slice(-maxN);
  const labels = history.map((h) => fmtDate(h.date));
  const weights = history.map((h) => h.weightKg);
  const bmis = history.map((h) => h.bmi);

  state.chart.data.labels = labels;
  state.chart.data.datasets = [{
    label: "Peso (kg)",
    data: weights,
    borderColor: cssVar("--accent") || "#38bdf8",
    backgroundColor: (cssVar("--accent") ? `${cssVar("--accent")}26` : "rgba(56,189,248,.15)"),
    fill: true,
    tension: 0.3,
    pointRadius: 4,
    pointBackgroundColor: cssVar("--accent") || "#38bdf8",
  }];

  const a = data.analysis || {};
  const pred = a.prediction;
  const wLabels = labels.slice();
  const wDatasets = state.chart.data.datasets.slice();

  // Linea de tendencia del modelo: regresion lineal de minimos cuadrados.
  let fit = [];
  if (weights.length >= 2) {
    fit = linearFit(weights);
    wDatasets.push({
      label: "Tendencia del modelo",
      data: fit,
      borderColor: cssVar("--accent-2") || "#a78bfa",
      backgroundColor: "transparent",
      borderDash: [8, 6],
      fill: false,
      tension: 0,
      pointRadius: 0,
      borderWidth: 2,
    });
  }

  if (pred && history.length) {
    const lastDate = history[history.length - 1].date;
    const lastFit = fit[fit.length - 1];
    const step = (lastFit - fit[0]) / (fit.length - 1);
    const projData = [lastFit];
    const projLabels = [];
    for (let d = 1; d <= 90; d++) {
      projLabels.push(addDaysFmt(lastDate, d));
      projData.push(lastFit + step * d);
    }
    wLabels.push(...projLabels);
    wDatasets.push({
      label: "Prediccion IA (kg)",
      data: [...fit.slice(0, -1).map(() => null), ...projData],
      borderColor: cssVar("--accent-2") || "#6366f1",
      backgroundColor: (cssVar("--accent-2") ? `${cssVar("--accent-2")}33` : "rgba(99,102,241,.2)"),
      borderDash: [6, 4],
      fill: false,
      tension: 0,
      pointRadius: 0,
      borderWidth: 2,
    });
  }
  state.chart.data.labels = wLabels;
  state.chart.data.datasets = wDatasets;
  state.chart.update();

  const bLabels = labels.slice();
  const bDatasets = [{
    label: "IMC",
    data: bmis,
    borderColor: cssVar("--green") || "#34d399",
    backgroundColor: (cssVar("--green") ? `${cssVar("--green")}26` : "rgba(52,211,153,.15)"),
    fill: true,
    tension: 0.3,
    pointRadius: 4,
    pointBackgroundColor: cssVar("--green") || "#34d399",
  }];
  let bFit = [];
  if (bmis.length >= 2) {
    bFit = linearFit(bmis);
  }
  if (pred && history.length) {
    const lastBmiDate = history[history.length - 1].date;
    const lastBFit = bFit[bFit.length - 1];
    const bStep = (lastBFit - bFit[0]) / (bFit.length - 1);
    const bProjData = [lastBFit];
    const bProjLabels = [];
    for (let d = 1; d <= 90; d++) {
      bProjLabels.push(addDaysFmt(lastBmiDate, d));
      bProjData.push(lastBFit + bStep * d);
    }
    bLabels.push(...bProjLabels);
    bDatasets.push({
      label: "Proyeccion IA (IMC)",
      data: [...bFit.slice(0, -1).map(() => null), ...bProjData],
      borderColor: cssVar("--accent-2") || "#6366f1",
      backgroundColor: (cssVar("--accent-2") ? `${cssVar("--accent-2")}33` : "rgba(99,102,241,.2)"),
      borderDash: [6, 4],
      fill: false,
      tension: 0,
      pointRadius: 0,
      borderWidth: 2,
    });
  }
  state.bmiChart.data.labels = bLabels;
  state.bmiChart.data.datasets = bDatasets;
  state.bmiChart.update();
}

function renderGoal(data) {
  const goal = data.goal;
  const box = $("goal-status");
  if (!goal) {
    $("goal-weight").value = "";
    box.innerHTML = '<p class="placeholder">Define una meta de peso para ver tu progreso.</p>';
    return;
  }
  const history = (data.history || []).slice().reverse();
  const start = history.length ? history[0].weightKg : goal.currentWeight;
  const current = goal.currentWeight;
  const target = goal.goalWeightKg;
  $("goal-weight").value = target;

  let pct = target !== start ? ((start - current) / (start - target)) * 100 : 100;
  pct = Math.max(0, Math.min(100, pct));
  const losing = target < start;
  const deltaTxt = goal.reached
    ? '<span class="ok-txt">Meta alcanzada</span>'
    : `<span>${losing ? "Te faltan" : "Te sobran"} ${fmt(Math.abs(goal.deltaKg))} kg</span>`;

  box.innerHTML = `
    <div class="goal-info">
      <span>Inicio <strong>${fmt(start)} kg</strong></span>
      <span>Actual <strong>${fmt(current)} kg</strong></span>
      <span>Meta <strong>${fmt(target)} kg</strong></span>
      ${deltaTxt}
    </div>
    <div class="progress"><div class="progress-bar" style="width:${pct.toFixed(1)}%"></div></div>
    <div class="progress-labels"><span>${Math.round(pct)}% de la meta</span></div>`;
}

function renderRecommendations(data) {
  const recs = (data.analysis?.recommendations) || [];
  const box = $("recommendations");
  box.innerHTML = "";
  if (!recs.length) {
    box.innerHTML = '<p class="placeholder">Registra tu primera medicion para recibir recomendaciones personalizadas.</p>';
    return;
  }
  recs.forEach((r) => {
    const card = document.createElement("div");
    card.className = "rec-card";
    card.innerHTML = `
      <div class="rec-head">
        <span class="pill ${r.priority}">${r.priority}</span>
        <h4>${r.title}</h4>
      </div>
      <p>${r.detail}</p>`;
    box.appendChild(card);
  });
}

function renderModelInfo(data) {
  const a = data.analysis || {};
  const pred = a.prediction || {};
  const stats = data.stats || {};
  const box = $("model-info");
  if (!stats.recordCount) {
    box.innerHTML = '<p class="placeholder">El modelo aprende con cada registro que guardes.</p>';
    return;
  }
  const conf = pred.confidence || "baja";
  const confIcon = conf === "alta" ? "✓" : conf === "media" ? "◐" : "○";
  box.innerHTML = `
    <div class="stat"><span>Algoritmo</span><span>${pred.modelType || "Regresion lineal"}</span></div>
    <div class="stat"><span>Registros aprendidos</span><span>${stats.recordCount}</span></div>
    <div class="stat"><span>Precision (R²)</span><span>${fmt(pred.r2, 3)}</span></div>
    <div class="stat"><span>Error medio (RMSE)</span><span>${fmt(pred.rmse, 2)} kg</span></div>
    <div class="stat"><span>Desviacion media (MAE)</span><span>${fmt(pred.mae, 2)} kg</span></div>
    <div class="stat"><span>Pendiente del modelo</span><span>${fmt(pred.slope, 3)} kg/dia</span></div>
    <div class="stat"><span>Confianza de prediccion</span><span>${confIcon} ${conf}</span></div>
    <div class="stat"><span>IMC en 90 dias</span><span>${fmt(pred.bmi90d)}</span></div>`;
}

async function loadDashboard() {
  const data = await api("/dashboard/summary");
  renderMetrics(data);
  renderChart(data);
  renderGoal(data);
  renderRecommendations(data);
  renderModelInfo(data);
  return data;
}

/* ---------------- Historial (editar / eliminar) ---------------- */

function renderHistory() {
  const box = $("history-body");
  box.innerHTML = "";
  if (!state.records.length) {
    box.innerHTML = '<tr><td colspan="6" class="placeholder hist-empty">Sin registros todavia.</td></tr>';
    return;
  }
  state.records.forEach((r) => {
    const tr = document.createElement("tr");
    tr.innerHTML = `
      <td>${escapeHtml(fmtDate(r.createdAt))}</td>
      <td>${fmt(r.weightKg)} kg</td>
      <td>${fmt(r.bmi)}</td>
      <td>${r.activityLevel}</td>
      <td class="cell-note">${escapeHtml(r.note || "")}</td>
      <td class="cell-actions">
        <button class="btn-icon" data-act="edit" data-id="${r.id}">Editar</button>
        <button class="btn-icon danger" data-act="delete" data-id="${r.id}">Eliminar</button>
      </td>`;
    box.appendChild(tr);
  });
}

function resetRecordForm() {
  $("record-form").reset();
  state.editingId = null;
  $("record-submit").textContent = "Guardar y analizar";
  $("record-cancel").classList.add("hidden");
}

function startEdit(id) {
  const rec = state.records.find((r) => r.id === id);
  if (!rec) return;
  state.editingId = id;
  $("r-weight").value = rec.weightKg;
  $("r-height").value = rec.heightCm;
  $("r-activity").value = rec.activityLevel;
  $("r-note").value = rec.note || "";
  $("record-submit").textContent = "Guardar cambios";
  $("record-cancel").classList.remove("hidden");
  const result = $("record-result");
  result.className = "record-result show ok";
  result.innerHTML = "<strong>Editando registro del " + escapeHtml(fmtDate(rec.createdAt)) + ".</strong>";
  $("r-weight").scrollIntoView({ behavior: "smooth", block: "center" });
}

async function removeRecord(id) {
  if (!confirm("Eliminar este registro? La IA se reentrenara sin el.")) return;
  try {
    await api(`/records/${id}`, { method: "DELETE" });
    toast("Registro eliminado");
    await Promise.all([loadDashboard(), loadHistory()]);
  } catch (err) {
    toast(err.message);
  }
}

function initHistoryEvents() {
  $("history-body").addEventListener("click", (e) => {
    const btn = e.target.closest("[data-act]");
    if (!btn) return;
    const id = Number(btn.dataset.id);
    if (btn.dataset.act === "edit") startEdit(id);
    else if (btn.dataset.act === "delete") removeRecord(id);
  });
  $("record-cancel").addEventListener("click", () => {
    resetRecordForm();
    const result = $("record-result");
    result.className = "record-result";
    result.innerHTML = "";
  });
}

async function loadHistory() {
  const data = await api("/records?limit=200");
  state.records = data.records || [];
  renderHistory();
}

/* ---------------- Registro de mediciones ---------------- */

function initRecordForm() {
  $("record-form").addEventListener("submit", async (e) => {
    e.preventDefault();
    const payload = {
      weightKg: parseFloat($("r-weight").value),
      heightCm: parseFloat($("r-height").value),
      activityLevel: parseInt($("r-activity").value, 10),
      note: $("r-note").value.trim(),
    };
    const isEdit = state.editingId != null;
    const box = $("record-result");
    box.className = "record-result";
    try {
      const data = await api(isEdit ? `/records/${state.editingId}` : "/records", {
        method: isEdit ? "PUT" : "POST",
        body: JSON.stringify(payload),
      });
      const rec = isEdit ? data.record : data.record;
      box.classList.add("show", "ok");
      box.innerHTML = `
        <strong>${isEdit ? "Registro actualizado." : "Registro guardado."}</strong> IMC = ${fmt(rec.bmi)}
        ${isEdit ? "" : `<div class="formula">${data.record.procedure.formula}</div>`}`;
      resetRecordForm();
      await Promise.all([loadDashboard(), loadHistory()]);
    } catch (err) {
      box.classList.add("show", "err");
      box.textContent = err.message;
    }
  });
}

/* ---------------- Meta de peso ---------------- */

function initGoalForm() {
  $("goal-form").addEventListener("submit", async (e) => {
    e.preventDefault();
    const v = parseFloat($("goal-weight").value);
    if (!(v > 0)) {
      toast("Ingresa una meta valida");
      return;
    }
    try {
      await api("/users/me", { method: "PUT", body: JSON.stringify({ goalWeightKg: v }) });
      toast("Meta guardada");
      await loadDashboard();
    } catch (err) {
      toast(err.message);
    }
  });
}

/* ---------------- Perfil ---------------- */

function closeProfile() {
  $("profile-modal").classList.add("hidden");
  $("profile-error").textContent = "";
}

function openProfile() {
  $("p-name").value = state.user?.name || "";
  $("p-email").value = state.user?.email || "";
  $("p-current").value = "";
  $("p-new").value = "";
  $("profile-error").textContent = "";
  $("profile-modal").classList.remove("hidden");
}

function initProfile() {
  $("profile-btn").addEventListener("click", openProfile);
  $("user-chip").addEventListener("click", openProfile);
  $("profile-close").addEventListener("click", closeProfile);
  $("profile-cancel").addEventListener("click", closeProfile);
  $("profile-modal").addEventListener("click", (e) => {
    if (e.target.id === "profile-modal") closeProfile();
  });
  $("profile-form").addEventListener("submit", async (e) => {
    e.preventDefault();
    const payload = {
      name: $("p-name").value.trim(),
      email: $("p-email").value.trim(),
    };
    if ($("p-new").value) {
      payload.currentPassword = $("p-current").value;
      payload.newPassword = $("p-new").value;
    }
    try {
      const data = await api("/users/me", { method: "PUT", body: JSON.stringify(payload) });
      state.user = data.user;
      localStorage.setItem("healthiq_user", JSON.stringify(data.user));
      $("user-label").textContent = data.user.name;
      $("chip-name").textContent = data.user.name;
      $("user-avatar").textContent = (data.user.name || "?").trim().charAt(0).toUpperCase();
      toast("Perfil actualizado");
      closeProfile();
    } catch (err) {
      $("profile-error").textContent = err.message;
    }
  });
}

/* ---------------- Tema claro / oscuro ---------------- */

function applyTheme(theme) {
  document.documentElement.dataset.theme = theme;
  $("theme-label").textContent = theme === "dark" ? "Tema claro" : "Tema oscuro";
}

function initTheme() {
  const saved = localStorage.getItem("healthiq_theme") || "dark";
  applyTheme(saved);
  $("theme-btn").addEventListener("click", () => {
    const next = document.documentElement.dataset.theme === "dark" ? "light" : "dark";
    localStorage.setItem("healthiq_theme", next);
    applyTheme(next);
    resetCharts();
    loadDashboard().catch(() => {});
  });
}

/* ---------------- Navegacion por secciones ---------------- */

function switchSection(name) {
  document.querySelectorAll(".section").forEach((s) => s.classList.remove("active"));
  const el = $("section-" + name);
  if (el) el.classList.add("active");
  document.querySelectorAll(".nav-item[data-nav]").forEach((b) =>
    b.classList.toggle("active", b.dataset.nav === name)
  );
  const titles = {
    dashboard: "Inicio",
    record: "Registrar",
    history: "Historial",
    goal: "Meta",
  };
  $("page-title").textContent = titles[name] || "Inicio";
  closeSidebar();
}

function openSidebar() {
  $("sidebar").classList.add("open");
  $("side-backdrop").classList.add("show");
}

function closeSidebar() {
  $("sidebar").classList.remove("open");
  $("side-backdrop").classList.remove("show");
}

function initNav() {
  document.querySelectorAll(".nav-item[data-nav]").forEach((btn) =>
    btn.addEventListener("click", () => switchSection(btn.dataset.nav))
  );
  $("menu-btn").addEventListener("click", openSidebar);
  $("side-backdrop").addEventListener("click", closeSidebar);
}

/* ---------------- WebSocket en tiempo real ---------------- */

function connectWs() {
  if (!state.token) return;
  const proto = location.protocol === "https:" ? "wss" : "ws";
  const url = `${proto}://${location.host}/api/v1/ws/dashboard?token=${state.token}`;
  state.ws = new WebSocket(url);
  state.ws.onopen = () => {
    const b = $("ws-badge");
    b.textContent = "EN VIVO";
    b.className = "ws-badge online";
  };
  state.ws.onclose = () => {
    const b = $("ws-badge");
    b.textContent = "SIN CONEXION";
    b.className = "ws-badge offline";
    setTimeout(connectWs, 3000);
  };
  state.ws.onmessage = (evt) => {
    try {
      const msg = JSON.parse(evt.data);
      if (msg.event === "connected") return;
      if (msg.event === "new_record") {
        toast(`Nuevo registro analizado por la IA · IMC ${fmt(msg.record.bmi)}`);
        loadDashboard().catch(() => {});
        loadHistory().catch(() => {});
      } else if (msg.event === "update_record" || msg.event === "delete_record") {
        toast(msg.event === "update_record" ? "Registro actualizado" : "Registro eliminado");
        loadDashboard().catch(() => {});
        loadHistory().catch(() => {});
      }
    } catch (_) {}
  };
}

/* ---------------- Flujo principal ---------------- */

function initLogout() {
  $("logout-btn").addEventListener("click", () => {
    if (state.ws) state.ws.close();
    state.token = null;
    state.user = null;
    localStorage.removeItem("healthiq_token");
    localStorage.removeItem("healthiq_user");
    showView("auth");
  });
}

async function enterApp() {
  showView("dash");
  connectWs();
  await Promise.all([loadDashboard(), loadHistory()]);
}

function bootstrap() {
  initAuth();
  initLogout();
  initRecordForm();
  initHistoryEvents();
  initGoalForm();
  initProfile();
  initTheme();
  initNav();
  initCharts();
  if (state.token && state.user) {
    setAuth(state.token, state.user);
    showView("dash");
    connectWs();
    Promise.all([loadDashboard(), loadHistory()]).catch(() => {
      state.token = null;
      localStorage.removeItem("healthiq_token");
      showView("auth");
    });
  } else {
    showView("auth");
  }
}

bootstrap();
