# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The Reluva contributors

# Reluva's mission scripts (docs/formats/newworlds/mission-scripts.md), built
# into the program by cmake/EmbedFiles.cmake: the rules every mission shares,
# then a file for each mission (with its side, for the joint missions, which
# have one for each), in the order missionScripts() lists them. A new
# script's file goes here too.
set(RELUVA_MISSION_SCRIPTS
    generic.nws
    # Federation
    f1.nws f2.nws f3.nws j1-f.nws j2-f.nws j3-f.nws j4-f.nws f8.nws j6-f.nws j7-f.nws f12.nws j8-f.nws
    f14.nws j9-f.nws c9.nws
    # Klingons
    k1.nws k2.nws j1-k.nws j2-k.nws j3-k.nws k7.nws j4-k.nws k8.nws j6-k.nws j7-k.nws k12.nws j8-k.nws
    k14.nws j9-k.nws c10.nws
    # Romulans
    r1.nws r2.nws j1-r.nws j2-r.nws j3-r.nws j4-r.nws r8.nws r9.nws j6-r.nws j7-r.nws r12.nws j8-r.nws
    r14.nws j9-r.nws c11.nws
    # Training (Federation)
    c1.nws c4.nws c5.nws c6.nws c7.nws c8.nws)
