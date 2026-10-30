# Savestates

Reluva's own saves, for any engine that supports them, whether or not the
game had saving of its own. They're Reluva's feature, kept apart from each
game's behaviour.

## Using them

- **Ctrl+F5** in a game opens the Reluva menu over it: **Resume**,
  **Save...**, **Load...**, **Return to Launcher**, **Quit Reluva**. The
  game's clock and sound stop while it's open. Escape (or Ctrl+F5 again)
  closes it.
- **Save...** lists the slots (01-99), with a new one first. Pick one, edit
  the description if you like (the game suggests one: New Worlds gives the
  mission, the race and the mission time), then **Save**. Saving over a
  slot asks first. **Delete** removes one.
- **Load...** lists the saves, newest picked, with each one's thumbnail,
  description and date. **Load** (or a double click) puts the game back as
  it was and closes the menu. A save that turns out damaged part way
  through leaves the game somewhere sound (New Worlds starts the mission
  over), and the menu's message says so after what was wrong; if the game
  can't manage that, the message says that instead, and Resume and Save
  stay greyed out until another save loads.
- The launcher's **Load...** button lists the selected game's saves and
  starts the game from one. `reluva <target> --load <slot>` does the same
  from the command line.
- Save and Load are greyed out when the game can't save now (hover for
  why). In New Worlds that means outside a mission being played: you
  save during play started with **G** or **F3** on the missions page, not
  during a briefing, a movie or the mission report.

Saves live beside the settings, a folder per game (its launcher name):
`~/.local/share/reluva/reluva/saves/<target>/slotNN.rlvs` on Linux,
`%APPDATA%\reluva\reluva\saves\<target>\` on Windows.

**While the engines are still being made to match their games, an update
will often stop older saves loading** (the menu says so); they're refused
rather than misread.

## The file

Little-endian throughout:

| Part | What |
|---|---|
| `RLVS` | magic |
| u32 | the file format's version (1) |
| u32 + bytes | the header: engine id, game id, target, the engine's state version, description, creation time (seconds since 1970, UTC), Reluva's version, thumbnail width and height, the thumbnail (RGBA, at most 160 × 120) |
| u32 + bytes | the engine's state |
| u32 | CRC-32 of the state |

Strings and lists are a u32 count and their contents (a string's bytes,
and a byte list's, as they are; numbers little-endian on any host). A
file is written beside the old one first and then moved over it, so a
failed save leaves the old one. The header can be read alone (for the lists).

## For an engine

An engine opts in through five virtual functions in `Engine`
(`src/engines/engine.h`):

- `canSaveState(why)` / `canLoadState(why)`: whether it can now, and why
  not.
- `stateVersion()`: goes up whenever the state's layout changes; a save of
  another version is refused.
- `stateDescription()`: the description a new save is given.
- `syncState(Serializer&)`: lists its state once. The same code saves and
  loads (`s.saving()` / `s.loading()` say which), so the two can't drift
  apart.
- `loadFailed()`: called when a load fails once `syncState` has begun
  reading: its own error, the serializer not `ok()` after it, or bytes
  left over (`!atEnd()`). The state may be half-read; the engine puts
  itself somewhere sound (its menu, the mission from its start) and
  returns true, setting `done` to what it did ("The mission starts
  over.") unless its own error already said; the menu shows that after
  the error. False (the default) says it can't: the menu then says so,
  and won't resume or save until a load succeeds, offering only Load,
  Return to Launcher and Quit.

`save::Serializer` (`src/common/savestate.h`) handles numbers, enums,
strings and the standard containers (vector, deque, array, map,
unordered_map, set, optional, unique_ptr, pair, C arrays) of anything it
handles. A struct gives a `syncState(save::Serializer&, T&)` (a friend
function beside its fields is easiest), and `s(a)(b)(c)` syncs fields in
turn. `save::syncIndex` saves a pointer into a table the engine rebuilds
(the game's data) as its index. A loader that runs out of bytes, or reads
a count longer than what's left (each element taking at least the bytes
its type is written in), fails instead of reading past the end; check
`s.ok()`. Lists are grown as their elements are read, so a damaged count
costs memory only for the elements really there. An enum is read as any
value of its type: where one out of range would do harm (a `switch`, an
index), sync it with `save::syncEnum(s, v, Last)`, which fails a load
whose value isn't 0 .. `Last`, or check it after loading.

The menu calls them from inside `System::pollEvent()`, between the
engine's frames, so the engine's own loop needs nothing new. A load
should rebuild what the save doesn't hold (data from the game's files,
caches, sound handles) the way starting does, then overwrite what play
changed.

## New Worlds

A load starts the saved mission as play starts it: the map, the colony's
layout, the sides and their AI, the mission script, from the game's data.
Then it overwrites everything play changes. That covers the world (every
unit, shot and group), each colony's economy (its bees, pods, crew, rides,
power and life support), the AI's running orders, the fog, repairs, field
work (scans, captures, officers, courses, raids), research, the script's
effects, the mission script's state, and the missions page's messages,
camera and Cadet mode. It also keeps the colony capacity the mission
started from (what the session carried in from the missions played
before it, [exe.md](formats/newworlds/exe.md#the-economy)): a load puts
that back as the session's value, so a restart of the loaded mission
begins from it, and the next mission from what the loaded one ends with. Its random numbers are seeded counters, saved with
the rest, so a mission loaded carries on exactly as it would have. A load
that fails once the mission has started (a damaged save, bytes left over)
starts that mission over from its beginning, as whom it was saved, and
the menu's message ends "The mission starts over."; one that fails before
(a mission or map the game doesn't have) leaves the game as it was, or the
missions page with nothing playing.

Two checks keep that true as the code changes:

- `testSaveState` (`tests/test_newworlds_cat.cpp`) saves F1, K1 and R1
  after two minutes, loads each into a fresh mission, runs both three
  minutes more, and checks the two states are byte for byte the same.
- `reluva-tool play-check -r all <seconds> <file.cat>...` does the same on
  every mission's real map, from each side: it loads a save halfway
  through into a second run and checks both end the same. All 59
  mission-and-side runs pass at 600 seconds.

A save also records a hash of the mission's script, and is refused if the
script has changed since (a rule moved or edited, even with the same count).
The simulation runs in fixed 50 ms ticks, so a save plays on the same at
any frame rate.

A field added to the simulation without being added to its `syncState`
shows up there as a difference, if it changes how the mission plays. When
the layout changes, raise `kStateVersion` in `newworlds.cpp`.
