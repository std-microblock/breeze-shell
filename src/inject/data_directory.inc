std::filesystem::path data_directory() {
  static std::optional<std::filesystem::path> path;
  static std::mutex mtx;
  std::lock_guard lock(mtx);

  if (!path) {
    wchar_t custom_dir[32767] = {0};
    DWORD custom_len =
        GetEnvironmentVariableW(L"BREEZE_DATA_DIR", custom_dir, 32767);
    if (custom_len > 0 && custom_len < 32767) {
      path = std::filesystem::path(custom_dir);
    } else {
      wchar_t home_dir[MAX_PATH];
      GetEnvironmentVariableW(L"USERPROFILE", home_dir, MAX_PATH);
      path = std::filesystem::path(home_dir) / ".breeze-shell";
    }

    std::error_code ec;
    std::filesystem::create_directories(*path, ec);
  }

  return path.value();
}