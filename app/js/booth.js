// booth: Mixxx-replica decks (UI shell — knobs move state + readouts, engine Phase 1)
function fakeWave(id, seed) {
  const c = $(id), x = c.getContext('2d');
  c.width = c.offsetWidth * 2 || 700; c.height = 128;
  const W = c.width, H = c.height;
  x.clearRect(0, 0, W, H);
  let s = seed;
  const rnd = () => (s = (s * 16807) % 2147483647) / 2147483647;
  x.fillStyle = id === 'waveA' ? 'rgba(91,157,255,.45)' : 'rgba(78,208,126,.45)';
  for (let p = 0; p < W; p += 4) {
    const h = 6 + rnd() * H * .4;
    x.fillRect(p, H / 2 - h / 2, 2.4, h);
  }
  x.fillStyle = 'rgba(255,255,255,.14)';
  const n = (S.bpm / 60) * DUR;
  for (let b = 0; b < n; b++) x.fillRect((b / n) * W, 0, b % 4 ? 1 : 2, H);
  x.fillStyle = '#e05252';
  x.fillRect((S.t / DUR) * W, 0, 2, H);
}
function drawWaves() { fakeWave('waveA', 3); fakeWave('waveB', 7); }

function sync(id) {
  const t = S.tr[id];
  const b = t.bpm || (id === 'a' ? 124 : 132);
  t.ratio = +(S.bpm / b).toFixed(4);
  t.lock = true;
  $(id === 'a' ? 'pitchA' : 'pitchB').value = t.ratio * 100;
  updPitch();
  drawTimeline();
  log(`deck ${id.toUpperCase()} sync → ${t.ratio} (file ${b} → master ${S.bpm}) keylock ON`, 'ok');
}
function updPitch() {
  const a = $('pitchA').value / 100, b = $('pitchB').value / 100;
  S.tr.a.ratio = a; S.tr.b.ratio = b;
  $('pitchvA').textContent = (a >= 1 ? '+' : '') + ((a - 1) * 100).toFixed(2) + '%';
  $('pitchvB').textContent = (b >= 1 ? '+' : '') + ((b - 1) * 100).toFixed(2) + '%';
  drawTimeline();
}
function keyShift(id, delta) {
  const t = S.tr[id];
  t.key = Math.max(-6, Math.min(6, t.key + delta));
  $(id === 'a' ? 'keyAv' : 'keyBv').textContent = (t.key >= 0 ? '+' : '') + t.key + ' st';
  log(`deck ${id.toUpperCase()} key ${(t.key >= 0 ? '+' : '') + t.key} st${t.lock ? ' · tempo held' : ' · keylock OFF'}`);
}
function beatjump(id, n) {
  const beat = 60 / S.bpm;
  let nt = S.t + n * beat;
  if ($('quant').classList.contains('on')) nt = Math.round(nt / beat) * beat;
  S.t = Math.max(0, Math.min(DUR, nt));
  refreshTransport();
  log(`deck ${id.toUpperCase()} ${n > 0 ? '+' : ''}${n} beats → ${fmt(S.t)}`);
}
function cueGo(id) {
  S.t = S.tr[id].clip.s;
  refreshTransport();
  log(`deck ${id.toUpperCase()} CUE → ${fmt(S.t)}`);
}
function armLoop(id, beats) {
  const btn = $(`loop${id.toUpperCase()}${beats}`);
  const wasOn = S.loop.on && S.loop.beats === beats;
  document.querySelectorAll('.looprow button').forEach((b) => b.classList.remove('on'));
  if (wasOn) {
    S.loop.on = false;
    $('loopClear').textContent = 'loop: off';
    log('loop released');
    return;
  }
  const beat = 60 / S.bpm;
  S.loop = { on: true, beats, start: +S.t.toFixed(2), end: +(S.t + beats * beat).toFixed(2) };
  btn.classList.add('on');
  $('loopClear').textContent = `loop ${beats}b ${fmt(S.loop.start)}–${fmt(S.loop.end)}`;
  log(`loop ${beats} beats ${fmt(S.loop.start)}–${fmt(S.loop.end)}`, 'ok');
}

