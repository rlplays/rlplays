/*******************************************************************************************
 *
 *   raylib game template
 *
 *   RLPlays Game
 *   A playable 2D pixel platformer game with an editor.
 *   Mainly to explore and exploit RL.
 *
 *   This game has been created using raylib (www.raylib.com)
 *   raylib is licensed under an unmodified zlib/libpng license (View raylib.h for details)
 *
 *   Copyright (c) 2021 Ramon Santamaria (@raysan5)
 *   Copyright (c) 2025 Perumaal Shanmugam (@perumaal_s)
 *
 ********************************************************************************************/

#include "main_game.h"
#include "raylib.h"
#include <world_fileset.h>
#include <raylib_utils.h>
#include <rl_utils.h>

#include <utility>
using namespace RLPlays;
using namespace std::filesystem;

//! @brief Public data dir used for a single world file as an output dir (alldata/ is the source of all our levels etc).
inline std::string GetPublicDataDir() { return GetRootGameDir() + "/data/"; }
inline std::string GetResourcesDataDir() { return GetRootGameDir() + "/gameui/src/resources/"; }

struct TConverterConfig
{
  std::shared_ptr<TWorldFiles> WorldFiles;
  TWorldFile* WorldMetaFile;
  std::unordered_set<std::string> ContentFiles;
};

static void ProcessFile(TConverterConfig& config)
{
  RLPlays::SetupGlobal();

  TGameLoadInfo loadInfo = {.Filename = config.WorldMetaFile->Filename, .UseCachedData = true};
  {
    const auto& worldFiles = config.WorldFiles;
    auto gameInfo = LoadGame(*config.WorldFiles, loadInfo);
    TLOG(LOG_DEBUG, "Processing file: %s\n", config.WorldMetaFile->Filename.c_str());
    //UpdateFrame(gameInfo);

    // For RL training, we need a reward, a terminal state and a thing that can plug into the actions (Player).
    if (IsValidRLPlays(gameInfo.Game))
    {
      if (worldFiles->RLTrain == nullptr) { worldFiles->RLTrain = std::make_shared<TRLTrain>(); }
      config.WorldMetaFile->SupportsRLTraining = true;
      FillRLStuff(gameInfo.Game, worldFiles->RLTrain, config.WorldMetaFile);
    }

    // Save automatically converts the file.
    // To add a converter, simply add the code to the block's Convert().
    LoadContent(gameInfo);
#if DEBUG
    for (auto& filename : THeadless::ContentFilenames)
    {
      TLOG(LOG_INFO, "  Content: %s", filename.c_str());
    }
#endif
    config.ContentFiles.insert(THeadless::ContentFilenames.begin(), THeadless::ContentFilenames.end());
    SaveGame(gameInfo);
    UnloadGame(gameInfo);
  }

  // Load again.
  {
    auto gameInfo = LoadGame(*config.WorldFiles, loadInfo);
    //UpdateFrame(gameInfo);
    UnloadGame(gameInfo);
  }
}

static void UpdateRL(const std::shared_ptr<TWorldFiles> worldFiles, const int prevMaxCells)
{
  if (worldFiles->RLTrain == nullptr) { worldFiles->RLTrain = std::make_shared<TRLTrain>(); }
  auto train = worldFiles->RLTrain;
  if (prevMaxCells != train->MaxNumCells)
  {
    TLOG(LOG_WARNING, "**** WARNING: RL Training max cells changed from %d to %d  ****",
      prevMaxCells, train->MaxNumCells);
    //rlWorldFiles->RLTrain->MaxNumCells = prevMaxCells;
    auto config = TConfig(ConfigToTrainFilepath());
    config.SetInt("env", "num_obs", GetNumObs(train)).SaveFile();
  }
  std::vector<string> newCurriculumList{};
  for (auto& rlFile : train->CurriculumList)
  {
    for (auto& file : worldFiles->Files)
    {
      if (file.SupportsRLTraining)
      {
        if (rlFile == file.Filename)
        {
          newCurriculumList.push_back(file.Filename);
          break;
        }
      }
    }
  }
  train->CurriculumList = newCurriculumList;

  worldFiles->Save();
  TLOG(LOG_INFO, "(RL Training files) Saved to %s", worldFiles->WorldFileSetName.c_str());
  TLOG(LOG_INFO, " ---------------------------------------- ");
}


static void InitPublicDataDir()
{
  const std::string dirName = GetPublicDataDir();
  if (dirName.find(GetRootGameDir()) == 0)
  {
    remove_all(dirName);
    create_directory(dirName);
  }
  else
  {
    TLOG(LOG_WARNING, "Unable to remove and recreate public data dir: %s (may be invalid).", dirName.c_str());
  }
}

//! @brief Creates the given {@param fullpath} and all its parent directories if they do not exist.
bool EnsureCreateDirectory(std::filesystem::path fullpath)
{
  if (fullpath.has_parent_path())
  {
    const auto parentPath = fullpath.parent_path();
    if (!exists(parentPath))
    {
      if (!EnsureCreateDirectory(parentPath))
      {
        return false;
      }
    }
  }

  if (!exists(fullpath))
  {
    create_directory(fullpath);
    return true;
  }
  return false;
}

