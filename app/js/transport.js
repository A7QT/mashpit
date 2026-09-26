// transport: mock clock (UI shell — moves the playhead, no audio yet)
let tickHandle = null, lastTick = 0;
function togglePlay() {
  if (S.playing) {
    stopMock();
    log('stop');
  } else {
    pushHist();
    startMock();
    log(`play @ ${fmt(S.t)} · xf ${(S.xf * 100) | 0} · A ${S.tr.a.ratio} B ${S.tr.b.ratio}`);
  }
}
function startMock() {
  S.playing = true;
  $('play').textContent = 'Pause';
  lastTick = performance.now();
  const step = (now) => {
    if (!S.playing) return;
    S.t += (now - lastTick) / 1000;
    lastTick = now;
    if (S.loop.on && S.t >= S.loop.end) S.t = S.loop.start;
    if (S.t >= DUR) {
      stopMock();
      S.t = 0;
      refreshTransport();
      return;
    }
    refreshTransport();
    vuMock();
    requestAnimationFrame(step);
  };
  requestAnimationFrame(step);
}
function stopMock() {
  S.playing = false;
  $('play').textContent = 'Play';
}
function refreshTransport() {
  drawWaves();
  drawTimeline();
}
function vuMock() {
  const c = $('vu');
  if (!c) return;
  const W2 = c.offsetWidth * 2 || 260;
  if (c.width !== W2) c.width = W2;
  if (c.height !== 128) c.height = 128;
  const x = c.getContext('2d'), W = c.width, H = c.height;
  x.clearRect(0, 0, W, H);
  const lvl = 0.35 + 0.3 * Math.abs(Math.sin(S.t * 5)) + Math.random() * 0.15;
  const n = 22, bw = W / n;
  for (let i = 0; i < n; i++) {
    const wob = 0.55 + 0.45 * Math.abs(Math.sin(i * 1.7 + S.t * 7));
    const h = Math.max(2, Math.min(H, lvl * H * wob));
    x.fillStyle = lvl > 0.9 ? '#e05252' : lvl > 0.65 ? '#e0a100' : '#4ed07e';
    x.fillRect(i * bw + 1, H - h, bw - 2, h);
  }
}
function wireTransport() {
  $('play').onclick = togglePlay;
  $('stop').onclick = () => {
    stopMock();
    S.t = 0;
    refreshTransport();
  };
  $('rec').onclick = (e) => {
    e.target.classList.toggle('on');
    log('rec arm (stub — Render mix… to bounce)', 'warn');
  };
  $('bpm').onchange = (e) => {
    S.bpm = +e.target.value || 128;
    refreshTransport();
    log(`master → ${S.bpm} BPM (decks unchanged — hit SYNC)`, 'warn');
  };
  $('snap').onclick = (e) => e.target.classList.toggle('on');
  $('quant').onclick = (e) => e.target.classList.toggle('on');
  $('loadA').onclick = () => log('file picker lands with the engine (Phase 1)', 'warn');
  $('loadB').onclick = () => log('file picker lands with the engine (Phase 1)', 'warn');
  $('loadS').onclick = () => log('file picker lands with the engine (Phase 1)', 'warn');
  $('export').onclick = () => log('render → `lmms render mashup.mmp -f wav` (stub — Phase 1)', 'ok');
  window.addEventListener('keydown', (e) => {
    if (e.target.tagName === 'INPUT') return;
    if (e.code === 'Space') {
      e.preventDefault();
      togglePlay();
    }
  });
  window.addEventListener('resize', () => drawWaves());
}
