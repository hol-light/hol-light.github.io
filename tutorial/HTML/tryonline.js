/* ---------------------------------------------------------------------------
   "Try Online" for the HOL Light tutorial.
   ---------------------------------------------------------------------------

   The tutorial is full of printed HOL Light sessions: lines that start with
   the toplevel's "#" prompt, followed by the output they produced.  This
   script turns every one of those prompt lines into a button that runs the
   phrase for real, in HOL Light compiled to JavaScript
   (https://hol-light.github.io/web/site/, source: hol-light/hol-light-web),
   shown in a pane docked to the right of the page.

     * "Open HOL Light in a web browser" (top right) opens the pane and boots
       the kernel.  Booting takes a minute or two, so the pane stays alive
       until the page is reloaded: closing and re-opening it keeps whatever
       has already been defined, and clicks that arrive while the kernel is
       still coming up are queued, not dropped.
     * Clicking a "#" line runs that phrase.  Shift-click (or Alt-click) puts
       it in the REPL's input box without running it, ready to be edited.
     * "Run" at the top right of a session block runs every phrase in it, in
       order -- useful for the blocks that build up a proof step by step.

   Communication with the pane is the postMessage protocol documented in
   hol-light-web's index.html ({holweb: 'eval'|'insert'|'reset'|'ping'} out,
   {holweb: 'hello'|'status'} back).  Nothing here reaches into the iframe's
   DOM, so the REPL can move to another origin without breaking this page.

   Loaded by HTML/tutorial.html, which build-html.sh patches to pull in this
   file and tryonline.css.  Both are plain assets copied into HTML/ -- there
   is nothing to build.
   --------------------------------------------------------------------------- */

