# Opening JellyDazzle for the first time

macOS will say **"Apple could not verify JellyDazzle is free of malware."**
That is normal for free open-source apps that haven't been through Apple's paid
notarization service. Here's how to open it — you only do this once.

## The quick way

- Download the zip and double-click it to unzip
- **Right-click** (or Control-click) the **JellyDazzle** app → choose **Open**
- The warning appears with an **Open** button this time → click **Open**
- Done. From now on, double-click it like any other app

## If there's no "Open" option (newer macOS)

- Double-click the app once and let it get blocked
- Go to **System Settings → Privacy & Security**
- Scroll down to the note about JellyDazzle → click **Open Anyway**
- Confirm with your password or Touch ID

## Terminal one-liner (if you prefer)

```
xattr -dr com.apple.quarantine /path/to/JellyDazzle.app
```

## Good to know

- **Apple Silicon Macs only** (M1 or newer)
- Press **ESC** to quit
- Nothing to install — SDL is bundled inside the app
- Every line of source is public: https://github.com/LIBCSYS/JellyDazzle
- Building it yourself (`make`) produces an app with no warning at all

*A signed, notarized build is coming once the Apple Developer certificate is in
place — then downloads will open with no warning and none of this will be needed.*
