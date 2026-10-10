#!/usr/bin/env python3
"""asc_release.py — the whole App Store Connect side of a JellyDazzle release, one command per step.

Written after 3.5.9 (2026-10-10), which was submitted by hand through the API and hit every trap
this file now handles: a burned build number, a version created twice, export compliance missing
on the Mac build (409 on submit), and descriptions that still said things the app no longer does.

    asc_release.py status                                  every version + state, both platforms
    asc_release.py builds                                  recent builds, highest ever, next safe number
    asc_release.py prepare IOS 3.6.0 17 notes_iphone.txt   create/find version (MANUAL release),
    asc_release.py prepare MAC_OS 3.6.0 18 notes_mac.txt     set What's New, export compliance, attach build
    asc_release.py submit IOS 3.6.0                        submit that version for App Review
    asc_release.py ship 3.6.0 17 18 notes_iphone.txt notes_mac.txt
                                                           all of the above for both platforms, waiting
                                                           for each build to finish processing first

Credentials: the same App Store Connect API key asc-status uses (~/.secrets/asc/config.json).
Nothing here ever presses Release — releaseType is MANUAL on purpose; J presses Release.
"""
import base64, json, pathlib, subprocess, sys, time, urllib.request, urllib.error

APP_ID = "6808884433"                                   # JellyDazzle, team 46AXDWA6F8
CONF = json.load(open(pathlib.Path.home() / ".secrets/asc/config.json"))
API = "https://api.appstoreconnect.apple.com"

# ---- auth: ES256 JWT signed with openssl, exactly as ~/bin/asc-status does it -------------
def _b64u(b): return base64.urlsafe_b64encode(b).rstrip(b"=").decode()
def _der_to_raw(der):
    def rd(buf, i):
        assert buf[i] == 0x02; n = buf[i + 1]
        return buf[i + 2:i + 2 + n].lstrip(b"\x00").rjust(32, b"\x00"), i + 2 + n
    i = 2 if der[1] < 0x80 else 3
    r, i = rd(der, i); s, _ = rd(der, i)
    return r + s
def _token():
    now = int(time.time())
    head = _b64u(json.dumps({"alg": "ES256", "kid": CONF["key_id"], "typ": "JWT"}).encode())
    body = _b64u(json.dumps({"iss": CONF["issuer"], "iat": now, "exp": now + 1200,
                             "aud": "appstoreconnect-v1"}).encode())
    der = subprocess.run(["openssl", "dgst", "-sha256", "-sign", CONF["key_path"]],
                         input=f"{head}.{body}".encode(), capture_output=True, check=True).stdout
    return f"{head}.{body}.{_b64u(_der_to_raw(der))}"

def call(method, path, body=None):
    req = urllib.request.Request(API + path, method=method,
                                 data=json.dumps(body).encode() if body is not None else None,
                                 headers={"Authorization": "Bearer " + _token(),
                                          "Content-Type": "application/json"})
    try:
        raw = urllib.request.urlopen(req, timeout=60).read()
        return json.loads(raw) if raw else {}
    except urllib.error.HTTPError as e:
        # Apple hides the actual reason in associatedErrors — surface it, it is the whole point.
        try:
            for err in json.loads(e.read()).get("errors", []):
                print(f"  ASC {e.code}: {err.get('title')} — {err.get('detail')}", file=sys.stderr)
                for group in (err.get("meta", {}).get("associatedErrors") or {}).values():
                    for x in group: print(f"    - {x.get('code')}: {x.get('detail')}", file=sys.stderr)
        except Exception:
            pass
        raise SystemExit(f"{method} {path} failed ({e.code})")

# ---- helpers --------------------------------------------------------------------------------
def find_version(plat, ver):
    d = call("GET", f"/v1/apps/{APP_ID}/appStoreVersions?filter[platform]={plat}"
                    f"&filter[versionString]={ver}")["data"]
    return d[0]["id"] if d else None

def find_build(plat, num):
    d = call("GET", f"/v1/builds?filter[app]={APP_ID}&filter[version]={num}"
                    f"&filter[preReleaseVersion.platform]={plat}"
                    "&fields[builds]=version,processingState,usesNonExemptEncryption")["data"]
    return d[0] if d else None

# ---- commands -------------------------------------------------------------------------------
def status():
    d = call("GET", f"/v1/apps/{APP_ID}/appStoreVersions"
                    "?fields[appStoreVersions]=versionString,platform,appStoreState,releaseType&limit=20")
    for x in d["data"]:
        a = x["attributes"]
        print(f"{a['platform']:7} {a['versionString']:8} {a['appStoreState']:28} {a.get('releaseType') or ''}")

