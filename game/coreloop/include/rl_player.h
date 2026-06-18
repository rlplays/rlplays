#pragma once
#include "world_fileset.h"

namespace RLPlays
{
// This separates the state of the RL player from the rest of the game context.
// This also avoids cyclic dependency as the RL env needs the context etc.
// While also pushing all the complexity straight into the single rl_player.cpp.
// This is also a RAII style object while the TRLPlayer is more of a 'load/unload' style object.
struct TRLState;
struct RLWeights
{
  float* weights;
  int size;

  RLWeights(float* weights, int size) : weights(weights), size(size) {}
  ~RLWeights() { free(weights); }
};

struct TRLPlayer
{
  void SetupRLEnv(TContextPtr context, std::shared_ptr<TRLTrain> rlTrain, bool isRLPlayer, bool activePlayer, std::shared_ptr<RLWeights> shared_weights = nullptr);
  void HandleRLPlayerActions(TContextPtr context);
  void HandlePostUpdateActions(TContextPtr context);
  void DebugRender(TContextPtr context);
  void UnloadRLEnv(TContextPtr context);
  void UpdateSelfPlayWeights(std::shared_ptr<RLWeights> new_weights);
  bool IsRLPlayer() const { return isRLPlayer_; }

private:
  bool isRLPlayer_ = false;
  // Each player has their own state, for now it's one.
  TRLState* state_ = nullptr;
};
}
