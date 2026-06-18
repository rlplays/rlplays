#pragma once

#include <nlohmann/json.hpp>
#include <serialize.h>

#include <game_types.h>
#include <memory>
#include <string>

#include <atomic>
#include <mutex>
#include "game_actions.h"

// This struct helps you load a number of levels/maps and go through them (supporting curriculum training/self-play etc).

namespace RLPlays
{
//! @brief Default directory for the game inside the data directory.
static constexpr auto DefaultDir = "game/";

//! @brief Default filename for the game world.
static constexpr auto DefaultWorldFilename = "rlplays_level3.json";


// Forward declaration for json
using json = nlohmann::json;

static constexpr auto DefaultWorldFilesetName = "worlds.json";

struct TWorldFile
{
  std::string Filename;
  bool SupportsRLTraining = false;
  int NumCells = 0;
  int NumCellTypes = 0;

  // RL stuff here:
  //! @brief How many times should we try self-play training? (Only if mirror mode + other agent is available).
  int NumSelfPlayTraining = 0;

  //! @brief Whether this file supports self-play training (requires mirror mode + other agent etc). This is filled by the converter.
  bool SupportsSelfPlay = false;

  //! @brief Number of times to learn from this specific level/map (as a multiplier of max success training).
  float TrainingMultiplier = 1.0f;
  Serializer(TWorldFile, Filename, SupportsRLTraining, NumCells, NumCellTypes, NumSelfPlayTraining, SupportsSelfPlay, TrainingMultiplier)
};

struct TRLWeights
{
  std::string Filename; // Of the form "models/rlplays_weights_blah.bin"
  std::string Config;   // Of the form "models/rlplays_config_blah.ini"
  std::string Comment;  // Optional comment about the weights/config for human reference.
  Serializer(TRLWeights, Filename, Config, Comment)
};

//! @brief RL training config across all world files. Contains the syllabus/curriculum and other training-related
//! config.
//!        This is filled during the Converter process.
struct TRLTrain
{
  static inline int WEIGHT_FILE_INDEX = 0;
  int MaxNumCells = 10;
  int CapMaxCells = 200;
  int MaxNumSecondsToTrain = 45;
  int MaxNumSuccessfulTrainingPerFile = 15;
  int MaxNumFailureTrainingPerFile = 15 * 1000 * 1000;

  TRLWeights PreferredWeight = {"models/rlplays_weights_2026_03_14.bin", "models/rlplays_config_2026_03_14.ini",
                                "Mar 14, 2026 - pretty good initial model."};
  std::vector<TRLWeights> Weights = {};

  //! @brief A sorted list of files to train on starting from easiest to toughest as a form of ladder-climbing syllabus.
  std::vector<std::string> CurriculumList = {};

  Serializer(TRLTrain, MaxNumCells, CapMaxCells, MaxNumSecondsToTrain, MaxNumSuccessfulTrainingPerFile,
             MaxNumFailureTrainingPerFile, CurriculumList, PreferredWeight, Weights)
};


//
// Directory-related functions go here.
//

//! @brief The root game/ directory (if it exists) e.g. /blah/rlplays/game without the trailing forward slash.
//!        On Web, returns "".
const std::string& GetRootGameDir();

//! @brief The data (rlplays/game/data) directory path for our game (works on *Nix/Windows) with the trailing forward
//! slash.
const std::string& GetDataDir();

constexpr auto GAME_LEVELS_SUBDIR = "game/";

//! @brief The levels directory path for our game (works on *Nix/Windows).
const std::string& GetGameLevelsDir();

//! @brief Returns the absolute path to <datadir>/worlds/.
const std::string& GetWorldsDir();

static const char* TRAINED_CONFIG_FILE_RELPATH = "models/rlplays_config.ini";
static const char* TRAINED_MODEL_FILE_RELPATH = "models/rlplays_weights.bin";

//! @brief (For RL Training) Contains the final configuration from the training pointing to the model/weights.
inline std::string TrainedConfigFilepath() { return GetDataDir() + TRAINED_CONFIG_FILE_RELPATH; }
inline std::string TrainedModelPath() { return GetDataDir() + TRAINED_MODEL_FILE_RELPATH; }
inline std::string TrainedModelPathWith(int index, const std::shared_ptr<TRLTrain>& rlTrain)
{
  if (!rlTrain || rlTrain->Weights.empty())
  {
    return TrainedModelPath();
  }
  if (index < 0 || index >= static_cast<int>(rlTrain->Weights.size()))
  {
    index = 0;
  }
  return GetDataDir() + rlTrain->Weights[index].Filename;
}

struct TWorldFiles
{
  static std::shared_ptr<TWorldFiles> CachedDefault;
  static std::mutex CachedDefaultMutex;
  TWorldFile SelectedFile = {DefaultWorldFilename};
  int FPS = 60;
  std::vector<TWorldFile> Files;
  std::shared_ptr<TRLTrain> RLTrain;
  std::string WorldFileSetName = DefaultWorldFilesetName;
  Serializer(TWorldFiles, Files, FPS, SelectedFile, RLTrain, WorldFileSetName)

