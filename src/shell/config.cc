#include "config.h"
#include <chrono>
#include <filesystem>
#include <fstream>
#include <thread>

#include "rfl.hpp"
#include "rfl/DefaultIfMissing.hpp"
#include "rfl/json.hpp"

#include "windows.h"

namespace mb_shell {
std::unique_ptr<config> config::current;
void config::write_config() {
  auto config_file = data_directory() / "config.json";
  std::ofstream ofs(config_file);
  if (!ofs) {
    std::cerr << "Failed to write config file." << std::endl;
    return;
  }

  ofs << rfl::json::write(*config::current);
}
void config::read_config() {
  auto config_file = data_directory() / "config.json";

  std::ifstream ifs(config_file);

  if (!std::filesystem::exists(config_file)) {
    auto config_file = data_directory() / "config.json";
    std::ofstream ofs(config_file);
    if (!ofs) {
      std::cerr << "Failed to write config file." << std::endl;
      return;
    }

    ofs << "{}";
  }

  if (!ifs) {
    std::cerr
        << "Config file could not be opened. Using default config instead."
        << std::endl;
    config::current = std::make_unique<config>();
    config::current->debug_console = true;
  } else {
    if (auto json =
            rfl::json::read<config, rfl::NoExtraFields, rfl::DefaultIfMissing>(
                ifs)) {
      config::current = std::make_unique<config>(json.value());
    } else {
      std::cerr << "Failed to read config file: " << json.error()->what()
                << "\nUsing default config instead." << std::endl;
      config::current = std::make_unique<config>();
      config::current->debug_console = true;
    }
  }

  if (config::current->debug_console) {
    ShowWindow(GetConsoleWindow(), SW_SHOW);
  } else {
    ShowWindow(GetConsoleWindow(), SW_HIDE);
  }
}

std::filesystem::path config::data_directory() {
  static std::optional<std::filesystem::path> path;
  if (!path) {
    wchar_t home_dir[MAX_PATH];
    GetEnvironmentVariableW(L"HOMEPATH", home_dir, MAX_PATH);
    path = home_dir;
    *path /= ".breeze-shell";
  }

  if (!std::filesystem::exists(*path)) {
    std::filesystem::create_directories(*path);
  }

  return path.value();
}
void config::run_config_loader() {
  std::thread([]() {
    config::read_config();
    auto config_path = config::data_directory() / "config.json";
    auto last_mod = std::filesystem::last_write_time(config_path);
    while (true) {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      if (std::filesystem::last_write_time(config_path) != last_mod) {
        last_mod = std::filesystem::last_write_time(config_path);
        config::read_config();
      }
    }
  }).detach();
}
} // namespace mb_shell