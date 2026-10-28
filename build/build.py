#!/usr/bin/env python3
"""
Build the student bundle into dist/ (run by GitHub Actions on every push; you
can also run it by hand:  python3 build/build.py).

dist/
  START HERE.html      one-file installer: the loader + the whole bundle inside
  version.json         {version, date, notes}      <- the dashboard checks this
  manifest.json        {version, date, files:[{path,size,type}]}
  config.json          teacher settings (copied from the repo root)
  Studica-START-HERE.zip   same installer, zipped so browsers download it
  <every bundle file>  served raw so the Update button can fetch them
"""
import base64, datetime, json, os, re, shutil, subprocess, sys, zipfile

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
DIST = os.path.join(ROOT, "dist")
RAW_BASE = os.environ.get("RAW_BASE", "https://raw.githubusercontent.com/Heshamoov/Studica/dist/")

# Files that travel to the students (and to the robot).
BUNDLE = ["BEGINNER-GUIDE.html", "tools/settings.html", "tools/controlpanel.html", "docs/Wiring-Sheet-1-Buttons.pdf"]
ROBOT = ["robot-onboard/Makefile", "robot-onboard/run.sh"]
BINARY = (".pdf", ".png", ".jpg")


def sh(*cmd):
    try:
        return subprocess.check_output(cmd, cwd=ROOT, stderr=subprocess.DEVNULL).decode().strip()
    except Exception:
        return ""


def main():
    now = datetime.datetime.now(datetime.timezone.utc)
    sha = sh("git", "rev-parse", "--short", "HEAD") or "local"
    stamped = open(os.path.join(ROOT, "robot-onboard", "VERSION")).read().strip() if os.path.exists(os.path.join(ROOT, "robot-onboard", "VERSION")) else ""
    version = os.environ.get("VERSION") or (stamped if stamped and stamped != "dev" else now.strftime("%Y-%m-%d.%H%M") + "-" + sha)
    date = now.strftime("%Y-%m-%dT%H:%M:%SZ")
    notes = os.environ.get("NOTES") or sh("git", "log", "-1", "--pretty=%s") or ""

    files = list(BUNDLE) + list(ROBOT)
    for d, pat in [("robot-onboard/src", r".*\.cpp$"), ("robot-onboard/include", r".*\.h$"), ("robot-onboard/manager", r".*")]:
        for n in sorted(os.listdir(os.path.join(ROOT, d))):
            if re.match(pat, n) and os.path.isfile(os.path.join(ROOT, d, n)) and not n.startswith("."):
                files.append(d + "/" + n)

    if os.path.isdir(DIST):
        shutil.rmtree(DIST)
    os.makedirs(DIST)

    bundle, manifest = {}, []
    for rel in files:
        src = os.path.join(ROOT, rel)
        with open(src, "rb") as f:
            data = f.read()
        is_bin = rel.lower().endswith(BINARY)
        bundle[rel] = {"t": "b64", "d": base64.b64encode(data).decode()} if is_bin else {"t": "text", "d": data.decode("utf-8")}
        manifest.append({"path": rel, "size": len(data), "type": "b64" if is_bin else "text"})
        dst = os.path.join(DIST, rel)
        os.makedirs(os.path.dirname(dst), exist_ok=True)
        shutil.copyfile(src, dst)
    # the robot's VERSION file is generated, so the robot can report what it runs
    bundle["robot-onboard/VERSION"] = {"t": "text", "d": version + "\n"}
    manifest.append({"path": "robot-onboard/VERSION", "size": len(version) + 1, "type": "text"})
    with open(os.path.join(DIST, "robot-onboard", "VERSION"), "w") as f:
        f.write(version + "\n")

    with open(os.path.join(DIST, "manifest.json"), "w") as f:
        json.dump({"version": version, "date": date, "files": manifest}, f)
    with open(os.path.join(DIST, "version.json"), "w") as f:
        json.dump({"version": version, "date": date, "notes": notes}, f)
    cfg = os.path.join(ROOT, "config.json")
    shutil.copyfile(cfg if os.path.exists(cfg) else os.devnull, os.path.join(DIST, "config.json"))
    if not os.path.exists(cfg):
        with open(os.path.join(DIST, "config.json"), "w") as f:
            f.write("{}")

    with open(os.path.join(ROOT, "build", "loader.html"), encoding="utf-8") as f:
        loader = f.read()
    payload = json.dumps({"version": version, "date": date, "notes": notes, "files": bundle}).replace("</", "<\\/")
    loader = loader.replace("/*BUILTIN_JSON*/", payload).replace("/*RAW_BASE*/", RAW_BASE)
    out = os.path.join(DIST, "START HERE.html")
    with open(out, "w", encoding="utf-8") as f:
        f.write(loader)
    # light START HERE.html for the repo root (GitHub "Download ZIP"): no built-in copy,
    # it downloads the newest version the first time it opens
    light = open(os.path.join(ROOT, "build", "loader.html"), encoding="utf-8").read()
    light = light.replace("/*BUILTIN_JSON*/", '{"version": "", "date": "", "files": {}}').replace("/*RAW_BASE*/", RAW_BASE)
    with open(os.path.join(DIST, "START-HERE-light.html"), "w", encoding="utf-8") as f:
        f.write(light)
    with zipfile.ZipFile(os.path.join(DIST, "Studica-START-HERE.zip"), "w", zipfile.ZIP_DEFLATED) as z:
        z.write(out, "START HERE.html")
    print("built", version, "-", len(files) + 1, "files,", os.path.getsize(out) // 1024, "KB installer")


if __name__ == "__main__":
    main()
