Import("env")
import os

# Adds the apps listed in `custom_modules` (platformio.ini) to lib_deps.
# Each app is a folder in lib/ that registers itself with REGISTER_MODULE().
# Apps not listed are not compiled at all.
#
#   custom_modules =
#       LEDStripModule
#       FireworksModule

mods = env.GetProjectOption("custom_modules", "").split()
lib_dir = env.subst("$PROJECT_DIR/lib")

missing = [m for m in mods if not os.path.isdir(os.path.join(lib_dir, m))]
if missing:
    avail = sorted(d for d in os.listdir(lib_dir) if d.endswith("Module"))
    print("modules: not found in lib/: %s  (available: %s)" % (", ".join(missing), ", ".join(avail)))
    env.Exit(1)

if mods:
    cfg  = env.GetProjectConfig()
    sect = "env:" + env["PIOENV"]
    deps = cfg.get(sect, "lib_deps", [])
    cfg.set(sect, "lib_deps", deps + [m for m in mods if m not in deps])

print("-- Modules: %s" % (", ".join(mods) if mods else "none"))
