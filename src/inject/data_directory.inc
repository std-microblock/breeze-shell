std::filesystem::path data_directory() {
  static std::optional<std::filesystem::path> path;
  static std::mutex mtx;
  std::lock_guard lock(mtx);

  if (!path) {
    wchar_t home_dir[MAX_PATH];
    GetEnvironmentVariableW(L"USERPROFILE", home_dir, MAX_PATH);
    path = std::filesystem::path(home_dir) / ".breeze-shell";
  }

  if (!std::filesystem::exists(*path)) {
    std::filesystem::create_directories(*path);
  }

  return path.value();
}