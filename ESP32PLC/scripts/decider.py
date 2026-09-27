Import("env")

# Faster up-to-date checks (a no-change build went from ~15.8 s to ~7.5 s).
#
# implicit_cache: SCons re-reads the #include lines of every source and header
# on every build to rebuild the dependency tree. This keeps that tree between
# builds and only re-reads files that changed. A header edit still rebuilds
# every file that includes it (checked). Only corner case: a NEW header that
# shadows an existing one of the same name on the include path — run
# `pio run -t clean` if you ever do that.
env.SetOption("implicit_cache", 1)

# MD5-timestamp: only re-hash files whose timestamp moved; a real edit is still
# caught by its content hash.
env.Decider("MD5-timestamp")
