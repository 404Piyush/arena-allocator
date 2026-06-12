/* arena.js — JavaScript port of the arena-allocator C API.
 *
 * Same surface as the C version:
 *     arena_create(capacity)
 *     arena_alloc(a, size)
 *     arena_reset(a)
 *     arena_destroy(a)
 *     arena_in_use(a)
 *     arena_high_wat(a)
 *
 * The implementation mirrors the C bump-pointer design: a single
 * backing store, a bump pointer, high-watermark tracking.
 */
'use strict';

const ALIGN = 16;

function alignUp(n, a) { return (n + (a - 1)) & ~(a - 1); }

function arena_create(capacity) {
  if (capacity <= 0) return null;
  /* Round capacity up to a multiple of 4096 to mimic page
     alignment. */
  const cap = alignUp(capacity, 4096);
  const base = new ArrayBuffer(cap);
  return {
    base, cap,
    inUse: 0,
    highWat: 0,
    /* Used for the visual allocation log. */
    allocs: [],
    nextId: 1,
  };
}

function arena_alloc(a, size) {
  if (a === null) return null;
  if (size <= 0) return null;
  const aligned = alignUp(size, ALIGN);
  if (a.inUse + aligned > a.cap) return null;
  const offset = a.inUse;
  a.inUse += aligned;
  if (a.inUse > a.highWat) a.highWat = a.inUse;
  const view = new DataView(a.base, offset, aligned);
  const id = a.nextId++;
  a.allocs.push({ id, offset, size: aligned, req: size, view });
  return { id, view, offset, size: aligned };
}

function arena_reset(a) {
  if (a === null) return;
  a.inUse = 0;
  a.allocs = [];
}

function arena_destroy(a) {
  /* Nothing to do in JS; GC will reclaim. */
  if (a !== null) a.base = null;
}

function arena_in_use(a)  { return a === null ? 0 : a.inUse; }
function arena_high_wat(a) { return a === null ? 0 : a.highWat; }
function arena_cap(a)     { return a === null ? 0 : a.cap; }

window.arena = {
  create: arena_create, alloc: arena_alloc, reset: arena_reset,
  destroy: arena_destroy,
  inUse: arena_in_use, highWat: arena_high_wat, cap: arena_cap,
};
