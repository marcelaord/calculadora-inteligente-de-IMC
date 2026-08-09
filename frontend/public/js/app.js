const API_BASE = "/api/v1";

const state = {
  token: localStorage.getItem("healthiq_token") || null,
  user: JSON.parse(localStorage.getItem("healthiq_user") || "null"),
  ws: null,
  chart: null,
};

const $ = (id) => document.getElementById(id);

/* ---------------- Utilidades ---------------- */

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

/* ---------------- Autenticacion ---------------- */

function setAuth(token, user) {
  state.token = token;
  state.user = user;
  localStorage.setItem("healthiq_token", token);
  localStorage.setItem("healthiq_user", JSON.stringify(user));
  $("user-label").textContent = user.name;
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

function initChart() {
  if (state.chart) return;
  const ctx = $("trend-chart").getContext("2d");
  state.chart = new Chart(ctx, {
    type: "line",
    data: { labels: [], datasets: [] },
    options: {
      responsive: true,
      maintainAspectRatio: false,
      animation: { duration: 500 },
      interaction: { mode: "index", intersect: false },
      plugins: {
        legend: { labels: { color: "#8b9bc4" } },
        tooltip: { backgroundColor: "#151f38", titleColor: "#e8eefc", bodyColor: "#e8eefc" },
      },
      scales: {
        x: { ticks: { color: "#8b9bc4" }, grid: { color: "#1a2645" } },
        y: { ticks: { color: "#8b9bc4" }, grid: { color: "#1a2645" } },
      },
    },
  });
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
  const history = (data.history || []).reverse(); // ascendente por fecha
  const labels = history.map((h) => h.date.slice(5, 16).replace("T", " "));
  const weights = history.map((h) => h.weightKg);

  const datasets = [{
    label: "Peso (kg)",
    data: weights,
    borderColor: "#38bdf8",
    backgroundColor: "rgba(56,189,248,.15)",
    fill: true,
    tension: 0.3,
    pointRadius: 4,
    pointBackgroundColor: "#38bdf8",
  }];

  const a = data.analysis || {};
  const pred = a.prediction;
  if (pred && history.length) {
    const lastW = history[history.length - 1].weightKg;
    const dates = [
      history[history.length - 1].date.slice(5, 16).replace("T", " "),
      null, null, null,
    ];
    const p = [lastW, pred.weight7d, pred.weight30d, pred.weight90d];
    datasets.push({
      label: "Prediccion IA (kg)",
      data: p,
      borderColor: "#6366f1",
      backgroundColor: "rgba(99,102,241,.2)",
      borderDash: [6, 4],
      fill: false,
      tension: 0.3,
      pointRadius: 5,
      pointBackgroundColor: "#6366f1",
    });
    labels.push("+7 dias", "+30 dias", "+90 dias");
  }

  state.chart.data.labels = labels;
  state.chart.data.datasets = datasets;
  state.chart.update();
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
  box.innerHTML = `
    <div class="stat"><span>Registros aprendidos</span><span>${stats.recordCount}</span></div>
    <div class="stat"><span>Pendiente del modelo</span><span>${fmt(pred.slope, 3)} kg/dia</span></div>
    <div class="stat"><span>Confianza de prediccion</span><span>${pred.confidence || "baja"}</span></div>
    <div class="stat"><span>IMC en 90 dias</span><span>${fmt(pred.bmi90d)}</span></div>`;
}

async function loadDashboard() {
  const data = await api("/dashboard/summary");
  renderMetrics(data);
  renderChart(data);
  renderRecommendations(data);
  renderModelInfo(data);
  return data;
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
    const box = $("record-result");
    box.className = "record-result";
    try {
      const data = await api("/records", { method: "POST", body: JSON.stringify(payload) });
      const rec = data.record;
      box.classList.add("show", "ok");
      box.innerHTML = `
        <strong>Registro guardado.</strong> IMC = ${fmt(rec.bmi)} · ${rec.category}
        <div class="formula">${rec.procedure.formula}</div>`;
      $("record-form").reset();
      await loadDashboard();
    } catch (err) {
      box.classList.add("show", "err");
      box.textContent = err.message;
    }
  });
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
  await loadDashboard();
}

function bootstrap() {
  initAuth();
  initLogout();
  initRecordForm();
  initChart();
  if (state.token && state.user) {
    $("user-label").textContent = state.user.name;
    showView("dash");
    connectWs();
    loadDashboard().catch(() => {
      state.token = null;
      localStorage.removeItem("healthiq_token");
      showView("auth");
    });
  } else {
    showView("auth");
  }
}

bootstrap();
