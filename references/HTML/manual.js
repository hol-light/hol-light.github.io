/* The behaviour of index.html: the entry list, the search box and the pane
   that shows one entry.  The entries themselves come from entries.js, which
   hlp2html.py generates from the .hlp files. */
(function () {
  'use strict';

  var data = window.HOL_LIGHT_ENTRIES;
  var FILEMAP = data.filemap;
  var ENTRIES = data.entries.map(function (e) {
    return {name: e[0], file: e[1], synopsis: e[2], seealso: e[3]};
  });

  var search = document.getElementById('search');
  var sidebar = document.getElementById('sidebar');
  var list = document.getElementById('entry-list');
  var count = document.getElementById('result-count');
  var pane = document.getElementById('entry');

  var HOME = '<h2>HOL Light Reference Manual</h2>' +
    '<p>Pick an entry from the list on the left, or search above. A search ' +
    'lists the entries whose name matches, and then the entries that name ' +
    'the match in their <SPAN CLASS=FIELD-NAME>SEE ALSO</SPAN> field.</p>' +
    '<p class="note">Press <kbd>/</kbd> to reach the search box and ' +
    '<kbd>Enter</kbd> to search it; then <kbd>&#8593;</kbd> and ' +
    '<kbd>&#8595;</kbd> walk the results and <kbd>Enter</kbd> opens one. ' +
    'The whole manual is also available as a ' +
    '<a href="../reference.pdf">single PDF</a>.</p>';

  /* Every link this page writes, and every link it rewrites, addresses an
     entry here rather than on its own page, so that opening one in a new tab
     or copying its address gives back the list and the search box too. */
  var ENTRY_HREF = 'index.html#';

  /* Same mapping from an identifier to its page as hlp2html.py does. */
  function nameToFile(name) {
    var mapped = FILEMAP[name] || name;
    return mapped.charAt(0) === '.' ? 'DOT_' + mapped.slice(1) : mapped;
  }

  var byFile = {};
  ENTRIES.forEach(function (e) { byFile[e.file] = e; });

  /* SEE ALSO, reversed: which entries point at a given page. */
  var mentionedBy = {};
  ENTRIES.forEach(function (e) {
    e.seealso.forEach(function (name) {
      var file = nameToFile(name);
      if (!mentionedBy[file]) mentionedBy[file] = [];
      if (mentionedBy[file].indexOf(e) < 0) mentionedBy[file].push(e);
    });
  });

  function escapeHtml(text) {
    return text.replace(/&/g, '&amp;').replace(/</g, '&lt;')
               .replace(/>/g, '&gt;');
  }

  function byName(a, b) {
    var x = a.name.toLowerCase(), y = b.name.toLowerCase();
    return x < y ? -1 : x > y ? 1 : 0;
  }

  /* Exact match first, then names starting with the query, then the rest. */
  function rank(name, query) {
    name = name.toLowerCase();
    if (name === query) return 0;
    return name.indexOf(query) === 0 ? 1 : 2;
  }

  function results(query) {
    var q = query.trim().toLowerCase();
    if (!q) return {names: ENTRIES, seealso: []};

    var names = ENTRIES.filter(function (e) {
      return e.name.toLowerCase().indexOf(q) >= 0;
    }).sort(function (a, b) {
      return rank(a.name, q) - rank(b.name, q) || byName(a, b);
    });

    var matched = {};
    names.forEach(function (e) { matched[e.file] = true; });
    var seealso = ENTRIES.filter(function (e) {
      return !matched[e.file] && e.seealso.some(function (name) {
        return name.toLowerCase().indexOf(q) >= 0;
      });
    }).sort(byName);

    return {names: names, seealso: seealso};
  }

  var links = [];      /* the rendered <a>s, in display order */
  var selected = -1;   /* index into links, for the arrow keys */
  var current = null;  /* page basename shown in the reading pane */
  var rendered = null; /* the query the list was built from */
  var countLabel = ''; /* what the line above the list normally says */

  function groupItem(text) {
    var li = document.createElement('li');
    li.className = 'group';
    li.textContent = text;
    return li;
  }

  function entryItem(entry, withSynopsis) {
    var li = document.createElement('li');
    var a = document.createElement('a');
    a.href = ENTRY_HREF + entry.file;
    a.setAttribute('data-file', entry.file);
    var name = document.createElement('span');
    name.className = 'name';
    name.textContent = entry.name;
    a.appendChild(name);
    if (withSynopsis && entry.synopsis) {
      var synopsis = document.createElement('span');
      synopsis.className = 'synopsis';
      synopsis.textContent = entry.synopsis;
      a.appendChild(synopsis);
    }
    if (entry.file === current) a.className = 'current';
    li.appendChild(a);
    links.push(a);
    return li;
  }

  function plural(n, word) {
    return n + ' ' + word + (n === 1 ? '' : 's');
  }

  /* Rebuild the list from whatever is in the search box.  This is deliberately
     not called while the user types: see commitSearch. */
  function render() {
    var found = results(search.value);
    var searching = search.value.trim() !== '';
    var group = document.createDocumentFragment();

    links = [];
    selected = -1;
    rendered = search.value;

    if (!searching) {
      countLabel = ENTRIES.length + ' entries';
      ENTRIES.forEach(function (e) { group.appendChild(entryItem(e, false)); });
    } else if (!found.names.length && !found.seealso.length) {
      countLabel = 'no match';
    } else {
      countLabel = plural(found.names.length, 'name') +
        (found.seealso.length ? ', ' + found.seealso.length + ' via SEE ALSO'
                              : '');
      if (found.names.length) {
        group.appendChild(groupItem('Names'));
        found.names.forEach(function (e) {
          group.appendChild(entryItem(e, true));
        });
      }
      if (found.seealso.length) {
        group.appendChild(groupItem('Mentioned in SEE ALSO of'));
        found.seealso.forEach(function (e) {
          group.appendChild(entryItem(e, true));
        });
      }
    }

    count.textContent = countLabel;
    list.textContent = '';
    list.appendChild(group);
  }

  /* Searching is a deliberate act: the list stands still while the query is
     typed and catches up on Enter, so it never shuffles under the pointer
     halfway through a name.  Returns whether there was anything to catch up
     on, which is how Enter tells "search this" from "open what I found". */
  function commitSearch() {
    if (search.value === rendered) return false;
    render();
    markCurrent();
    syncState();
    return true;
  }

  /* Meanwhile the line above the list admits that it is out of date. */
  function noteEdit() {
    count.textContent = search.value === rendered ? countLabel
                                                  : 'press Enter to search';
  }

  function markCurrent() {
    links.forEach(function (a) {
      var isCurrent = a.getAttribute('data-file') === current;
      a.classList.toggle('current', isCurrent);
      if (isCurrent && !search.value.trim()) {
        a.scrollIntoView({block: 'nearest'});
      }
    });
  }

  function select(step) {
    if (!links.length) return;
    selected += step;
    if (selected < 0) selected = links.length - 1;
    if (selected >= links.length) selected = 0;
    links.forEach(function (a, i) {
      a.classList.toggle('selected', i === selected);
    });
    links[selected].scrollIntoView({block: 'nearest'});
  }

  /* An entry page holds its content in a DIV that we lift out wholesale.
     hlp2html.py writes that DIV, and checks at build time that this selector
     still names it, so the two cannot drift apart unnoticed. */
  var ENTRY_SELECTOR = 'div.ENTRY';

  function extractEntry(html) {
    var parsed = new DOMParser().parseFromString(html, 'text/html');
    var body = parsed.querySelector(ENTRY_SELECTOR);
    return body ? body.innerHTML : null;
  }

  function backlinksHtml(file) {
    var refs = (mentionedBy[file] || []).slice().sort(byName);
    if (!refs.length) return '';
    return '<DL class="backlinks"><P><DT><SPAN CLASS=FIELD-NAME>' +
      'MENTIONED IN SEE ALSO OF</SPAN><DD>' +
      refs.map(function (e) {
        return '<A href="' + ENTRY_HREF + e.file + '">' +
          escapeHtml(e.name) + '</A>';
      }).join(', ') + '</DL>';
  }

  var cache = {};

  /* The SEE ALSO links arrive as an entry page wrote them, pointing at other
     entry pages; here they should lead back into this page instead.  The entry
     pages themselves keep their own links, so they stay self-contained. */
  function relinkEntry() {
    var anchors = pane.querySelectorAll('a[href]');
    for (var i = 0; i < anchors.length; i++) {
      var page = /^([^\/?#]+)\.html$/.exec(anchors[i].getAttribute('href'));
      if (page && page[1] !== 'index') {
        anchors[i].setAttribute('href', ENTRY_HREF + page[1]);
      }
    }
  }

  function paint(file, body) {
    pane.innerHTML = body + backlinksHtml(file);
    relinkEntry();
    pane.scrollTop = 0;
  }

  function openEntry(file) {
    current = file;
    markCurrent();

    var entry = byFile[file];
    if (!entry) {
      document.title = 'HOL Light reference manual';
      pane.innerHTML = '<p class="note">This manual has no entry named <SPAN ' +
        'CLASS=BRKT>' + escapeHtml(file) + '</SPAN>.</p>';
      return;
    }

    document.title = entry.name + ' — HOL Light reference manual';
    if (cache[file]) { paint(file, cache[file]); return; }

    pane.innerHTML = '<p class="note">Loading…</p>';
    fetch(file + '.html').then(function (response) {
      if (!response.ok) throw new Error(response.status);
      return response.text();
    }).then(function (html) {
      var body = extractEntry(html);
      if (body === null) throw new Error('no entry div');
      cache[file] = body;
      if (current === file) paint(file, body);
    }).catch(function () {
      if (current !== file) return;
      pane.innerHTML = '<p class="note">Could not load <A href="' + file +
        '.html">' + file + '.html</A>. Reading the manual straight off the ' +
        'file system stops this page from fetching entries; serve the ' +
        'directory over HTTP (<SPAN CLASS=BRKT>python3 -m http.server</SPAN>) ' +
        'or follow the link above.</p>';
    });
  }

  function showHome() {
    current = null;
    markCurrent();
    document.title = 'HOL Light reference manual';
    pane.innerHTML = HOME;
    pane.scrollTop = 0;
  }

  /* Where the whole page stands is the entry on show and the query the list
     was built from.  The fragment carries the entry, since that is what links
     and bookmarks address; the query travels in the history entry's state, out
     of sight of the address bar.  Both together mean that going back restores
     the list beside the pane and not just the pane. */
  var canPush = !!(window.history && history.pushState);

  function hashFile() {
    return decodeURIComponent(location.hash.replace(/^#/, ''));
  }

  function stateUrl(file) {
    return location.pathname + location.search + (file ? '#' + file : '');
  }

  /* Record the query on the history entry we are already on, without adding
     one: searching is not a step you go back through, but it does change what
     coming back here should show. */
  function syncState() {
    if (canPush) {
      history.replaceState({file: current, query: rendered}, '',
                           stateUrl(current));
    }
  }

  /* Put the page into a given state, from a history entry or from the URL. */
  function show(state) {
    var query = (state && state.query) || '';
    if (query !== search.value || query !== rendered) {
      search.value = query;
      render();
    }
    var file = state ? state.file : null;
    if (file) openEntry(file); else showHome();
  }

  /* Reached from the fragment alone -- a bookmark, a typed URL, a history
     entry from before this script took over -- so the query starts empty. */
  function stateFromUrl() {
    return {file: hashFile() || null, query: ''};
  }

  /* Go somewhere, adding a step to the history unless we are already there. */
  function goTo(state) {
    var here = canPush ? history.state : null;
    var same = here && here.file === state.file && here.query === state.query;
    if (canPush && !same) {
      history.pushState(state, '', stateUrl(state.file));
    }
    show(state);
  }

  function navigate(file) {
    if (canPush) goTo({file: file, query: rendered});
    else location.hash = '#' + file;  /* hashchange picks it up */
  }

  /* The title in the corner is the way back to the page as it loads: no
     entry, no query, the whole list from the top. */
  function goHome() {
    sidebar.scrollTop = 0;
    if (canPush) {
      goTo({file: null, query: ''});
    } else {
      search.value = '';
      render();
      location.hash = '';
    }
  }

  /* Put a name in the search box, so that the list beside the entry becomes
     that name's neighbourhood: the name itself, then everything that mentions
     it.  ``a`` supplies the name for the handful of SEE ALSO references that
     no entry provides a page for; searching for those still finds the entries
     that name them. */
  function searchFor(file, a) {
    var entry = byFile[file];
    search.value = entry ? entry.name : a.textContent.trim();
    render();
  }

  /* Keep links to other entries inside this page instead of leaving it. */
  function handleClick(event) {
    if (event.button !== 0 || event.metaKey || event.ctrlKey ||
        event.shiftKey || event.altKey) return;
    var a = event.target.closest ? event.target.closest('a') : null;
    if (!a || (!list.contains(a) && !pane.contains(a))) return;
    /* Only links into this page, with or without an entry's fragment.  Any
       other link -- the PDF, or an entry page named by the message that
       appears when an entry cannot be fetched -- is the browser's business. */
    var target = /^index\.html(?:#(.*))?$/.exec(a.getAttribute('href') || '');
    if (!target) return;
    event.preventDefault();
    if (!target[1]) { goHome(); return; }
    var file = decodeURIComponent(target[1]);
    /* Only the links in the entry itself, which are its SEE ALSO references
       and the entries mentioning it.  A click in the list must leave the
       search box alone: that query is what put the link there. */
    if (pane.contains(a)) searchFor(file, a);
    navigate(file);
  }

  search.addEventListener('input', noteEdit);
  /* An INPUT type=search has its own clear button, which empties the box
     without a keystroke; the browser reports that here. */
  search.addEventListener('search', function () { commitSearch(); });
  search.addEventListener('keydown', function (event) {
    if (event.key === 'ArrowDown' || event.key === 'ArrowUp') {
      /* Walking the results implies searching for them first. */
      event.preventDefault();
      commitSearch();
      select(event.key === 'ArrowDown' ? 1 : -1);
    } else if (event.key === 'Enter') {
      /* Enter searches; once the list is showing the query, Enter again opens
         the result the arrow keys are on, or the first one. */
      event.preventDefault();
      if (commitSearch()) return;
      var a = links[selected < 0 ? 0 : selected];
      if (a) navigate(a.getAttribute('data-file'));
    } else if (event.key === 'Escape') {
      search.value = '';
      commitSearch();
    }
  });

  document.addEventListener('keydown', function (event) {
    if (event.key === '/' && event.target !== search) {
      event.preventDefault();
      search.focus();
      search.select();
    }
  });

  list.addEventListener('click', handleClick);
  pane.addEventListener('click', handleClick);
  document.getElementById('top').querySelector('h1 a')
    .addEventListener('click', function (event) {
      event.preventDefault();
      goHome();
    });
  /* Back and forward: each of our own steps carries its state, so the list and
     the pane both go back to how they were.  A step made before this script
     ran, or by another page, has no state and starts from the fragment. */
  window.addEventListener('popstate', function (event) {
    show(event.state || stateFromUrl());
  });

  /* A fragment change we did not make -- a typed URL, or a link on another
     page.  pushState does not fire this, and a traversal that does fire it has
     already been dealt with above, hence the check. */
  window.addEventListener('hashchange', function () {
    if ((hashFile() || null) !== current) show(stateFromUrl());
  });

  render();
  /* A reload keeps the state of the entry it reloads, so it also keeps the
     query; a first visit has only the fragment to go on. */
  show((canPush && history.state) || stateFromUrl());
  syncState();
})();
