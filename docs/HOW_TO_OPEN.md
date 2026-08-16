# Opening JellyDazzle for the first time

macOS will say **"Apple could not verify JellyDazzle is free of malware."**
That is Gatekeeper flagging any app without a paid Apple notarization
certificate — not a problem with the app. Every line of source is in this repo.

You do this **once**, and it takes about 30 seconds:

1. **Double-click** the app once and let it get refused
2. Open **System Settings → Privacy & Security**
3. Scroll to the bottom — there's a line saying
   *"JellyDazzle was blocked to protect your Mac"*
4. Click **Open Anyway** → confirm with Touch ID or your password
5. It launches, and every launch after that is a normal double-click

## Or the one-liner, which works regardless

Paste this in Terminal after downloading:

```
xattr -dr com.apple.quarantine ~/Downloads/JellyDazzle.app
```

Then double-click normally.

> On macOS 15 and newer, the old "right-click → Open" trick no longer works for
> unsigned apps. Use **Open Anyway** or the command above.

## Good to know

- **Apple Silicon Macs only** (M1 or newer)
- Press **ESC** to quit
- Nothing to install — SDL is bundled inside the app
- Building it yourself (`make`) produces an app with no warning at all

*A signed, notarized build is coming as soon as the Apple Developer certificate
is in place — then downloads open with no warning and none of this is needed.*