(() => {
  'use strict';

  /* Where the in-browser HOL Light lives.  ?holweb=<url> on the tutorial's
     own URL overrides it, which is how you test against a local checkout:
       tutorial.html?holweb=http://localhost:8000/  */
  const DEFAULT_REPL = 'https://hol-light.github.io/web/site/';
  const REPL_BASE = new URLSearchParams(location.search).get('holweb') || DEFAULT_REPL;

  const WIDTH_KEY = 'holweb-pane-width';
  const MIN_WIDTH = 320;

  function replUrl() {
    const u = new URL(REPL_BASE, location.href);
    u.searchParams.set('embed', '1');
    u.searchParams.set('theme',
      window.matchMedia('(prefers-color-scheme: dark)').matches ? 'dark' : 'light');
    return u.href;
  }

  /* ---- 1.  Reading the phrases out of a printed session -------------------

     A verbatim block holds the source verbatim: build-html.sh step 4
     (fix-tt-glyphs.py) turns tex4ht's typewriter glyphs and U+00A0 spaces
     back into ASCII, so .textContent is already a phrase HOL Light accepts.
     (It decodes entities like &gt; for us too, which .innerHTML would not.)

     A page built without that step decorates nothing, since PROMPT below
     does not match its "#\u00a0" prompt lines -- which is why step 4 fails
     the build rather than shipping one. */

  /* A prompt line is "#" in the first column followed by a space.  That is
     deliberately strict: it skips the shell transcripts (where "#" appears
     mid-line as a comment) and the indented OCaml directives such as
     `#use "filename";;`, neither of which is a phrase to run. */
  const PROMPT = /^#(?:[ \t]|$)/;

  /* A phrase ends at the ";;" that terminates it, ignoring a trailing OCaml
     comment -- and only outside a term, since ";;" is also the sequencing
     operator of the little imperative language in the verification chapter,
     where it appears inside backquotes.  Input that spans several printed
     lines therefore swallows its continuation lines, and stops before the
     output that follows. */
  const PHRASE_END = /;;\s*(?:\(\*[\s\S]*?\*\)\s*)*$/;
  const MAX_PHRASE_LINES = 40;
  const complete = (parts) => {
    const text = parts.join('\n');
    return PHRASE_END.test(parts[parts.length - 1]) &&
           (text.split('`').length - 1) % 2 === 0;
  };

  /* Split a block's text into the phrases it contains, as
     {from, to, code} with `from`/`to` line indices into `text`. */
  function phrasesOf(text) {
    const src = text.split('\n');
    const out = [];
    for (let i = 0; i < src.length; i++) {
      if (!PROMPT.test(src[i])) continue;
      /* A bare "#" is a prompt printed for its own sake (the end of a boot
         transcript, say); there is no phrase on it to pick up. */
      if (!src[i].slice(1).trim()) continue;
      const parts = [src[i].slice(1)];
      let j = i;
      while (!complete(parts) && j + 1 < src.length && !PROMPT.test(src[j + 1]) &&
             parts.length < MAX_PHRASE_LINES) {
        parts.push(src[++j]);
      }
      i = j;
      /* If we never reached the end of the phrase we have misread the block --
         an interrupted session, a prompt shown for its own sake.  Leave those
         lines alone rather than offering to run something truncated. */
      if (!complete(parts)) continue;
      const code = parts.join('\n').trim();
      if (code) out.push({ from: i - (parts.length - 1), to: j, code });
    }
    return out;
  }

  /* ---- 2.  Decorating the blocks ----------------------------------------- */

  /* Rebuild `pre`'s contents with each phrase wrapped in a clickable span.
     Returns the phrases found (empty if this block has none). */
  function decorate(pre) {
    const text = pre.textContent;
    const phrases = phrasesOf(text);
    if (!phrases.length) return phrases;
    const lines = text.split('\n');

    /* Partition the lines into runs: each phrase is one run, everything
       between them is another. */
    const runs = [];
    let k = 0, p = 0;
    while (k < lines.length) {
      if (p < phrases.length && phrases[p].from === k) {
        runs.push({ from: k, to: phrases[p].to, code: phrases[p].code });
        k = phrases[p].to + 1;
        p++;
      } else {
        const from = k;
        while (k < lines.length && !(p < phrases.length && phrases[p].from === k)) k++;
        runs.push({ from, to: k - 1 });
      }
    }

    pre.textContent = '';
    runs.forEach((run, idx) => {
      const chunk = lines.slice(run.from, run.to + 1).join('\n');
      if (run.code) {
        const span = document.createElement('span');
        span.className = 'holweb-phrase';
        span.dataset.holwebCode = run.code;
        span.setAttribute('role', 'button');
        span.tabIndex = 0;
        span.title = 'Run this in HOL Light (Shift+click: insert without running)';
        /* Split the leading "#" out so it can be colored as an affordance. */
        const hash = document.createElement('span');
        hash.className = 'holweb-prompt';
        hash.textContent = chunk.slice(0, 1);
        span.append(hash, chunk.slice(1));
        pre.appendChild(span);
      } else {
        pre.appendChild(document.createTextNode(chunk));
      }
      if (idx < runs.length - 1) pre.appendChild(document.createTextNode('\n'));
    });
    return phrases;
  }

  /* The gray code box is a table tex4ht wraps in div.flushleft; hang the
     block button off that, where it will not scroll away with an overlong
     line.  Two blocks can share one wrapper, so fall back to the <pre>'s own
     parent for the second and later ones. */
  function addRunBlockButton(pre, phrases) {
    let host = pre.closest('div.flushleft') || pre.parentElement;
    if (!host || host.querySelector(':scope > .holweb-runblock')) host = pre.parentElement;
    if (!host) return;
    const btn = document.createElement('button');
    btn.type = 'button';
    btn.className = 'holweb-runblock';
    btn.textContent = phrases.length > 1 ? '▶ Run all' : '▶ Run';
    btn.title = phrases.length > 1
      ? 'Run all ' + phrases.length + ' phrases of this session in HOL Light'
      : 'Run this in HOL Light';
    /* One message per block: the worker takes a whole `;;`-separated run of
       phrases, so they stay in order and are echoed as one input. */
    btn.dataset.holwebCode = phrases.map((ph) => ph.code).join('\n');
    host.classList.add('holweb-host');
    host.appendChild(btn);
  }

  function decorateAll() {
    let blocks = 0, total = 0;
    for (const pre of document.querySelectorAll('pre.verbatim')) {
      const phrases = decorate(pre);
      if (!phrases.length) continue;
      addRunBlockButton(pre, phrases);
      blocks++;
      total += phrases.length;
    }
    return { blocks, phrases: total };
  }

  /* ---- 3.  The pane ------------------------------------------------------ */

  let pane = null, frame = null, statusEl = null;
  let paneLive = false;      /* the iframe has said hello */
  const outbox = [];         /* messages held until it does */

  function paneWidth() {
    let stored = NaN;
    /* localStorage throws outright when the browser blocks site data. */
    try { stored = parseInt(localStorage.getItem(WIDTH_KEY) || '', 10); } catch (_) {}
    const wanted = Number.isFinite(stored) ? stored : Math.round(window.innerWidth * 0.45);
    return setPaneWidth(wanted, false);
  }

  function setPaneWidth(px, persist = true) {
    const max = Math.max(MIN_WIDTH, window.innerWidth - 260);
    const w = Math.round(Math.min(max, Math.max(MIN_WIDTH, px)));
    document.documentElement.style.setProperty('--holweb-pane-width', w + 'px');
    if (persist) { try { localStorage.setItem(WIDTH_KEY, String(w)); } catch (_) {} }
    return w;
  }

  function setStatus(text, busy) {
    if (!statusEl) return;
    statusEl.textContent = text;
    statusEl.classList.toggle('holweb-busy', !!busy);
  }

  function buildPane() {
    if (pane) return;
    pane = document.createElement('div');
    pane.id = 'holweb-pane';
    pane.innerHTML =
      '<div id="holweb-grip" title="Drag to resize"></div>' +
      '<div id="holweb-head">' +
        '<b>HOL Light</b><span id="holweb-status"></span>' +
        '<button type="button" id="holweb-reset" title="Reset the toplevel and re-boot the kernel">⟲</button>' +
        '<button type="button" id="holweb-newtab" title="Open the REPL in its own tab">↗</button>' +
        '<button type="button" id="holweb-close" title="Close the pane (the kernel keeps running)">✕</button>' +
      '</div>' +
      '<iframe id="holweb-frame" title="HOL Light in the browser"></iframe>';
    document.body.appendChild(pane);
    statusEl = document.getElementById('holweb-status');
    frame = document.getElementById('holweb-frame');
    setStatus('starting…', true);

    document.getElementById('holweb-close')
      .addEventListener('click', () => closePane());
    document.getElementById('holweb-newtab')
      .addEventListener('click', () => window.open(REPL_BASE, '_blank', 'noopener'));
    document.getElementById('holweb-reset').addEventListener('click', () => {
      if (!confirm('Reset the toplevel?  Everything defined in this session is '
                   + 'lost and the kernel re-boots (a minute or two).')) return;
      send({ holweb: 'reset' });
    });
    installGrip(document.getElementById('holweb-grip'));

    /* Set src last: booting starts the moment the iframe navigates, and only
       ever once per page load. */
    frame.src = replUrl();

    /* The REPL says hello as soon as its script runs, well before the kernel
       is up.  Silence means it is a build from before the embedder API (or
       ?holweb= points somewhere unexpected), in which case the pane still
       works by hand -- it just cannot be driven from this page. */
    setTimeout(() => {
      if (!paneLive) setStatus('no answer from the REPL — older build?  Type into it directly.', true);
    }, 20000);
  }

  function openPane() {
    buildPane();
    paneWidth();
    document.documentElement.classList.add('holweb-open');
  }

  function closePane() {
    /* Keep the iframe: re-opening should not cost another kernel boot. */
    document.documentElement.classList.remove('holweb-open');
  }

  function installGrip(grip) {
    let dragging = false;
    grip.addEventListener('mousedown', (ev) => {
      ev.preventDefault();
      dragging = true;
      grip.classList.add('holweb-dragging');
      document.documentElement.classList.add('holweb-dragging');
    });
    document.addEventListener('mousemove', (ev) => {
      if (dragging) setPaneWidth(window.innerWidth - ev.clientX);
    });
    document.addEventListener('mouseup', () => {
      if (!dragging) return;
      dragging = false;
      grip.classList.remove('holweb-dragging');
      document.documentElement.classList.remove('holweb-dragging');
    });
    window.addEventListener('resize', () => { if (pane) paneWidth(); });
  }

  /* ---- 4.  Talking to the pane ------------------------------------------- */

  function send(msg) {
    openPane();
    if (!paneLive) { outbox.push(msg); return; }
    frame.contentWindow.postMessage(msg, '*');
  }

  window.addEventListener('message', (ev) => {
    if (!frame || ev.source !== frame.contentWindow) return;
    const m = ev.data;
    if (!m || typeof m !== 'object') return;
    if (m.holweb === 'hello') {
      paneLive = true;
      while (outbox.length) frame.contentWindow.postMessage(outbox.shift(), '*');
      return;
    }
    if (m.holweb !== 'status') return;
    const queued = m.queued ? ' (+' + m.queued + ' queued)' : '';
    if (m.state === 'booting') setStatus('booting the kernel… (1–2 min)' + queued, true);
    else if (m.state === 'busy') setStatus('running…' + queued, true);
    else setStatus('ready', false);
  });

  /* ---- 5.  Wiring the clicks -------------------------------------------- */

  function run(code, insertOnly) {
    send({ holweb: insertOnly ? 'insert' : 'eval', src: code });
  }

  /* Shift+click is also a selection gesture: it extends the selection from the
     previous caret, which in Firefox tripped the guard below on every
     Shift+click after a run.  Cancelling the default on mousedown stops the
     selection ever starting, so the click comes through clean. */
  document.addEventListener('mousedown', (ev) => {
    if (!(ev.shiftKey || ev.altKey) || !ev.target.closest) return;
    if (ev.target.closest('.holweb-phrase, .holweb-runblock')) ev.preventDefault();
  });

  document.addEventListener('click', (ev) => {
    const insertOnly = ev.shiftKey || ev.altKey;
    const btn = ev.target.closest('.holweb-runblock');
    if (btn) { run(btn.dataset.holwebCode, insertOnly); return; }
    const span = ev.target.closest('.holweb-phrase');
    if (!span) return;
    /* Don't hijack a click that ended a text selection, unless it carried a
       modifier and so said plainly what it wants. */
    if (!insertOnly && String(window.getSelection())) return;
    run(span.dataset.holwebCode, insertOnly);
  });

  document.addEventListener('keydown', (ev) => {
    if (ev.key !== 'Enter' && ev.key !== ' ') return;
    const span = ev.target.closest && ev.target.closest('.holweb-phrase');
    if (!span) return;
    ev.preventDefault();
    run(span.dataset.holwebCode, ev.shiftKey || ev.altKey);
  });

  /* ---- 6.  Boot ---------------------------------------------------------- */

  const found = decorateAll();
  if (found.phrases) {
    const launch = document.createElement('button');
    launch.type = 'button';
    launch.id = 'holweb-launch';
    launch.textContent = 'Open HOL Light in a web browser';
    launch.title = 'Run the tutorial’s examples for real, in a pane on the right.\n'
                 + 'Then click any “#” line to send it to HOL Light.';
    launch.addEventListener('click', () => openPane());
    document.body.appendChild(launch);
  }
})();
