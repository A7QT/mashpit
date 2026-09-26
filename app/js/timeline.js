// timeline: ruler, lanes, clips, xfade zone, playhead (UI shell — drag moves state)
function drawTimeline() {
  const r = $('ruler');
  r.innerHTML = '';
  for (let s = 0; s <= DUR; s += s < 10 ? 1 : 5) {
    const e = document.createElement('span');
    e.style.left = `${(s / DUR) * 100}%`;
    e.textContent = s % 5 ? '·' : `0:${String(s).padStart(2, '0')}`;
    r.appendChild(e);
  }
  document.querySelectorAll('.lane').forEach((l) =>
    l.querySelectorAll('.clip,.xfade').forEach((e) => e.remove()));
  const mk = (lane, clip, cls, name) => {
    const L = document.querySelector(`.lane[data-lane="${lane}"]`);
    const el = document.createElement('div');
    el.className = `clip ${cls}`;
    el.style.left = `${(clip.s / DUR) * 100}%`;
    el.style.width = `${Math.max(1.5, ((clip.e - clip.s) / DUR) * 100)}%`;
    el.title = `${name}: drag to move in time. Gold edges = fade handles (visual). x = remove (samples only)`;
    el.innerHTML = '<div class="t"></div><div class="fade l" title="Fade in (visual)"></div><div class="fade r" title="Fade out (visual)"></div><div class="x" title="Remove this clip">x</div>';
    el.querySelector('.t').textContent = name;
    el.querySelector('.x').onclick = (e) => {
      e.stopPropagation();
      if (lane === 's') {
        S.samples = S.samples.filter((c) => c !== clip);
        drawTimeline();
      }
    };
    dragClip(el, lane, clip);
    L.appendChild(el);
  };
  mk('a', S.tr.a.clip, 'A', 'a.mp3');
  mk('b', S.tr.b.clip, 'B', 'b.mp3');
  S.samples.forEach((c) => mk('s', c, 'S', c.name));
  const w = S.xfade.bars * secPerBar(), s0 = S.xfade.at - w / 2;
  ['a', 'b'].forEach((l) => {
    const L = document.querySelector(`.lane[data-lane="${l}"]`);
    const z = document.createElement('div');
    z.className = 'xfade';
    z.style.left = `${(s0 / DUR) * 100}%`;
    z.style.width = `${(w / DUR) * 100}%`;
    z.textContent = ` XF ${S.xfade.bars}b`;
    L.appendChild(z);
  });
  $('ratioA').textContent = S.tr.a.ratio.toFixed(4);
  $('ratioB').textContent = S.tr.b.ratio.toFixed(4);
  const cA = S.tr.a.clip, cB = S.tr.b.clip;
  $('timeA').textContent = `${fmt(Math.max(0, Math.min(cA.e - cA.s, S.t - cA.s)))} / ${fmt(cA.e - cA.s)}`;
  $('timeB').textContent = `${fmt(Math.max(0, Math.min(cB.e - cB.s, S.t - cB.s)))} / ${fmt(cB.e - cB.s)}`;
  $('stXfade').textContent = `xfade ${S.xfade.bars} bars @ 0:${String(S.xfade.at % 60).padStart(2, '0')}`;
  document.querySelectorAll('[data-m]').forEach((b) =>
    b.classList.toggle('on', S.tr[b.dataset.m].mute));
  document.querySelectorAll('[data-s]').forEach((b) =>
    b.classList.toggle('on', S.tr[b.dataset.s].solo));
  document.querySelectorAll('[data-h]').forEach((b) =>
    b.classList.toggle('on', S.tr[b.dataset.h].solo));
  document.querySelectorAll('[data-main]').forEach((b) =>
    b.classList.toggle('on', !S.tr[b.dataset.main].mute));
  positionPlayhead();
}
function positionPlayhead() {
  $('ph').style.left = `${(S.t / DUR) * 100}%`;
  $('time').textContent = `${fmt(S.t)} / 00:45`;
}
function dragClip(el, lane, clip) {
  let sx = 0, o = null;
  el.addEventListener('mousedown', (e) => {
    if (e.target.className.includes('fade') || e.target.className === 'x') return;
    sx = e.clientX;
    o = { ...clip };
    e.preventDefault();
  });
  window.addEventListener('mousemove', (e) => {
    if (!o) return;
    const W = el.parentElement.offsetWidth, dx = ((e.clientX - sx) / W) * DUR, len = o.e - o.s;
    const ns = Math.max(0, Math.min(DUR - len, o.s + dx));
    clip.s = +ns.toFixed(1);
    clip.e = +(ns + len).toFixed(1);
    drawTimeline();
  });
  window.addEventListener('mouseup', () => {
    if (o) {
      log(`${lane.toUpperCase()} clip → ${clip.s.toFixed(1)}–${clip.e.toFixed(1)}s`);
      o = null;
    }
  });
}
function wireTimeline() {
  document.querySelectorAll('.lane').forEach((L) => {
    L.ondragover = (e) => e.preventDefault();
    L.ondrop = () => log('file drop lands with the engine (Phase 1)', 'warn');
  });
}