function wireBooth() {
  $('syncA').onclick = () => sync('a');
  $('syncB').onclick = () => sync('b');
  $('lockA').onclick = () => toggleLock('a');
  $('lockB').onclick = () => toggleLock('b');
  $('filebpmA').onchange = (e) => { S.tr.a.bpm = parseFloat(e.target.value) || null; };
  $('filebpmB').onchange = (e) => { S.tr.b.bpm = parseFloat(e.target.value) || null; };
  $('pitchA').oninput = updPitch;
  $('pitchB').oninput = updPitch;
  $('nudgeAm').onclick = () => { $('pitchA').value = +$('pitchA').value - 0.05; updPitch(); };
  $('nudgeAp').onclick = () => { $('pitchA').value = +$('pitchA').value + 0.05; updPitch(); };
  $('nudgeBm').onclick = () => { $('pitchB').value = +$('pitchB').value - 0.05; updPitch(); };
  $('nudgeBp').onclick = () => { $('pitchB').value = +$('pitchB').value + 0.05; updPitch(); };
  $('keyAm').onclick = () => keyShift('a', -1);
  $('keyAp').onclick = () => keyShift('a', 1);
  $('keyBm').onclick = () => keyShift('b', -1);
  $('keyBp').onclick = () => keyShift('b', 1);
  [['jumpAm4', 'a', -4], ['jumpAm1', 'a', -1], ['jumpAp1', 'a', 1], ['jumpAp4', 'a', 4],
   ['jumpBm4', 'b', -4], ['jumpBm1', 'b', -1], ['jumpBp1', 'b', 1], ['jumpBp4', 'b', 4]
  ].forEach(([el, id, n]) => { $(el).onclick = () => beatjump(id, n); });
  $('cueA').onclick = () => cueGo('a');
  $('cueB').onclick = () => cueGo('b');
  [['loopA1', 'a', 1], ['loopA2', 'a', 2], ['loopA4', 'a', 4], ['loopA8', 'a', 8],
   ['loopB1', 'b', 1], ['loopB2', 'b', 2], ['loopB4', 'b', 4], ['loopB8', 'b', 8]
  ].forEach(([el, id, n]) => { $(el).onclick = () => armLoop(id, n); });
  $('loopClear').onclick = () => {
    S.loop.on = false;
    document.querySelectorAll('.looprow button').forEach((b) => b.classList.remove('on'));
    $('loopClear').textContent = 'loop: off';
    log('loop released');
  };
  document.querySelectorAll('[data-cue]').forEach((b) => {
    b.onclick = (e) => {
      const id = b.dataset.cue, i = b.dataset.i, d = S.tr[id];
      if (e.altKey) {
        delete d.cues[i];
        b.classList.remove('on');
        log(`hot ${+i + 1} cleared (deck ${id.toUpperCase()})`);
        return;
      }
      if (d.cues[i] == null) {
        d.cues[i] = +S.t.toFixed(1);
        b.classList.add('on');
        log(`hot ${+i + 1} set @ ${fmt(S.t)} (deck ${id.toUpperCase()})`, 'ok');
      } else {
        S.t = d.cues[i];
        refreshTransport();
        log(`hot ${+i + 1} → ${fmt(S.t)}`);
      }
    };
  });
  document.querySelectorAll('[data-h]').forEach((b) => {
    b.onclick = () => {
      const t = S.tr[b.dataset.h];
      t.solo = !t.solo;
      log(`${b.dataset.h.toUpperCase()} PFL ${t.solo ? 'on — cued alone' : 'off'}`);
      drawTimeline();
    };
  });
  document.querySelectorAll('[data-main]').forEach((b) => {
    b.onclick = () => {
      const t = S.tr[b.dataset.main];
      t.mute = !t.mute;
      log(`${b.dataset.main.toUpperCase()} main ${t.mute ? 'OUT' : 'in'}`);
      drawTimeline();
    };
  });
  [['trimA', 'a'], ['trimB', 'b']].forEach(([el, id]) => {
    $(el).oninput = (e) => { S.tr[id].trim = e.target.value / 100; };
  });
  [['eqAlow', 'a', 'low'], ['eqAmid', 'a', 'mid'], ['eqAhi', 'a', 'hi'],
   ['eqBlow', 'b', 'low'], ['eqBmid', 'b', 'mid'], ['eqBhi', 'b', 'hi'],
  ].forEach(([el, id, key]) => {
    $(el).oninput = (e) => {
      S.tr[id][key] = +e.target.value;
      $(`${el}V`).textContent = (+e.target.value >= 0 ? '+' : '') + e.target.value;
    };
  });
  [['sendA', 'a'], ['sendB', 'b']].forEach(([el, id]) => {
    $(el).oninput = (e) => {
      S.send[id] = +e.target.value;
      $(`${el}v`).textContent = `${e.target.value}%`;
    };
  });
  $('deckPlayA').onclick = $('deckPlayB').onclick = () => togglePlay();
  $('ejectA').onclick = () => log('eject A (stub — engine Phase 1)', 'warn');
  $('ejectB').onclick = () => log('eject B (stub — engine Phase 1)', 'warn');
  wireJog('a');
  wireJog('b');
}
function toggleLock(id) {
  const t = S.tr[id];
  t.lock = !t.lock;
  $(id === 'a' ? 'lockA' : 'lockB').textContent = t.lock ? 'KEYLOCK ON' : 'KEYLOCK OFF';
  log(`deck ${id} keylock ${t.lock ? 'on (tempo holds, pitch free)' : 'off (pitch follows tempo)'}`);
}
// jog: visual spin + seek while stopped (bend physics lands with the engine)
function wireJog(id) {
  const el = $(id === 'a' ? 'jogA' : 'jogB');
  let drag = false, lx = 0, rot = 0;
  el.addEventListener('pointerdown', (e) => {
    drag = true; lx = e.clientX;
    try { el.setPointerCapture(e.pointerId); } catch (_) { /* noop */ }
  });
  el.addEventListener('pointermove', (e) => {
    if (!drag) return;
    const dx = e.clientX - lx;
    lx = e.clientX;
    rot = (rot + dx * 0.8) % 360;
    el.style.setProperty('--rot', `${rot}deg`);
    if (!S.playing) {
      S.t = Math.max(0, Math.min(DUR, S.t + dx * 0.03));
      refreshTransport();
    }
  });
  const end = () => { drag = false; };
  el.addEventListener('pointerup', end);
  el.addEventListener('pointercancel', end);
}
