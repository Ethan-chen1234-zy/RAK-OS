#pragma once

#include <Arduino.h>
#include <vector>

struct AppEntry {
    String category;
    String name;
    String folder_path;
    String bin_path;
    bool has_bin = false;
};

class AppRegistry {
public:
    bool begin(bool sd_ready);
    const std::vector<AppEntry> &apps() const { return apps_; }
    const std::vector<String> &categories() const { return categories_; }
    const String &lastScanInfo() const { return last_scan_info_; }
    void refresh();
    bool findByName(const String &name, AppEntry &out) const;

private:
    std::vector<AppEntry> apps_;
    std::vector<String> categories_;
    String last_scan_info_;
    bool sd_ready_ = false;

    void scanMountRoot(const String &sd_root);
    void scanAppFolder(const String &category, const String &folder_path);
    void addApp(const String &category, const String &folder_path, const String &bin_path);
};
