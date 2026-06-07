Import("env")
import shutil

ccache = shutil.which("ccache")
if ccache:
    for tool in ("CC", "CXX", "AS"):
        if tool in env:
            env[tool] = ccache + " " + env[tool]
    print("ccache: enabled")
else:
    print("ccache: not found — install with 'scoop install ccache' or 'choco install ccache'")
