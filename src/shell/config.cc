#include "config.h"
#include <chrono>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <thread>

#include "logger.h"
#include "rfl.hpp"
#include "rfl/DefaultIfMissing.hpp"
#include "rfl/json.hpp"

#include "utils.h"
#include "windows.h"

namespace rfl {
template <>
struct Reflector<mb_shell::config::paint_color> {
  using ReflType = std::string;
  
  static mb_shell::config::paint_color to(const ReflType& v) noexcept {
    return mb_shell::config::paint_color::from_string(v);
  }

  static ReflType from(const mb_shell::config::paint_color& v) {
    return v.to_string();
  }
};
}

namespace mb_shell {
std::unique_ptr<config> config::current;
config::animated_float_conf config::_default_animation{
    .duration = 150,
    .easing = ui::easing_type::ease_in_out,
    .delay_scale = 1,
};

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

#ifdef __llvm__
  std::ifstream ifs(config_file);
  if (!std::filesystem::exists(config_file)) {
    auto config_file = data_directory() / "config.json";
    std::ofstream ofs(config_file);
    if (!ofs) {
      std::cerr << "Failed to write config file." << std::endl;
    }

    ofs << R"({
  "$schema": "https://raw.githubusercontent.com/std-microblock/breeze-shell/refs/heads/master/resources/schema.json"
})";
  }
  if (!ifs) {
    std::cerr
        << "Config file could not be opened. Using default config instead."
        << std::endl;
    config::current = std::make_unique<config>();
    config::current->debug_console = true;
  } else {
    std::string json_str;
    std::copy(std::istreambuf_iterator<char>(ifs),
              std::istreambuf_iterator<char>(), std::back_inserter(json_str));

    if (auto json =
            rfl::json::read<config, rfl::NoExtraFields, rfl::DefaultIfMissing>(
                json_str)) {
      // parse twice for default value
      _default_animation = json.value().default_animation;
      json = rfl::json::read<config, rfl::NoExtraFields, rfl::DefaultIfMissing>(
          json_str);
      config::current = std::make_unique<config>(json.value());
      std::cout << "Config reloaded." << std::endl;
    } else {
      std::cerr << "Failed to read config file: " << json.error()->what()
                << "\nUsing default config instead." << std::endl;
      config::current = std::make_unique<config>();
      config::current->debug_console = true;
    }
  }
#else
#pragma message                                                                \
    "We don't support loading config file on MSVC because of a bug in MSVC."
  dbgout("We don't support loading config file when compiled with MSVC "
               "because of a bug in MSVC.");
  config::current = std::make_unique<config>();
  config::current->debug_console = true;
#endif

  if (config::current->debug_console) {
    ShowWindow(GetConsoleWindow(), SW_SHOW);
  } else {
    ShowWindow(GetConsoleWindow(), SW_HIDE);
  }
}

