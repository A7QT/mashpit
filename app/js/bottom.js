// bottom: FX rack, sample pool, copilot (UI shell — mock suggestions, real state edits)
function renderPool() {
  const p = $('pool');
  p.innerHTML = '';
  if (!S.pool.length) {
    p.innerHTML = '<div style="color:var(--dim);font-size:11px;margin-top:4px">empty</div>';
    return;
  }
  S.pool.forEach((it) => {
    const c = document.createElement('div');
    c.className = 'chip';
    const nm = document.createElement('span');
    nm.className = 'nm';
    nm.textContent = it.name;
    nm.title = `${it.name} — audition or place at playhead (engine Phase 1 for sound)`;
    const a = document.createElement('button');
    a.textContent = '▶';
    a.title = 'Audition (stub — engine Phase 1)';
    a.onclick = () => log(`audition: ${it.name} (stub — engine Phase 1)`, 'warn');
    const b = document.createElement('button');
    b.textContent = `@ ${fmt(S.t)}`;
    b.title = 'Place on the S lane at the playhead';
    b.onclick = () => {
      S.samples.push({ name: it.name, s: +S.t.toFixed(1), e: +Math.min(DUR, S.t + 4).toFixed(1) });
      $('poolinfo').textContent = `${S.samples.length} items`;
      drawTimeline();
      log(`placed ${it.name} @ ${fmt(S.t)}`, 'ok');
    };
    c.append(nm, a, b);
    p.appendChild(c);
  });
}
function wireBottom() {
  $('fxCut').oninput = (e) => {
    S.fx.cutoff = +e.target.value;
    $('fxCutV').textContent = +e.target.value >= 18000 ? 'open' : `${(+e.target.value / 1000).toFixed(1)}kHz`;
  };
  $('fxEcho').oninput = (e) => {
    S.fx.echo = +e.target.value;
    $('fxEchoV').textContent = `${e.target.value}%`;
  };
  $('ask').onclick = () => {
    const q = $('prompt').value.trim().toLowerCase();
    pushHist();
    const m = q.match(/(\d+):(\d+)/);
    const at = m ? +m[1] * 60 + +m[2] : 21;
    if (/sync|match.*bpm/.test(q)) {
      sync('a'); sync('b');
      log('copilot: synced both + keylock', 'ok');
    } else if (/smooth|softer| abrupt/.test(q)) {
      S.xfade = { at, bars: 8 };
      drawTimeline();
      log(`copilot: xfade → 8 bars @ 0:${String(at % 60).padStart(2, '0')} equal-power`, 'ok');
    } else if (/gain|louder|quieter|vocal/.test(q)) {
      S.tr.b.vol = Math.min(1.2, S.tr.b.vol + 0.1);
      drawTimeline();
      log('copilot: B +~1dB', 'ok');
    } else if (/help/.test(q)) {
      log('cmds: sync | smooth 0:21 | vocal up | undo');
    } else {
      log(`copilot: no-op for "${q}" — try "smooth 0:21"`, 'warn');
      S.hist.pop();
    }
  };
  $('undo').onclick = () => {
    const p = S.hist.pop();
    if (!p) {
      log('nothing to undo', 'warn');
      return;
    }
    const o = JSON.parse(p);
    S.xfade = o.x;
    S.tr.a.ratio = o.r[0];
    S.tr.b.ratio = o.r[1];
    S.tr.a.clip = o.c[0];
    S.tr.b.clip = o.c[1];
    drawTimeline();
    log('undo ✓', 'ok');
  };
}
