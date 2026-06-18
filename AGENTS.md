# Project structure

This is a game demo with an editor with support for training the player using RL.

## Creating new characters/blocks

The project relies on being able to create new blocks and characters with a specific structure.

See [player_block.h](./game/plays/player_block.h) for an example.

To create a new character/block for `Blah`, here are the steps.
  - create a struct `T<Blah>Block` similar to `TPlayerBlock` in [player_block.h](./game/plays/player_block.h)
    - The new block should be in `blah_block.h` in [plays](./game/plays/).
  - create new enum in `TBlockType` in [tbloch.h](./game/plays/tblock.h) for `<Blah>` and add to `TBlock`'s SerializerDerived in tblock.h
   - Change `LastBlock` as needed.
   - If needed: modify `TBlockTraits` in [interactions.h](./game/plays/interactions.h) to add new enum. Otherwise, reuse existing enum such as `Solid` etc.
   - IMPORTANT: If you change the number of block traits, ensure `TBlockTraitsCount` is changed as well!
  - Implement basic init/rendering using `Init`/`Draw` (and `LoadContent` too).
  - Implement update logic to move the block/character in `Update`. 
  - The project uses:
    - `TCountdownTimer` for simple timer-based movement.
    - `TSceneTransitions` to stage step-by-step transitions.
  - If the character/block interacts with other elements, implement `Interact` and ensure the `TInteractionResult/TInteraction` are correctly used/filled.
  - Finally, add a template for this block in [`block_templates.h`](./game/plays/block_templates.h) following the examples there.

- Use [`TGrid`](./game/coreloop/include/grid.h) to find neighboring blocks within a certain region. Use `FindNeighborsWithinDistance` or `FindNeighbors` as needed. Use `grid.h` methods liberally as they are well tested.
  - Fix any issues in `grid.h/.cpp` methods as needed.

 



