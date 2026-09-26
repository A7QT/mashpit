// mixer: faders, mute/solo, crossfader mirror, master (UI shell — state only)
function setXf(v) {
  S.xf = v;
  $('xf').value = v * 100;
  $('xf2').value = v * 100;
  const side = v < 0.45 ? 'A' : v > 0.55 ? 'B' : 'C';
  $('dbX').textContent = side;
  $('xfv2').textContent = side;
}
function autoXf() {
  S.xfade = { at: 21, bars: 8 };
  pushHist();
  drawTimeline();
  log('wrote auto-xfade: 8 bars @ 0:21, equal-power', 'ok');
}
function wireMixer() {
  document.querySelectorAll('[data-vol]').forEach((r) => {
    r.oninput = (e) => {
      const id = e.target.dataset.vol;
      S.tr[id].vol = e.target.value / 100;
      const db = e.target.value <= 0 ? '-inf' : `${(20 * Math.log10(e.target.value / 100)).toFixed(1)}dB`;
      if (id === 'a') $('dbA').textContent = db;
      if (id === 'b') $('dbB').textContent = db;
    };
  });
  document.querySelectorAll('[data-m]').forEach((b) => {
    b.onclick = () => {
      const t = S.tr[b.dataset.m];
      t.mute = !t.mute;
      log(`${b.dataset.m} mute ${t.mute ? 'on' : 'off'}`);
      drawTimeline();
    };
  });
  document.querySelectorAll('[data-s]').forEach((b) => {
    b.onclick = () => {
      const t = S.tr[b.dataset.s];
      t.solo = !t.solo;
      log(`${b.dataset.s} solo ${t.solo ? 'on' : 'off'}`);
      drawTimeline();
    };
  });
  $('xf').oninput = (e) => setXf(e.target.value / 100);
  $('xf2').oninput = (e) => setXf(e.target.value / 100);
  $('mstKnob').oninput = (e) => { S.master = e.target.value / 100; };
  $('autoX').onclick = autoXf;
  $('autoX2').onclick = autoXf;
}