def builds():
    # Build numbers are burned FOREVER per app record, across both platforms, including
    # deliveries Apple rejected. Always pick above the highest ever seen.
    d = call("GET", f"/v1/builds?filter[app]={APP_ID}&fields[builds]=version,uploadedDate,processingState"
                    "&sort=-uploadedDate&limit=50")["data"]
    nums = sorted((int(x["attributes"]["version"]), x["attributes"]["uploadedDate"][:10],
                   x["attributes"]["processingState"]) for x in d if x["attributes"]["version"].isdigit())
    for n, day, st in nums[-8:][::-1]: print(f"build {n:<4} {day}  {st}")
    hi = nums[-1][0] if nums else 0
    print(f"highest ever: {hi}   next safe BUILD={hi + 1}")
    return hi

def wait_build(plat, num, limit_min=60):
    t0 = time.time()
    while True:
        b = find_build(plat, num)
        st = b["attributes"]["processingState"] if b else "NOT_YET_VISIBLE"
        print(f"{time.strftime('%H:%M:%S')} {plat} build {num}: {st}", flush=True)
        if st == "VALID": return b
        if st in ("FAILED", "INVALID"): raise SystemExit(f"build {num} failed Apple processing")
        if time.time() - t0 > limit_min * 60: raise SystemExit(f"build {num} still processing after {limit_min} min")
        time.sleep(60)

def prepare(plat, ver, num, notes_path):
    vid = find_version(plat, ver)
    if not vid:
        vid = call("POST", "/v1/appStoreVersions", {"data": {"type": "appStoreVersions",
              "attributes": {"platform": plat, "versionString": ver, "releaseType": "MANUAL"},
              "relationships": {"app": {"data": {"type": "apps", "id": APP_ID}}}}})["data"]["id"]
        print(f"{plat} {ver}: version created (manual release)")
    else:
        print(f"{plat} {ver}: version already exists — reusing")
    notes = open(notes_path, encoding="utf-8").read().strip()
    if len(notes) > 4000: raise SystemExit("What's New is over Apple's 4000-character limit")
    for loc in call("GET", f"/v1/appStoreVersions/{vid}/appStoreVersionLocalizations")["data"]:
        call("PATCH", f"/v1/appStoreVersionLocalizations/{loc['id']}", {"data": {
             "type": "appStoreVersionLocalizations", "id": loc["id"], "attributes": {"whatsNew": notes}}})
        print(f"  What's New set ({loc['attributes']['locale']}, {len(notes)} chars)")
    b = wait_build(plat, num)
    # 3.5.9: the Mac build arrived without this and submission failed with a 409. JellyDazzle
    # uses no encryption (it makes no network calls at all), so the honest answer is false.
    if b["attributes"].get("usesNonExemptEncryption") is None:
        call("PATCH", f"/v1/builds/{b['id']}", {"data": {"type": "builds", "id": b["id"],
             "attributes": {"usesNonExemptEncryption": False}}})
        print("  export compliance set: no non-exempt encryption")
    call("PATCH", f"/v1/appStoreVersions/{vid}/relationships/build",
         {"data": {"type": "builds", "id": b["id"]}})
    print(f"  build {num} attached")

def submit(plat, ver):
    vid = find_version(plat, ver) or sys.exit(f"no {plat} {ver} version — run prepare first")
    open_subs = call("GET", f"/v1/reviewSubmissions?filter[app]={APP_ID}&filter[platform]={plat}"
                            "&filter[state]=READY_FOR_REVIEW")["data"]
    sid = open_subs[0]["id"] if open_subs else call("POST", "/v1/reviewSubmissions", {"data": {
          "type": "reviewSubmissions", "attributes": {"platform": plat},
          "relationships": {"app": {"data": {"type": "apps", "id": APP_ID}}}}})["data"]["id"]
    call("POST", "/v1/reviewSubmissionItems", {"data": {"type": "reviewSubmissionItems", "relationships": {
         "reviewSubmission": {"data": {"type": "reviewSubmissions", "id": sid}},
         "appStoreVersion": {"data": {"type": "appStoreVersions", "id": vid}}}}})
    r = call("PATCH", f"/v1/reviewSubmissions/{sid}", {"data": {"type": "reviewSubmissions",
             "id": sid, "attributes": {"submitted": True}}})
    print(f"{plat} {ver}: SUBMITTED — {r['data']['attributes'].get('state')}")

def ship(ver, ios_build, mac_build, ios_notes, mac_notes):
    prepare("IOS", ver, ios_build, ios_notes)
    prepare("MAC_OS", ver, mac_build, mac_notes)
    submit("IOS", ver); submit("MAC_OS", ver)
    status()

if __name__ == "__main__":
    a = sys.argv[1:]
    cmds = {"status": (status, 0), "builds": (builds, 0), "prepare": (prepare, 4),
            "submit": (submit, 2), "ship": (ship, 5)}
    if not a or a[0] not in cmds or len(a) - 1 != cmds[a[0]][1]:
        print(__doc__); sys.exit(1)
    fn, n = cmds[a[0]]
    fn(*[int(x) if x.isdigit() else x for x in a[1:]])
