#!/usr/bin/env python3
"""Stamp one version (Dubai time) into the page and the robot program, so the
teacher can see what each team runs. Runs as a git pre-commit hook."""
import datetime, os, re
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
v = (datetime.datetime.now(datetime.timezone.utc) + datetime.timedelta(hours=4)).strftime("%Y-%m-%d %H:%M")
open(os.path.join(ROOT, "robot-onboard", "VERSION"), "w").write(v + "\n")
p = os.path.join(ROOT, "BEGINNER-GUIDE.html")
s = open(p, encoding="utf-8").read()
s = re.sub(r'const PAGE_VER = "[^"]*";', 'const PAGE_VER = "%s";' % v, s, count=1)
open(p, "w", encoding="utf-8").write(s)
print(v)
