#!/bin/bash
set -e

echo "==================================================="
echo "[BUILD] Building Schedule & WorkLog for macOS..."
echo "==================================================="

APP_NAME="ScheduleWorklog"
APP_BUNDLE="${APP_NAME}.app"
DIST_DIR="dist"
DMG_NAME="ScheduleWorklog-v1.0.0-mac.dmg"

mkdir -p "${DIST_DIR}"
rm -rf "${APP_BUNDLE}"

# 1. Compile macOS native Cocoa + WKWebView executable
echo "[1/4] Compiling macOS native binary..."
clang -O2 \
  src/common.c \
  src/db.c \
  src/item.c \
  src/project.c \
  src/server.c \
  src/sqlite3.c \
  src/summary.c \
  src/main_mac.m \
  -Iinclude \
  -framework Cocoa \
  -framework WebKit \
  -lpthread \
  -o "${APP_NAME}"

# 2. Package macOS .app bundle
echo "[2/4] Structuring ${APP_BUNDLE}..."
mkdir -p "${APP_BUNDLE}/Contents/MacOS"
mkdir -p "${APP_BUNDLE}/Contents/Resources"

mv "${APP_NAME}" "${APP_BUNDLE}/Contents/MacOS/"
cp -r web "${APP_BUNDLE}/Contents/Resources/"
if [ -f "app.ico" ]; then
  cp app.ico "${APP_BUNDLE}/Contents/Resources/"
fi

# Write Info.plist
cat << 'EOF' > "${APP_BUNDLE}/Contents/Info.plist"
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>CFBundleExecutable</key>
    <string>ScheduleWorklog</string>
    <key>CFBundleIdentifier</key>
    <string>com.bytesforce.scheduleworklog</string>
    <key>CFBundleName</key>
    <string>Schedule &amp; WorkLog</string>
    <key>CFBundleDisplayName</key>
    <string>Schedule &amp; WorkLog</string>
    <key>CFBundlePackageType</key>
    <string>APPL</string>
    <key>CFBundleShortVersionString</key>
    <string>1.0.0</string>
    <key>CFBundleVersion</key>
    <string>1.0.0</string>
    <key>LSMinimumSystemVersion</key>
    <string>11.0</string>
    <key>NSHighResolutionCapable</key>
    <true/>
</dict>
</plist>
EOF

# 3. Create DMG disk image with Applications shortcut
echo "[3/4] Creating DMG installer..."
DMG_TMP="dmg_temp"
rm -rf "${DMG_TMP}"
mkdir -p "${DMG_TMP}"
cp -r "${APP_BUNDLE}" "${DMG_TMP}/"
ln -s /Applications "${DMG_TMP}/Applications"

rm -f "${DIST_DIR}/${DMG_NAME}"
hdiutil create -volname "ScheduleWorklog" -srcfolder "${DMG_TMP}" -ov -format UDZO "${DIST_DIR}/${DMG_NAME}"
rm -rf "${DMG_TMP}"

echo "==================================================="
echo "[SUCCESS] macOS DMG created successfully!"
echo "Path: ${DIST_DIR}/${DMG_NAME}"
echo "==================================================="