std::filesystem::path config::data_directory() {
  static std::optional<std::filesystem::path> path;
  static std::mutex mtx;
  std::lock_guard lock(mtx);

  if (!path) {
    path = std::filesystem::path(env("USERPROFILE").value()) / ".breeze-shell";
  }

  if (!std::filesystem::exists(*path)) {
    std::filesystem::create_directories(*path);
  }

  return path.value();
}
void config::run_config_loader() {
  auto config_path = config::data_directory() / "config.json";
  dbgout("config file: {}", config_path.string());
  config::read_config();
  std::thread([config_path]() {
    auto last_mod = std::filesystem::last_write_time(config_path);
    while (true) {
      if (std::filesystem::last_write_time(config_path) != last_mod) {
        last_mod = std::filesystem::last_write_time(config_path);
        config::read_config();
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
  }).detach();
}
void config::animated_float_conf::apply_to(ui::sp_anim_float &anim,
                                           float delay) {
  anim->set_duration(duration);
  anim->set_easing(easing);
  anim->set_delay(delay * delay_scale);
}
void config::animated_float_conf::operator()(ui::sp_anim_float &anim,
                                             float delay) {
  apply_to(anim, delay);
}

std::filesystem::path config::default_main_font() {
  return std::filesystem::path(env("WINDIR").value()) / "Fonts" / "segoeui.ttf";
}
std::filesystem::path config::default_fallback_font() {
  return std::filesystem::path(env("WINDIR").value()) / "Fonts" / "msyh.ttc";
}
std::string config::dump_config() { return rfl::json::write(*config::current); }
config::paint_color config::paint_color::from_string(const std::string &str) {
  paint_color res;
  // Trim whitespace
  std::string trimmed = str;
  trimmed.erase(0, trimmed.find_first_not_of(" \t\n\r"));
  trimmed.erase(trimmed.find_last_not_of(" \t\n\r") + 1);

  if (trimmed.starts_with("solid(") && trimmed.ends_with(")")) {
    std::string color_str = trimmed.substr(6, trimmed.length() - 7);
    res.type = type::solid;
    res.color = parse_color(color_str);
  } else if (trimmed.starts_with("linear-gradient(") &&
             trimmed.ends_with(")")) {
    std::string params = trimmed.substr(16, trimmed.length() - 17);

    // Split by commas
    std::vector<std::string> parts;
    size_t start = 0, end = 0;
    while ((end = params.find(',', start)) != std::string::npos) {
      std::string part = params.substr(start, end - start);
      part.erase(0, part.find_first_not_of(" \t"));
      part.erase(part.find_last_not_of(" \t") + 1);
      parts.push_back(part);
      start = end + 1;
    }
    std::string last_part = params.substr(start);
    last_part.erase(0, last_part.find_first_not_of(" \t"));
    last_part.erase(last_part.find_last_not_of(" \t") + 1);
    parts.push_back(last_part);

    if (parts.size() >= 3) {
      res.type = type::linear_gradient;
      res.angle = std::stof(parts[0]) * std::numbers::pi /
                  180.0f; // Convert degrees to radians
      res.color = parse_color(parts[1]);
      res.color2 = parse_color(parts[2]);
    }
  } else if (trimmed.starts_with("radial-gradient(") &&
             trimmed.ends_with(")")) {
    std::string params = trimmed.substr(16, trimmed.length() - 17);

    // Split by commas
    std::vector<std::string> parts;
    size_t start = 0, end = 0;
    while ((end = params.find(',', start)) != std::string::npos) {
      std::string part = params.substr(start, end - start);
      part.erase(0, part.find_first_not_of(" \t"));
      part.erase(part.find_last_not_of(" \t") + 1);
      parts.push_back(part);
      start = end + 1;
    }
    std::string last_part = params.substr(start);
    last_part.erase(0, last_part.find_first_not_of(" \t"));
    last_part.erase(last_part.find_last_not_of(" \t") + 1);
    parts.push_back(last_part);

    if (parts.size() >= 3) {
      res.type = type::radial_gradient;
      res.radius = std::stof(parts[0]);
      res.color = parse_color(parts[1]);
      res.color2 = parse_color(parts[2]);
      res.radius2 = res.radius * 2; // Default outer radius
    }
  } else {
    // Default to solid color
    res.type = type::solid;
    res.color = parse_color(trimmed);
  }

  return res;
}
std::string config::paint_color::to_string() const {
  switch (type) {
  case type::solid:
    return "solid(" + format_color(color) + ")";
  case type::linear_gradient:
    return "linear-gradient(" +
           std::to_string(angle * 180.0f / std::numbers::pi) + ", " +
           format_color(color) + ", " + format_color(color2) + ")";
  case type::radial_gradient:
    return "radial-gradient(" + std::to_string(radius) + ", " +
           format_color(color) + ", " + format_color(color2) + ")";
  }
  return "";
}
} // namespace mb_shell