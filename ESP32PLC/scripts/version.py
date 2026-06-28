Import("env")
import subprocess

def _git(*args):
    try:
        r = subprocess.run(
            ["git"] + list(args),
            capture_output=True, text=True,
            cwd=env.subst("$PROJECT_DIR")
        )
        return r.stdout.strip()
    except Exception:
        return ""

fw_ver  = _git("describe", "--tags", "--always", "--dirty") or "dev"
fw_sha  = _git("rev-parse", "--short", "HEAD")              or "0000000"
web_ver = _git("log", "-1", "--format=%h", "--", "data/")  or "0000000"

env.Append(CPPDEFINES=[
    ("FW_VERSION",  env.StringifyMacro(fw_ver)),
    ("FW_SHA",      env.StringifyMacro(fw_sha)),
    ("WEB_VERSION", env.StringifyMacro(web_ver)),
])

print(f"-- FW version:  {fw_ver}  (sha: {fw_sha})")
print(f"-- Web version: {web_ver}")
