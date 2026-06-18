#pragma once

#include <string>
#include <map>
#include <fstream>
#include <sstream>
#include <vector>
#include <stdexcept>

//! @brief TConfig is a simple INI file parser and writer. 
class TConfig
{
  /**
  (CoPilot+Claude written)
  Prompt used: Create a class TConfig that can read INI config file with a constructor TConfig(std::string filename)
  And: Add SaveFile
*/
public:
  // Constructor takes the path to an INI file
  TConfig(const std::string& filename)
  {
    LoadFromFile(filename);
    filename_ = filename;
  }

  // Default constructor creates an empty configuration
  TConfig() = default;

  // Get a string value from the configuration
  [[nodiscard]] std::string GetString(const std::string& section, const std::string& key,
                                      const std::string& defaultValue = "") const
  {
    const auto sectionIt = data.find(section);
    if (sectionIt != data.end())
    {
      const auto keyIt = sectionIt->second.find(key);
      if (keyIt != sectionIt->second.end()) { return keyIt->second; }
    }
    return defaultValue;
  }

  // Get an integer value from the configuration
  [[nodiscard]] int GetInt(const std::string& section, const std::string& key, const int defaultValue = 0) const
  {
    const auto value = GetString(section, key, "");
    if (value.empty()) { return defaultValue; }
    try { return std::stoi(value); }
    catch (const std::exception&) { return defaultValue; }
  }

  // Get a double value from the configuration
  [[nodiscard]] double GetDouble(const std::string& section, const std::string& key,
                                 const double defaultValue = 0.0) const
  {
    const auto value = GetString(section, key, "");
    if (value.empty()) { return defaultValue; }
    try { return std::stod(value); }
    catch (const std::exception&) { return defaultValue; }
  }

  // Get a boolean value from the configuration
  [[nodiscard]] bool GetBool(const std::string& section, const std::string& key, const bool defaultValue = false) const
  {
    const auto value = GetString(section, key, "");
    if (value.empty()) { return defaultValue; }

    // Convert to lowercase for case-insensitive comparison
    std::string lowerValue = value;
    for (auto& c : lowerValue) { c = std::tolower(c); }

    if (lowerValue == "true" || lowerValue == "yes" || lowerValue == "1") { return true; }
    else if (lowerValue == "false" || lowerValue == "no" || lowerValue == "0") { return false; }
    return defaultValue;
  }

  // Set a string value in the configuration
  TConfig& SetString(const std::string& section, const std::string& key, const std::string& value)
  {
    data[section][key] = value;
    return *this;
  }

  // Set an integer value in the configuration
  TConfig& SetInt(const std::string& section, const std::string& key, const int value)
  {
    data[section][key] = std::to_string(value);
    return *this;
  }

  // Set a double value in the configuration
  TConfig& SetDouble(const std::string& section, const std::string& key, const double value)
  {
    data[section][key] = std::to_string(value);
    return *this;
  }

  // Set a boolean value in the configuration
  TConfig& SetBool(const std::string& section, const std::string& key, const bool value)
  {
    data[section][key] = value ? "true" : "false";
    return *this;
  }

  // Save configuration to a file
  void SaveFile(const std::string& filename = "")
  {
    std::string outputFilename = filename.empty() ? filename_ : filename;
    if (outputFilename.empty()) { throw std::runtime_error("No filename specified for saving configuration"); }

    std::ofstream file(outputFilename);
    if (!file.is_open()) { throw std::runtime_error("Could not open file for writing: " + outputFilename); }

    // If we're saving to a new file, update the stored filename
    if (!filename.empty()) { filename_ = filename; }

    // Write each section
    for (const auto& [section, values] : data)
    {
      if (values.empty()) { continue; }
      file << "[" << section << "]\n";
      for (const auto& [k, v] : values)
      {
        if (k.find('#') == 0) { file << v << '\n'; } // Comment line
        else { file << k << " = " << v << '\n'; }
      }
      file << '\n';
    }

    file.close();
  }


  // Check if a section exists
  [[nodiscard]] bool HasSection(const std::string& section) const { return data.find(section) != data.end(); }

  // Check if a key exists in a section
  [[nodiscard]] bool HasKey(const std::string& section, const std::string& key) const
  {
    const auto sectionIt = data.find(section);
    if (sectionIt != data.end()) { return sectionIt->second.find(key) != sectionIt->second.end(); }
    return false;
  }

  // Get all section names
  [[nodiscard]] std::vector<std::string> GetSections() const
  {
    std::vector<std::string> sections;
    sections.reserve(data.size());
    for (const auto& section : data) { sections.push_back(section.first); }
    return sections;
  }

  // Get all keys in a section
  [[nodiscard]] std::vector<std::string> GetKeys(const std::string& section) const
  {
    std::vector<std::string> keys;
    auto sectionIt = data.find(section);
    if (sectionIt != data.end())
    {
      keys.reserve(sectionIt->second.size());
      for (const auto& key : sectionIt->second)
      {
        keys.push_back(key.first);
      }
    }
    return keys;
  }

private:
  std::string filename_;
  // Map of section -> (map of key -> value)
  std::map<std::string, std::map<std::string, std::string>> data;

  // Load and parse the INI file
  void LoadFromFile(const std::string& filename)
  {
    std::ifstream file(filename);
    if (!file.is_open()) { throw std::runtime_error("Could not open config file: " + filename); }

    std::string line;
    std::string currentSection;
    int lineIndex = 0;
    while (std::getline(file, line))
    {
      line = Trim(line);
      ++lineIndex;
      if (line.empty() || line[0] == ';' || line[0] == '#')
      {
        if (!currentSection.empty() && !line.empty()) { data[currentSection]["#_" + std::to_string(lineIndex)] = line; }
        continue;
      }
      if (line[0] == '[' && line[line.size() - 1] == ']')
      {
        currentSection = line.substr(1, line.size() - 2);
        continue;
      }

      const size_t equalsPos = line.find('=');
      if (equalsPos != std::string::npos)
      {
        std::string key = Trim(line.substr(0, equalsPos));
        std::string value = Trim(line.substr(equalsPos + 1));
        data[currentSection][key] = value;
      }
    }
  }

  // Utility function to trim whitespace from a string
  static std::string Trim(const std::string& str)
  {
    const std::string whitespace = " \t\n\r\f\v";
    const auto start = str.find_first_not_of(whitespace);
    if (start == std::string::npos) { return ""; }
    const auto end = str.find_last_not_of(whitespace);
    return str.substr(start, end - start + 1);
  }
};
