#pragma once
#include <raylib.h>
#include <log.h>
#include <unordered_set>
#include <string>
#include <cstdint>

namespace RLPlays
{
// Sometimes I run on a tiny portable monitor that measures only 1080p, so this is the closest 16:9 I could find.
static const int screenWidth = 1920;  // 1680
static const int screenHeight = 1080; // 945


//! @brief Obtains the monitor size and a good window size that sticks to the 16:9 asp ratio within the monitor.
inline void GetWindowSize(Vector2& monitor, Vector2& window)
{
  monitor.x = GetScreenWidth();
  monitor.y = GetScreenHeight();
  if (monitor.x < 100 || monitor.y < 100)
  {
    int currentMonitor = GetCurrentMonitor();
    monitor.x = GetMonitorHeight(currentMonitor);
    monitor.y = GetMonitorWidth(currentMonitor);
  }
  window.x = screenWidth;
  window.y = screenHeight;
  if (monitor.x > 100 && monitor.y > 100)
  {
    const int w = static_cast<int>(static_cast<double>(monitor.x) * 0.85);
    const int h = static_cast<int>(static_cast<double>(monitor.y) * 0.85);
    const auto aspRatio = static_cast<double>(screenWidth) / static_cast<double>(screenHeight);
    window.y = w / aspRatio;
    window.x = w;
    if (window.y > h)
    {
      window.x = h * aspRatio;
      window.y = h;
    }
  }
}

// Raylib graphics/sound functions that can be swapped with a noop impl based on the runtime.
// This also ensures performance of the final headless code (RL training/converters and tests) is good.
// Not creating any window/GL objects is also a huge plus.

struct THeadless
{
  //! @brief Headless mode skips all graphics rendering (raylib mostly and nullifies all content loading).
  static bool IsHeadless;

  //! @brief Skips scene transitions for RL training purposes. May be combined with IsHeadless.
  //! In particular, for RL demo-mode, we may have headless=false, hide scene transitions=true.
  static constexpr bool HideSceneTransitions =
#if RLPLAYS_HIDE_SCENE_TRANSITIONS
      true;
#else
      false;
#endif

  //! @brief Tracks content files in headless mode - traps all content loading and redirects to
  //! {@refitem TrackContentFiles} instead.
  static bool TrackContentFiles;

  // TODO: Not thread-safe but we only use it in a single-thread right now, so don't complicate this.
  static std::unordered_set<std::string> ContentFilenames;

  inline static void ClearContent() { if (TrackContentFiles) { ContentFilenames.clear(); } }

  inline static void AddContent(const std::string& filename)
  {
#if !defined(RLPLAYS_TRAIN)
    if (TrackContentFiles) { ContentFilenames.insert(filename); }
#endif
  }

  //! @brief If we are in headless or in 'hide scene transitions' mode, then skip input mode = true.
  inline static bool IsSkipInputMode() { return IsHeadless || HideSceneTransitions; }
};

constexpr uint32_t MakeBlockVersion(const uint32_t Major, const uint32_t Minor, const uint32_t Patch)
{
  return ((Major & 0xFF) << 24) | ((Minor & 0xFF) << 16) | (Patch & 0xFFFF);
}

struct TVersion
{
  // Follows Major (1byte), Minor (1byte), Patch (2bytes) versioning scheme.
  static constexpr uint8_t MajorVersion = 1;
  static constexpr uint8_t MinorVersion = 5;
  static constexpr uint16_t PatchVersion = 0;
  
  static constexpr uint32_t BlockVersion_1_4_0 = MakeBlockVersion(1, 4, 0);
  static constexpr uint32_t BlockVersion = MakeBlockVersion(MajorVersion, MinorVersion, PatchVersion);

  static const char* GetVersionStr()
  {
    static char versionBuffer[32] = {0}; // Static buffer to store formatted version string
    if (versionBuffer[0] == 0)           // Only format once
    {
      snprintf(versionBuffer, sizeof(versionBuffer), "%d.%d.%d",
        MajorVersion, MinorVersion, PatchVersion);
    }
    return versionBuffer;
  }
};

static void SetupGlobal()
{
#if defined(DEBUG_TRACE)
  // Uncomment this if needed during specific debugging needs (very spammy otherwise).
  //SetTraceLogLevel(LOG_TRACE);
#elif defined(DEBUG)
  SetTraceLogLevel(LOG_DEBUG);
#endif

#if defined(PLATFORM_WEB)
  TLOG(LOG_INFO, "(WASM) RLPlays Game. Version: %s", TVersion::GetVersionStr());
#else
  if (!THeadless::IsHeadless)
  {
    TLOG(LOG_INFO, "RLPlays Game. Version: %s", TVersion::GetVersionStr());
  }
#endif
}

inline void DisplayProgramInfo()
{
  Vector2 monitorSize;
  Vector2 windowSize;
  GetWindowSize(monitorSize, windowSize);
  auto compileFlags = std::string("[") +
#ifdef RLPLAYS_EDITOR
      "(EDITOR) " +
#endif
#ifdef DEBUG
      "(DEBUG) " +
#else
      "(RELEASE) " +
#endif
#ifdef PLATFORM_WEB
      "(WEB) " +
#endif
      "] ";
  auto infoStr = std::string("Starting with: ") + compileFlags +
      "(Screen: " + std::to_string(monitorSize.x) + "x" + std::to_string(monitorSize.y) +
      "/Window: " + std::to_string(windowSize.x) + "x" + std::to_string(windowSize.y) +
      ") " +
      (THeadless::IsHeadless ? " (HEADLESS) " : " (RENDER_ENABLED) ") +
      (THeadless::HideSceneTransitions ? " (NO SCENE TRANSITIONS) " : " (SCENE TRANSITIONS ENABLED) ") +
      (THeadless::TrackContentFiles ? " (HEADLESS CONTENT TRACKING) " : "");
  TLOG(LOG_INFO, "%s", infoStr.c_str());
}


inline Font RLPlays_LoadFontEx(const char* filename, int fontSize, int* codepoints, int codepointCount)
{
  if (THeadless::IsHeadless)
  {
    THeadless::AddContent(filename);
    return {10, 0, 0, {0, 1, 1, 0, 0}, nullptr, nullptr};
  }
  return ::LoadFontEx(filename, fontSize, codepoints, codepointCount);
}

inline Texture2D RLPlays_LoadTexture(const char* filename)
{
  if (THeadless::IsHeadless)
  {
    static unsigned int TextureId = 0;
    THeadless::AddContent(filename);
    return {++TextureId, 1, 1, 0, 0};
  }
  return ::LoadTexture(filename);
}

inline void RLPlays_UnloadFont(Font font) { if (!THeadless::IsHeadless) { ::UnloadFont(font); } }

inline void RLPlays_UnloadTexture(Texture2D texture) { if (!THeadless::IsHeadless) { ::UnloadTexture(texture); } }
}
