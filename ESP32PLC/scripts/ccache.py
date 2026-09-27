Import("env")
import shutil

ccache = shutil.which("ccache")
if ccache:
    # Prefix the compile COMMAND templates, not CC/CXX/AS: the espressif32 builder
    # (platforms/espressif32/builder/main.py) runs env.Replace(CC=..., CXX=..., AS=...)
    # after pre: scripts, so a prefix on those was silently dropped and ccache never
    # ran (its cache stayed empty). The platform leaves the *COM templates alone, and
    # library build environments are cloned from this one, so libraries are cached too.
    env.Replace(CCACHE='"%s"' % ccache)
    for com in ("CCCOM", "CXXCOM", "ASPPCOM", "SHCCCOM", "SHCXXCOM"):
        if com in env and "$CCACHE" not in str(env[com]):
            env[com] = "$CCACHE " + str(env[com])
    print("ccache: enabled")
else:
    print("ccache: not found - install with 'winget install Ccache.Ccache'")
