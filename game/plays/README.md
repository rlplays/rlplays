This dir contains the core blocks that define the gameplay.

Define blocks with each block type having its own (Init)/Update/Draw stuff.
It automatically shows up in the editor and the rest of the game just picks it up (including the RL training).


Three simple rules to add a new `ABlock` derived type:
- ensure the new struct name is `TSomeNameBlock`
- add the enum `SomeName` value to `TBlockType` in [tblock.h](./tblock.h)
- add `SomeName` to the `SerializedDerived` section to the same [tblock.h](./tblock.h)

Next, for the fun stuff:
- `Init` to initialize traits correctly as the game relies on it (including for RL training)
- `LoadContent` to load any content (which may be skipped during RL training)
- `Draw` function for rendering
- `Update` to step each frame and do collision detection / movement / etc
(Optionally add `EditorDraw` and other `ABlock` derived virtual functions as needed)

Use an LLM to generate these blocks as these are fun elements that require a lot of mundane effort otherwise.
It's also easy to tweak them using an LLM and adjust timings, add scene transitions, fun confetti or blast animations etc.

See [`AGENTS.md`](../../AGENTS.md) for more details.