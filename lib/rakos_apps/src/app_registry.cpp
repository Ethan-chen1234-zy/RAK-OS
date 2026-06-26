#include <rakos/app_registry.h>
#include <rakos/pin_config.h>
#include <rakos/sd_fs.h>
#include <FS.h>

static const char *kDefaultCategories[] = {"General", "Games", "GPIO", "USB", "Hacking"};

namespace {

String baseName(String name) {
    const int slash = name.lastIndexOf('/');
    if (slash >= 0 && slash < static_cast<int>(name.length()) - 1) {
        return name.substring(slash + 1);
    }
    return name;
}

bool isJunkEntry(const String &name) {
    if (name.isEmpty() || name[0] == '.') {
        return true;
    }
    return name.equalsIgnoreCase("System Volume Information") || name.equalsIgnoreCase("$RECYCLE.BIN");
}

bool sdFileExists(const char *path) {
    if (rakos::sdFs().exists(path)) {
        return true;
    }
    File f = rakos::sdFs().open(path, FILE_READ);
    if (!f || f.isDirectory()) {
        return false;
    }
    f.close();
    return true;
}

bool sdDirOpen(const char *path) {
    File d = rakos::sdFs().open(path);
    if (!d) {
        return false;
    }
    const bool ok = d.isDirectory();
    d.close();
    return ok;
}

String entryPath(const String &parent, File &entry) {
    const char *full = entry.path();
    if (full && full[0] != '\0') {
        return String(full);
    }
    return parent + "/" + baseName(entry.name());
}

String displayPath(const String &fs_path) {
    if (fs_path.startsWith(SD_MOUNT_POINT)) {
        return fs_path;
    }
    if (fs_path.startsWith("/")) {
        return String(SD_MOUNT_POINT) + fs_path;
    }
    return String(SD_MOUNT_POINT) + "/" + fs_path;
}

String joinFsPath(const String &root, const String &segment) {
    if (root == "/" || root.isEmpty()) {
        return "/" + segment;
    }
    if (root.endsWith("/")) {
        return root + segment;
    }
    return root + "/" + segment;
}

}  // namespace

bool AppRegistry::begin(bool sd_ready) {
    sd_ready_ = sd_ready;
    refresh();
    return true;
}

void AppRegistry::addApp(const String &category, const String &folder_path, const String &bin_path) {
    for (const AppEntry &existing : apps_) {
        if (existing.bin_path == bin_path) {
            return;
        }
    }

    AppEntry app;
    app.category = category;
    app.name = baseName(folder_path);
    app.folder_path = folder_path;
    app.bin_path = bin_path;
    app.has_bin = sdFileExists(bin_path.c_str());
    apps_.push_back(app);
}

void AppRegistry::scanAppFolder(const String &category, const String &folder_path) {
    if (isJunkEntry(baseName(folder_path))) {
        return;
    }

    const String bin_path = folder_path + "/app.bin";
    if (sdFileExists(bin_path.c_str())) {
        addApp(category, folder_path, bin_path);
        return;
    }

    File dir = rakos::sdFs().open(folder_path.c_str());
    if (!dir || !dir.isDirectory()) {
        return;
    }

    File entry = dir.openNextFile();
    while (entry) {
        const String child_path = entryPath(folder_path, entry);
        const String child_bin = child_path + "/app.bin";
        if (sdFileExists(child_bin.c_str())) {
            addApp(category, child_path, child_bin);
        }
        entry.close();
        entry = dir.openNextFile();
    }
    dir.close();
}

void AppRegistry::scanMountRoot(const String &fs_root) {
    last_scan_info_ = "FS root: " + displayPath(fs_root);

    File root = rakos::sdFs().open(fs_root.c_str());
    if (!root || !root.isDirectory()) {
        last_scan_info_ += " (open failed)";
        Serial.printf("[APP] Cannot open SD FS root: %s\n", fs_root.c_str());
        return;
    }

    uint16_t root_entries = 0;
    File entry = root.openNextFile();
    while (entry) {
        ++root_entries;
        entry.close();
        entry = root.openNextFile();
    }
    root.close();

    last_scan_info_ += ", entries=" + String(root_entries);

    for (const char *cat : kDefaultCategories) {
        const String cat_open = joinFsPath(fs_root, cat);
        if (!sdDirOpen(cat_open.c_str())) {
            continue;
        }

        bool known = false;
        for (const String &c : categories_) {
            if (c.equalsIgnoreCase(cat)) {
                known = true;
                break;
            }
        }
        if (!known) {
            categories_.push_back(String(cat));
        }

        scanAppFolder(String(cat), cat_open);
    }

    root = rakos::sdFs().open(fs_root.c_str());
    if (!root || !root.isDirectory()) {
        return;
    }

    entry = root.openNextFile();
    while (entry) {
        if (entry.isDirectory()) {
            const String name = baseName(entryPath(fs_root, entry));
            if (!isJunkEntry(name)) {
                bool known = false;
                for (const String &c : categories_) {
                    if (c.equalsIgnoreCase(name)) {
                        known = true;
                        break;
                    }
                }
                if (!known) {
                    categories_.push_back(name);
                    const String folder = entryPath(fs_root, entry);
                    scanAppFolder(name, folder);
                }
            }
        }
        entry.close();
        entry = root.openNextFile();
    }
    root.close();
}

void AppRegistry::refresh() {
    apps_.clear();
    categories_.clear();
    last_scan_info_.clear();

    if (!sd_ready_) {
        last_scan_info_ = "SD not ready";
        Serial.println("[APP] SD not ready, skip scan");
        return;
    }

    scanMountRoot(SD_FS_ROOT);

    if (apps_.empty() && String(SD_FS_ROOT) != "/") {
        Serial.println("[APP] No apps at SD_FS_ROOT — retrying /");
        scanMountRoot("/");
    }

    last_scan_info_ += ", apps=" + String(apps_.size());
    Serial.printf("[APP] Scan done: %s\n", last_scan_info_.c_str());
}

bool AppRegistry::findByName(const String &name, AppEntry &out) const {
    for (const AppEntry &app : apps_) {
        if (app.name == name) {
            out = app;
            return true;
        }
    }
    return false;
}
