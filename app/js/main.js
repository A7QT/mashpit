// mashpit UI shell — init (engine lands in Phase 1, see docs/ROADMAP.md)
wireBooth();
wireTimeline();
wireTransport();
wireMixer();
wireBottom();
renderPool();
drawWaves();
drawTimeline();
log('mashpit ui-shell ready. knobs move state; sound lands with the engine (Phase 1).', 'ok');
log('demos frozen in demo*.html — this app/ tree is the real UI going forward.', 'warn');
