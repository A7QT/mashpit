// mashpit UI shell — shared state (mock data, no audio engine yet)
const DUR = 45;
const S = {
  bpm: 128, t: 0, playing: false,
  xf: .5, master: .9, xfade: { at: 21, bars: 4 },
  tr: {
    a: { mute: false, solo: false, vol: .9, trim: 1, ratio: 1, lock: false,
         bpm: 124, key: 0, low: 0, mid: 0, hi: 0,
         clip: { s: 0, e: 29 }, cues: {} },
    b: { mute: false, solo: false, vol: .9, trim: 1, ratio: 1, lock: false,
         bpm: 132, key: 0, low: 0, mid: 0, hi: 0,
         clip: { s: 13, e: 45 }, cues: {} },
    s: { mute: false, solo: false, vol: 1 },
  },
  fx: { cutoff: 18000, echo: 0 },
  send: { a: 0, b: 0 },
  loop: { on: false, beats: 0, start: 0, end: 0 },
  samples: [
    { name: 'vocal chop 1', s: 20, e: 24 },
    { name: 'horn hit', s: 29, e: 31 },
  ],
  pool: [{ name: 'vocal chop 1' }, { name: 'horn hit' }, { name: 'riser 8b' }],
  hist: [],
};
const $ = (id) => document.getElementById(id);
const fmt = (t) => `${String(Math.floor(t / 60)).padStart(2, '0')}:${(t % 60).toFixed(1).padStart(4, '0')}`;
const secPerBar = () => (60 / S.bpm) * 4;
function log(m, c = '') {
  const d = document.createElement('div');
  if (c) d.className = c;
  d.textContent = `[${fmt(S.t)}] ${m}`;
  $('log').prepend(d);
}
function snapshot() {
  return JSON.stringify({ x: S.xfade, r: [S.tr.a.ratio, S.tr.b.ratio],
    c: [S.tr.a.clip, S.tr.b.clip] });
}
function pushHist() {
  S.hist.push(snapshot());
  if (S.hist.length > 20) S.hist.shift();
}
