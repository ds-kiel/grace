Import("env")

env.Append(LINKFLAGS=["-Wl-bDSCR_AREA=0x3e00",  "-Wl-bINT2JT=0x3f00"])