//! @brief Copies {@param sourceRelFilePath} (relative to data dir) to {@param targetDir} (absolute path).
void CopyFileToDir(const std::string& sourceRelFilePath, const std::string& targetDir, const std::string& targetRelFilePath = "")
{
  auto sourcePath = GetDataDir() + sourceRelFilePath;
  auto targetPath = targetDir + (targetRelFilePath.empty() ? sourceRelFilePath : targetRelFilePath);
  auto targetSubDir = path(targetPath).parent_path();
  EnsureCreateDirectory(targetSubDir);
  if (!exists(targetSubDir))
  {
    throw std::runtime_error("Unable to create dirs for: " + targetPath);
  }
  copy_file(sourcePath, targetPath, copy_options::overwrite_existing);
}

//! @brief Copies all the selected content files provided in the config from the data dir to the given target dir.
void CopyFilesToDir(TConverterConfig& config, std::string targetDir)
{
  auto& worldFiles = config.WorldFiles;
  if (targetDir.find(GetRootGameDir()) == std::string::npos)
  {
    TLOG(LOG_WARNING, "Unable to use public data dir: %s (may be invalid).", targetDir.c_str());
  }

  auto newWorldFileSetname = "worlds.json";
  TWorldFiles newWorldFiles = {
    worldFiles->SelectedFile, worldFiles->FPS, {worldFiles->SelectedFile}, {}, newWorldFileSetname
  };
  auto worldsDir = targetDir + "worlds/";
  create_directory(worldsDir);
  newWorldFiles.Save(worldsDir);

  TLOG(LOG_INFO, "** Copying %d content files to %s", (int)config.ContentFiles.size(), targetDir.c_str());
  config.ContentFiles.insert(GetGameLevelsDir() + worldFiles->SelectedFile.Filename);
  for (auto contentFile : config.ContentFiles)
  {
    auto dataDirPos = contentFile.find(GetDataDir());
    if (dataDirPos == std::string::npos)
    {
      TLOG(LOG_WARNING, "Invalid file %s - not in source data dir", contentFile.c_str());
      continue;
    }

    contentFile = contentFile.substr(GetDataDir().size());
    CopyFileToDir(contentFile, targetDir);
  }
}

void CopyFilesToPublicDir(TConverterConfig& config)
{
  CopyFilesToDir(config, GetPublicDataDir());
  CopyFilesToDir(config, GetResourcesDataDir());
}

void CopyFileToTargetDirs(const std::string& sourceRelFilePath, std::string targetRelFilePath = "")
{
  CopyFileToDir(sourceRelFilePath, GetPublicDataDir(), targetRelFilePath);
  CopyFileToDir(sourceRelFilePath, GetResourcesDataDir(), targetRelFilePath);
}

void CopyRLFiles(const std::shared_ptr<TWorldFiles>& worldFiles)
{
  for (const auto& weight : worldFiles->RLTrain->Weights)
  {
    if (weight.Filename == worldFiles->RLTrain->PreferredWeight.Filename)
    {
      worldFiles->RLTrain->PreferredWeight = weight;
      CopyFileToTargetDirs(weight.Filename, TRAINED_MODEL_FILE_RELPATH);
      CopyFileToTargetDirs(weight.Config, TRAINED_CONFIG_FILE_RELPATH);
      TLOG(LOG_INFO, "Copying preferred RL weight %s to %s", worldFiles->RLTrain->PreferredWeight.Filename.c_str(), TRAINED_MODEL_FILE_RELPATH);
      break;
    }
  }

}


static void ProcessFiles(const std::string& fileset)
{
  const auto worldFiles = TWorldFiles::Load(fileset);
  const auto selectedFilename = worldFiles->SelectedFile.Filename;
  TLOG(LOG_INFO, "Loading world files from %s", worldFiles->WorldFileSetName.c_str());
  TLOG(LOG_INFO, "Found %d world files, loading %s @ %d FPS", worldFiles->Files.size(),
    selectedFilename.c_str(), worldFiles->FPS);

  if (worldFiles->RLTrain == nullptr) { worldFiles->RLTrain = std::make_shared<TRLTrain>(); }
  const auto prevMaxCells = worldFiles->RLTrain->MaxNumCells;
  std::unordered_set<std::string> allContentFiles;
  auto sortedFiles = worldFiles->Files;
  std::sort(sortedFiles.begin(), sortedFiles.end(),
    [](const TWorldFile& a, const TWorldFile& b) { return a.Filename < b.Filename; });

  InitPublicDataDir();
  worldFiles->RLTrain->MaxNumCells = 0;
  // worldFiles->RLTrain->MaxNumSecondsToTrain = 0;
  for (auto& file : worldFiles->Files)
  {
    TLOG(LOG_INFO, "Processing file: %s", file.Filename.c_str());
    TConverterConfig config = {worldFiles, &file, {}};
    ProcessFile(config);
    if (file.Filename == worldFiles->SelectedFile.Filename)
    {
      CopyFilesToPublicDir(config);
    }
  }

  UpdateRL(worldFiles, prevMaxCells);
  CopyRLFiles(worldFiles);
  worldFiles->Save();
  TLOG(LOG_INFO, "Saved to %s", worldFiles->WorldFileSetName.c_str());
}

int main(int argc, char** argv)
{
  THeadless::TrackContentFiles = true;
  RLPlays::SetupGlobal();

  ProcessFiles(DefaultWorldFilesetName);

#ifdef RLPLAYS_WAIT_AFTER_RUN
  auto _ = getchar();
#endif

  return 0;
}
