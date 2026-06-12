/* app.js — arena-allocator playground.
 *
 * Drives a JS port of the arena and renders the bump pointer
 * as a horizontal bar. The allocation list is shown in the
 * log below.
 */
'use strict';

const CAPACITY = 65536;   /* 64 KiB */
const arena = arena.create(CAPACITY);

const elBar      = document.getElementById('pg-bar');
const elPointer  = document.getElementById('pg-pointer');
const elBarLabel = document.getElementById('pg-bar-label');
const elInUse    = document.getElementById('pg-inuse');
const elHigh     = document.getElementById('pg-high');
const elLog      = document.getElementById('pg-log');

const logBuf = [];
function log(line, kind) {
  logBuf.push({ line, kind });
  if (logBuf.length > 50) logBuf.shift();
  elLog.innerHTML = logBuf
    .map(o => `<span class="${o.kind || ''}">${escapeHtml(o.line)}</span>`)
    .join('\n');
  elLog.scrollTop = elLog.scrollHeight;
}
function escapeHtml(s) {
  return String(s).replace(/[&<>"']/g, c => ({
    '&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;',
  }[c]));
}

function render() {
  const inUse = arena.inUse(arena);
  const high  = arena.highWat(arena);
  const cap   = arena.cap(arena);
  const pct   = (inUse / cap) * 100;

  elBar.style.width = pct + '%';
  elPointer.style.left = pct + '%';
  elBarLabel.textContent = `${inUse} / ${cap} bytes`;
  elInUse.textContent = inUse;
  elHigh.textContent  = high;
}

function alloc(n) {
  const r = arena.alloc(arena, n);
  if (r === null) {
    log(`alloc(${n}): OOM (out of arena capacity)`, 'err');
    return;
  }
  log(`alloc(${n}) -> id=${r.id} offset=${r.offset} actual=${r.size}`, 'ok');
  render();
}
function reset() {
  arena.reset(arena);
  log('arena_reset()', 'note');
  render();
}
function destroy() {
  arena.destroy(arena);
  log('arena_destroy()', 'note');
  render();
}
function bench() {
  /* 100k small allocs + reset cycles, in-place. */
  const N = 100000;
  const t0 = performance.now();
  for (let i = 0; i < N; i++) arena.alloc(arena, 32);
  const t1 = performance.now();
  arena.reset(arena);
  const t2 = performance.now();
  const allocRate = (N / ((t1 - t0) / 1000)) / 1e6;
  log(`bench: ${N} allocs in ${(t1-t0).toFixed(2)}ms -> ${allocRate.toFixed(1)} M ops/s`, 'ok');
  log(`bench: reset in ${(t2-t1).toFixed(2)}ms`, 'note');
  render();
}

document.getElementById('pg-alloc-16').addEventListener('click', () => alloc(16));
document.getElementById('pg-alloc-64').addEventListener('click', () => alloc(64));
document.getElementById('pg-alloc-256').addEventListener('click', () => alloc(256));
document.getElementById('pg-alloc-1024').addEventListener('click', () => alloc(1024));
document.getElementById('pg-reset').addEventListener('click', reset);
document.getElementById('pg-bench').addEventListener('click', bench);

render();
log(`arena_create(${CAPACITY})`, 'note');
log('click an alloc button to bump the pointer', '');