    std::string GetFile(const int fileIndex)
  {
    if (fileIndex < 0 || fileIndex >= Files.size())
    {
      return SelectedFile.Filename;
    }
    return Files[fileIndex].Filename;
  }

  TWorldFile* GetWorldFileRef(const std::string filename) const
  {
    for (auto& file : Files)
    {
      if (file.Filename == filename)
      {
        return const_cast<TWorldFile*>(&file);
      }
    }
    return nullptr;
  }

  void Save(std::string worldsDir = GetWorldsDir())
  {
    const auto filename = worldsDir + WorldFileSetName;
    if (!Files.empty())
    {
      bool foundSelected = false;
      for (const auto& file : Files)
      {
        if (file.Filename == SelectedFile.Filename)
        {
          foundSelected = true;
          SelectedFile = file;
          break;
        }
      }
      if (!foundSelected)
      {
        SelectedFile = Files[Files.size() - 1];
      }
    }
    std::ofstream os(filename);
    const json data(*this);
    os << data.dump(2);
    os.close();
  }

  std::vector<std::string> GetFiles(const char* matchSubstr) const
  {
    std::vector<std::string> files;
    for (const auto& file : Files)
    {
      if (file.Filename.find(matchSubstr) != std::string::npos)
      {
        files.push_back(file.Filename);
      }
    }
    return files;
  }

  static void ClearCache()
  {
    std::lock_guard<std::mutex> lock(CachedDefaultMutex);
    CachedDefault = nullptr;
  }

  static std::shared_ptr<TWorldFiles> Load(std::string fileset = "", const bool useCached = true)
  {
    std::lock_guard<std::mutex> lock(CachedDefaultMutex);
    if (fileset.empty())
    {
      fileset = DefaultWorldFilesetName;
    }
    if (useCached && fileset == DefaultWorldFilesetName && CachedDefault != nullptr)
    {
      return TWorldFiles::CachedDefault;
    }
    auto filename = GetWorldsDir() + fileset;
    std::shared_ptr<TWorldFiles> worldFiles = {};
    if (FileExists(filename.c_str()))
    {
      std::ifstream is(filename);
      const auto data = json::parse(is);
      worldFiles = std::make_shared<TWorldFiles>(data.template get<TWorldFiles>());
      worldFiles->WorldFileSetName = fileset;
    }
    // For some reason, if this is the first time or if we change the filename, let's get all the files back.
    if (worldFiles == nullptr)
    {
      worldFiles = std::make_shared<TWorldFiles>();
    }
    if (fileset == DefaultWorldFilesetName)
    {
      worldFiles->DiscoverAllFiles();
      if (useCached)
      {
        TWorldFiles::CachedDefault = worldFiles;
      }
    }

    return worldFiles;
  }

  void DiscoverAllFiles()
  {
    std::vector<TWorldFile> filesCopy = Files;
    Files.clear();
    std::string dataDir = GetGameLevelsDir();
    NormalizeSlash(dataDir);
    AddFilesInSubDirs_(dataDir, dataDir, filesCopy);
    // Sort files alphabetically by filename for consistent display
    std::sort(Files.begin(), Files.end(),
              [](const TWorldFile& a, const TWorldFile& b) { return a.Filename < b.Filename; });
  }

private:
  void AddFilesInSubDirs_(const std::string& rootDir, const std::string& dir, std::vector<TWorldFile>& previousFiles)
  {
    for (const auto& entry : std::filesystem::directory_iterator(dir))
    {
      if (entry.is_regular_file())
      {
        std::string filename = entry.path().string();
        NormalizeSlash(filename);
        const auto pos = filename.find(rootDir);
        if (pos != 0)
        {
          continue;
        }
        filename = filename.substr(rootDir.size());
        if (entry.path().extension() == ".json")
        {
          bool foundExisting = false;
          for (const auto& existingFile : previousFiles)
          {
            if (existingFile.Filename == filename)
            {
              Files.push_back(existingFile);
              foundExisting = true;
              break;
            }
          }
          if (!foundExisting)
          {
            Files.push_back({filename});
          }
        }
      }
      else if (entry.is_directory())
      {
        AddFilesInSubDirs_(rootDir, entry.path().string(), previousFiles);
      }
    }
  }

  //! @brief Replaces the backward slash with forward slash that works on Win/*Nix.
  static void NormalizeSlash(std::string& s)
  {
    for (auto& c : s)
    {
      if (c == '\\')
        c = '/';
    }
  }
};
} // namespace RLPlays
