#!/usr/bin/env python
import os
import sys
from SCons.Script import *

env = SConscript("godot-cpp/SConstruct")

env.Append(CPPPATH=[
    "godot-cpp/include",
    "godot-cpp/include/gen",
    "src/",
])

env.Append(LIBPATH=["lib/"])
env.Append(LIBS=["vosk", "portaudio64bit"])  # Adapte si ton .lib/.a a un nom spécial

sources = Glob("src/*.cpp")

if env["platform"] == "windows":
    library = env.SharedLibrary(
        "demo/bin/godosk.{}{}.dll".format(env["platform"], env["target"]),
        source=sources,
    )
elif env["platform"] == "macos":
    library = env.SharedLibrary(
        "demo/bin/libgodosk.{}.{}.framework/libgodosk.{}.{}".format(
            env["platform"], env["target"], env["platform"], env["target"]
        ),
        source=sources,
    )
elif env["platform"] == "ios":
    if env["ios_simulator"]:
        library = env.StaticLibrary(
            "demo/bin/libgodosk.{}.{}.simulator.a".format(env["platform"], env["target"]),
            source=sources,
        )
    else:
        library = env.StaticLibrary(
            "demo/bin/libgodosk.{}.{}.a".format(env["platform"], env["target"]),
            source=sources,
        )

else:
    library = env.SharedLibrary(
        "demo/bin/libgodosk{}{}".format(env["suffix"], env["SHLIBSUFFIX"]),
        source=sources,
    )




Default(library)
