// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Reluva contributors

// Reluva's mission scripts: what each mission reacts to and how. They
// follow the game's own logic, as studied in its executable
// (docs/formats/newworlds/exe.md#the-script-engine): the same objectives
// and fail criteria (the mission's strings, by id), triggers, conditions
// and delays, written in Reluva's own format and words - the messages
// paraphrase the game's. The rules every mission shares come first. Each
// script ends with what it can't say yet ("Not expressible yet").

#include "engines/newworlds/mission_script.h"

#include <string>

namespace reluva::newworlds {

namespace {

constexpr const char* kScripts[] = {
R"(
# --- Rules every mission shares (seen in every recording) -------------------
mission generic F
every built *
  say engineering "{1} finished."
every mine-exhausted
  say engineering "The mine at {a}, {b} has run out of minerals."
every power-low
  say operations "Not enough power: systems are shutting down. Build more generators."
every short-resources
  say operations "Construction held up: not enough resources. Build mines and a resource processor."
every short-crew
  say operations "Production held up: not enough crew. Build accommodation and a transporter pad."
every hub-destroyed *
  say intelligence "The {1} colony hub has been destroyed."
every side-failed *
  say intelligence "The {1} can no longer complete their mission."

# --- Federation ----------------------------------------------------------

# --- F1 as F ----------------------------------------
# F1 as the Federation, rewritten from the game's own graph (STNW.exe dump F1.json, slot 0,
# with the Klingon (1), Taubat (3) and temple (5) objects that report to the Federation).
mission F1 F
site klingon-tracks 358 202
site taubat-colony 330 174
site temple-approach 338 154
site temple-edge 338 150
site temple 332 152
objective 16
objective 17
objective 18
objective 19 hidden

# --- The temple's reactor is already failing: the temple starts at 40 % of its strength ---
on start
  damage taubat-temple 60

# --- The Klingons: a timetable of probes and attacks from the start ---------------------
on start
  timer kli-guard 1
  timer kli-recce 10
  timer kli-first-attack 100
  timer kli-attack-1 500
  timer kli-attack-2 800
  timer kli-attack-3 1100
  timer kli-random-attack 300
  timer kli-random-scout 300
  timer kli-scout-response 300
on timer kli-guard
  ai klingon klingon-colony-protection-one
  ai klingon klingon-colony-protection-two
  ai klingon klingon-colony-protection-three
on timer kli-recce
  say intelligence "Command urges caution: the Klingons have set up a colony to the south."
  ai klingon initial-recce
on timer kli-first-attack
  say intelligence "Incoming vehicle, Sir. Klingon, and it's making for the colony!"
  ai klingon initial-assault-one
  ai klingon initial-assault-two
  ai klingon initial-assault-three
on timer kli-attack-1
  ai klingon assault-one
on timer kli-attack-2
  ai klingon assault-two
on timer kli-attack-3
  ai klingon assault-three
on timer kli-scout-response
  ai klingon scout-response1
  ai klingon scout-response2
  ai klingon scout-response3
  ai klingon scout-response4
  ai klingon scout-response5
  ai klingon scout-response6
  ai klingon scout-response7
  ai klingon scout-response8
# every 300 s one of eight assaults, and one of eight scouting runs, at random
every timer kli-random-attack
  random kli-raid-1 kli-raid-2 kli-raid-3 kli-raid-4 kli-raid-5 kli-raid-6 kli-raid-7 kli-raid-8
  timer kli-random-attack 300
every signal kli-raid-1
  ai klingon assault-random1
every signal kli-raid-2
  ai klingon assault-random2
every signal kli-raid-3
  ai klingon assault-random3
every signal kli-raid-4
  ai klingon assault-random4
every signal kli-raid-5
  ai klingon assault-random5
every signal kli-raid-6
  ai klingon assault-random6
every signal kli-raid-7
  ai klingon assault-random7
every signal kli-raid-8
  ai klingon assault-random8
every timer kli-random-scout
  random kli-scout-1 kli-scout-2 kli-scout-3 kli-scout-4 kli-scout-5 kli-scout-6 kli-scout-7 kli-scout-8
  timer kli-random-scout 300
every signal kli-scout-1
  ai klingon scout-random1
every signal kli-scout-2
  ai klingon scout-random2
every signal kli-scout-3
  ai klingon scout-random3
every signal kli-scout-4
  ai klingon scout-random4
every signal kli-scout-5
  ai klingon scout-random5
every signal kli-scout-6
  ai klingon scout-random6
every signal kli-scout-7
  ai klingon scout-random7
every signal kli-scout-8
  ai klingon scout-random8
on seen klingon-tracks
  say security "Disruptor fire on the sensors. Looks like the Klingons mean to take this planet."

# --- Scans of things the mission has no use for ------------------------------------------
every scanned-nothing science
  say science "Routine science scans find nothing unusual, Sir."
every scanned-nothing strong-science
  say science "A detailed scan, Sir, but it's all just as I'd expect."
every scanned-nothing medical
  say medical "Routine medical scans show nothing wrong, Sir."
every scanned-nothing strong-medical
  say medical "The full diagnostics turn up nothing of note, Sir."
every scanned-nothing engineering
  say engineering "Structure and systems read normal, Sir."
every scanned-nothing strong-engineering
  say engineering "Nothing much here for engineering, Sir."

# --- Objective 16: the first of five buildings --------------------------------------------
on built-all vehicle-yard construction-yard power-generator resource-processor accommodation
  complete 16
  signal objective
  timer colony-built 5
on timer colony-built
  say command "The colony expansion is finished."

# --- The Taubat mine (science scans, 1 s to start, 10 s to finish) -------------------------
every scan-started science taubat-mine
  say science "Scan under way, Sir."
on scanned science taubat-mine
  say science "The scan's done, but the readings are unclear. I'd like someone else to look."
every scan-started strong-science taubat-mine
  say science "Scanning, with low-pass filters to clean up the signal."
every scan-lost strong-science taubat-mine
  say science "Lost the scan: it didn't run long enough, Sir."
on scanned strong-science taubat-mine
  say science "Done: it looks like a primitive mine, but the radiation there is unusually high."

# --- Objective 17: the plague in the Taubat colony ------------------------------------------
on seen taubat-colony
  say medical "We've located the centre of the plague. A medical scan of the central buildings would help."
every scan-started medical taubat-colony
  say medical "Starting the medical scan, Sir."
every scan-lost medical taubat-colony
  say medical "We lost the medical scan, Sir. We'll need another go."
on scanned medical taubat-colony
  say medical "The scan is over, but its results aren't conclusive. Someone with expertise would judge it better in person."
every scan-started strong-medical taubat-colony
  say medical "Starting a full diagnostic scan."
every scan-lost strong-medical taubat-colony
  say medical "The scan was lost. We'll have to repeat it to get a diagnosis."
on scanned strong-medical taubat-colony
  say medical "Done. Taubat biology is new to us, so I can't be certain, but I'd say radiation sickness."
  say medical "Let's have this studied at the colony. Our sickbay staff can handle the research."
  research taubat-colony-research
on researched taubat-colony-research
  complete 17
  signal objective
  signal radiation-hint
  timer plague-report-1 5
  timer plague-report-2 15
  timer plague-reminder 30
  timer research-done 20
on timer plague-report-1
  say medical "The research bears out the Doctor: radiation sickness, worsened by mutated strains of ordinary Taubat germs."
on timer plague-report-2
  say medical "Only an artificial source could give this much contamination. We must find it and shut it off, and soon."
on timer research-done
  say command "The medical research is finished."
on timer plague-reminder
  say medical "The Taubat are dying. We have to find where that radiation comes from, now."

# --- Objective 18: the temple ----------------------------------------------------------------
on seen temple-approach
  say "Incoming message" "Strangers, this ground is holy. Please, leave at once."
on seen temple-edge
  signal radiation-hint
# needs both the research and a look at the temple
on signal radiation-hint*2
  say engineering "Sensors read a powerful energy signature at {site:temple}, westward of the Taubat settlement. It's worth investigating."
every scan-started medical temple
  say medical "No wonder these Taubat are dying: the radiation is off the scale. Engineering should look into it."
every scan-started strong-medical temple
  say medical "No wonder these Taubat are dying: the radiation is off the scale. Engineering should look into it."
every scan-started science temple
  say science "Very high radiation, and the field gradients point to this building as the source. Engineering should know."
every scan-started strong-science temple
  say science "Very high radiation, and the field gradients point to this building as the source. Engineering should know."
# engineering scans: 30 s to finish
every scan-started engineering temple
  say engineering "Starting a systems scan."
every scan-lost engineering temple
  say engineering "The scan was cut short. We'll have to run it again for that data."
on scanned engineering temple
  say engineering "Scan done: the trouble seems to be a reactor containment field out of alignment."
  timer temple-apc-hint 10
  signal temple-surveyed
on timer temple-apc-hint
  say engineering "To repair the field generators we'll have to send an away team in an APC."
every scan-started strong-engineering temple
  say engineering "Starting a full systems scan."
every scan-lost strong-engineering temple
  say engineering "The scan was cut short. We'll have to run it again for that data."
on scanned strong-engineering temple
  say engineering "Scan done: quite an advanced reactor, but the Taubat have let it fall into disrepair."
  timer temple-field-hint 10
  signal temple-surveyed
on timer temple-field-hint
  say engineering "The main containment field is misaligned and leaking badly. Easy to fix, but we must get in quickly."
on signal temple-surveyed
  timer temple-report 20
# 20 s after the survey: objective 18 met, 19 revealed, the capture watch woken, and the
# Taubat's patience runs out
on timer temple-report
  complete 18
  reveal 19
  arm temple-capture
  signal objective
  timer temple-done 15
  timer temple-security 20
  timer taubat-demand 25
  timer taubat-death 1200
on timer temple-done
  say command "The engineering scan of the temple is finished."
on timer temple-security
  say security "We mustn't provoke the Taubat. Use a single APC to take the temple."
on timer taubat-demand
  say intelligence "Sir, the Taubat insist we leave this area. They give us five minutes."
  timer taubat-hostile 300
on timer taubat-hostile
  hostile taubat
on timer taubat-death
  fail 23 20
  say medical "The Taubat are dead. We couldn't stop the plague."
  signal failing

# --- Objective 19: take the temple and hold it 10 s (a raid that sleeps until the survey) ---------
every entered temple as temple-capture dormant
  say engineering "The away team is inside, Sir. Starting on the field realignment."
every capture-lost temple as temple-capture dormant
  say engineering "We left the temple too soon: the containment field is still unfixed."
on captured temple as temple-capture dormant
  say engineering "The containment field is aligned again and radiation is dropping. Just in time, Sir."
  complete 19
  signal objective

# --- The Klingon colony falls (if the player takes it) -----------------------------------
# their build timetable (con yard 60 s ... science block 660 s) runs by itself; losing
# their hub cancels the rest
on hub-destroyed klingon
  ai klingon abort kli-npc-build-con-yard
  ai klingon abort kli-npc-build-veh-yard
  ai klingon abort kli-npc-build-res-proc
  ai klingon abort kli-npc-build-accom-blck
  ai klingon abort kli-npc-build-shld-gen1
  ai klingon abort kli-npc-build-sci-blck

# --- The pass: four objectives, a message at 10 s, the win at 20 s ----------------------------
on signal objective*4
  timer pass-message 10
  timer pass 20
on timer pass-message
  say command "All objectives met. Well done."
on timer pass
  win

# --- Fails: the cause at once, "failed" at 10 s, the end at 20 s ------------------------------
on count taubat-hangar == 2
  fail 20 20
  say security "One of the Taubat colony's buildings has been destroyed."
  signal failing
on count taubat-house == 5
  fail 20 20
  say security "One of the Taubat colony's buildings has been destroyed."
  signal failing
on destroyed colony-hub
  fail 21 20
  say command "Our colony hub has been destroyed."
  signal failing
on destroyed temple
  fail 22 20
  say security "The Taubat temple has been destroyed."
  signal failing
on signal failing
  timer fail-message 10
on timer fail-message
  say command "We have not met our objectives."

# Not expressible yet:
# - Repeating a finished scan or capture gets its own line in the game ("consult a specialist",
#   "radiation still rising", (the game's line), "health deteriorating, research at once",
#   "containment field deteriorating", "field won't hold long", "temple retaken, field holding"):
#   no event for "the same thing again, after it succeeded".
# - The weak science scan of the mine has no "lost" line, the strong scans do (kept as written).
# - The Klingons' build timetable is part of their AI orders, not the script; the random
#   assaults and scouts are a 300 s repeating random pick among eight orders each.
# - The research topic's points and field (150000, medical) are research data, not script.

# --- F2 as F ----------------------------------------
# F2 as the Federation, rewritten from the game's own graph (STNW.exe dump F2.json, slot 0,
# with the Orion/Metar slot 5 objects that act on the Federation).
mission F2 F
site orion-drop-north 70 365
site orion-drop-west 46 309
site orion-drop-east 94 381
zone meteors-north 70 394 166 322
zone meteors-middle 178 374 262 290
zone meteors-south 274 398 362 262
objective 14
objective 15
objective 16
objective 17 hidden
objective 18 hidden optional

# --- Meteor storms over three stretches of the map, all mission long ----------------------
on start
  meteors meteors-north 1
  meteors meteors-middle 1.5
  meteors meteors-south 2

# --- Orion pirates: from 360 s, every 240 s one of eight raids at random -----------------
on start
  timer starship-ready 1800
  timer orion-raids 360
every timer orion-raids
  random orion-raid-1 orion-raid-2 orion-raid-3 orion-raid-4 orion-raid-5 orion-raid-6 orion-raid-7 orion-raid-8
  timer orion-raids 240
# each raid: its order beams four pirates down at a drop point, then sends them at the colony
every signal orion-raid-1
  ai orion orion-assault-fed-random1
every signal orion-raid-2
  ai orion orion-assault-fed-random2
every signal orion-raid-3
  ai orion orion-assault-fed-random3
every signal orion-raid-4
  ai orion orion-assault-fed-random4
every signal orion-raid-5
  ai orion orion-assault-fed-random5
)",
R"(every signal orion-raid-6
  ai orion orion-assault-fed-random6
every signal orion-raid-7
  ai orion orion-assault-fed-random7
every signal orion-raid-8
  ai orion orion-assault-fed-random8

# --- Scans of things the mission has no use for ------------------------------------------
every scanned-nothing science
  say science "Routine science scans find nothing unusual, Sir."
every scanned-nothing strong-science
  say science "A detailed scan, Sir, but it's all just as I'd expect."
every scanned-nothing medical
  say medical "Routine medical scans show nothing wrong, Sir."
every scanned-nothing strong-medical
  say medical "The full diagnostics turn up nothing of note, Sir."
every scanned-nothing engineering
  say engineering "Structure and systems read normal, Sir."
every scanned-nothing strong-engineering
  say engineering "Nothing much here for engineering, Sir."

# --- Objective 14: the first of six buildings ----------------------------------------------
on built-all vehicle-yard construction-yard accommodation mining-station resource-processor transporter-pad
  complete 14
  signal primary
  timer first-stage 5
on timer first-stage
  say command "Sir, the first stage of the colony is finished."

# --- Objective 15: 120 T of dilithium for the starship, 1800 s in, within 180 s -------------
on timer starship-ready
  say communications "The starship is ready to take the supplies aboard."
  timer delivery-deadline 180
  arm delivery
# the dilithium check sleeps until the starship is ready, then looks at the stock as it is
on stock dilithium 120 as delivery dormant
  stop delivery-deadline
  say communications "The supplies are on their way up to the ship. Objective done."
  take dilithium 120
  complete 15
  signal primary
on timer delivery-deadline
  fail 20 20
  say communications "We can't hold on for the delivery any longer. Primary objective 2 has failed."
  signal failing

# --- Objective 16: the crashed starship (science scans, 1 s to start, 10 s to finish) ------
every scan-started science crashed-starship
  say science "Scan under way, Sir."
on scanned science crashed-starship
  say science "The scan's done, but the readings are unclear. I'd like someone else to look."
every scan-started strong-science crashed-starship
  say science "Scanning, with low-pass filters to clean up the signal."
every scan-lost strong-science crashed-starship
  say science "Lost the scan: it didn't run long enough, Sir."
on scanned strong-science crashed-starship
  say science "Finished. The data is going to the science building now; research on it should start right away."
  research crash-site-research
  arm wreck-medical
on researched crash-site-research
  complete 16
  disarm site-fail
  signal primary
  timer derelict-1 1
  timer primary-3-done 5
  timer derelict-2 8
on timer derelict-1
  say science "It's a derelict starship, age unknown. It may be Taubat."
on timer primary-3-done
  say command "Primary objective 3 is complete."
on timer derelict-2
  say science "The Doctor should give the wreck a medical scan to trace its bio-patterns."
on signal primary*3
  say command "The primary objectives are complete, Sir."
  signal stage

# --- Objective 17: the virus on the wreck (medical scans asleep until the strong science scan) --
every scan-started medical crashed-starship as wreck-medical dormant
  say medical "I'm picking up particles in the air. I'll need more skill to study them."
on scan-started strong-medical crashed-starship as wreck-medical dormant
  say medical "We've found an airborne virus, and it's very contagious. A sickbay should research it now."
  reveal 17
  signal disease-research
  timer infection 10
# the research needs the strong medical scan and the first sickbay
on built sickbay
  signal disease-research
on signal disease-research*2
  research disease-research
on researched disease-research
  complete 17
  say medical "The research identifies it as xenopolycythemia. Now trying to isolate the viral markers..."
  timer secondary-1-done 10
  signal stage
  signal cure
on timer secondary-1-done
  say command "Secondary objective 1 is complete."
  reveal 18

# --- The epidemic: it breaks out 10 s after the strong medical scan starts ------------------
on timer infection
  infect
  signal cure
on infection 1
  say medical "The infection is spreading. Sickbay must research a cure and get it out at once!"
# these two sleep once the cure is under way
on infection 30 as disease-warnings
  say medical "We must act against this disease before it kills the colony!"
on infection 80 as disease-warnings
  fail 21 20
  say medical "The disease has swept through the whole colony."
  signal failing

# --- Objective 18: the cure (needs the outbreak and the research), 10 s later --------------
on signal cure*2
  timer disease-shut-off 10
  timer cure 10
on timer disease-shut-off
  disarm disease-warnings
on timer cure
  say medical "We have a cure. Releasing it through the colony's life support now."
  cure
on cured
  say medical "The xenopolycythemia outbreak is under control."
  complete 18
  timer secondary-2-done 10
on timer secondary-2-done
  say command "Secondary objective 2 is complete."

# --- The pass: primaries and secondary 1, a message at 10 s, the win at 20 s -----------------
on signal stage*2
  timer pass-message 10
  timer pass 20
on timer pass-message
  say command "All objectives met. Well done."
on timer pass
  win

# --- Fails: the cause at once, "failed" at 10 s, the end at 20 s ----------------------------
on destroyed colony-hub
  fail 19 20
  say command "Our colony hub has been destroyed."
  signal failing
# watched only until objective 16 is met
on destroyed crashed-starship as site-fail
  fail 22 20
  say command "The archaeological site has been destroyed."
  signal failing
on signal failing
  timer fail-message 10
on timer fail-message
  say command "We have not met our objectives."

# Not expressible yet:
# - Repeating a finished scan gets its own line ("consult a specialist", "need longer to get
#   the information").

# --- F3 as F ----------------------------------------
# F3 as the Federation, rewritten from the game's own graph (STNW.exe dump F3.json, slot 0,
# with the Klingon (1), Romulan (2) and Taubat (3) objects that report to the Federation).
mission F3 F
objective 15
objective 16
objective 17

# --- Klingon and Romulan raids: from 300 s, every 180 s each picks one of eight at random ---
on start
  timer kli-raids 300
  timer rom-raids 300
every timer kli-raids
  random kli-raid-1 kli-raid-2 kli-raid-3 kli-raid-4 kli-raid-5 kli-raid-6 kli-raid-7 kli-raid-8
  timer kli-raids 180
every signal kli-raid-1
  ai klingon kli-assault-fed-random1
every signal kli-raid-2
  ai klingon kli-assault-fed-random2
every signal kli-raid-3
  ai klingon kli-assault-fed-random3
every signal kli-raid-4
  ai klingon kli-assault-fed-random4
every signal kli-raid-5
  ai klingon kli-assault-fed-random5
every signal kli-raid-6
  ai klingon kli-assault-fed-random6
every signal kli-raid-7
  ai klingon kli-assault-fed-random7
every signal kli-raid-8
  ai klingon kli-assault-fed-random8
every timer rom-raids
  random rom-raid-1 rom-raid-2 rom-raid-3 rom-raid-4 rom-raid-5 rom-raid-6 rom-raid-7 rom-raid-8
  timer rom-raids 180
every signal rom-raid-1
  ai romulan rom-assault-fed-random1
every signal rom-raid-2
  ai romulan rom-assault-fed-random2
every signal rom-raid-3
  ai romulan rom-assault-fed-random3
every signal rom-raid-4
  ai romulan rom-assault-fed-random4
every signal rom-raid-5
  ai romulan rom-assault-fed-random5
every signal rom-raid-6
  ai romulan rom-assault-fed-random6
every signal rom-raid-7
  ai romulan rom-assault-fed-random7
every signal rom-raid-8
  ai romulan rom-assault-fed-random8

# --- Objective 15: the first of six buildings -----------------------------------------------
on built-all sickbay vehicle-yard construction-yard science-station power-generator transporter-pad
  say command "Every building we needed is up, Sir."
  complete 15
  signal objective

# --- The Taubat ("Inbetweener") colony under attack ------------------------------------------
on count taubat-house == 11
  say "Incoming message" "Help! They are destroying us."
# two of these three and the colony counts as lost (each watch fires once, then sleeps)
every count taubat-hangar == 0 as hangars-ruined
  disarm hangars-ruined
  signal taubat-ruin
every count taubat-house == 9 as houses-ruined
  disarm houses-ruined
  signal taubat-ruin
every count taubat-temple == 0 as temple-ruined
  disarm temple-ruined
  signal taubat-ruin
on signal taubat-ruin*2
  fail 18 20
  say command "The Inbetweener colony has been laid waste. Primary objective 2 has failed."
  signal failing

# --- Objective 16: the Klingon photon artillery ------------------------------------------------
on count klingon-photon-artillery == 0
  say command "The Klingon attackers are all destroyed. Primary objective 2 is complete."
  complete 16
  signal objective
# (the shared rules announce the fall of a hub)
on hub-destroyed klingon
  ai klingon abort kli-npc-build-con-yard
  ai klingon abort kli-npc-build-veh-yard
  ai klingon abort kli-npc-build-advpwrgen-one
  ai klingon abort kli-npc-build-advpwrgen-two
  ai klingon abort kli-npc-build-shld-gen
  ai klingon abort kli-npc-build-def-weap-one
  ai klingon abort kli-npc-build-def-weap-two
  ai klingon abort kli-npc-build-def-art

# --- Objective 17: the Romulan cloaked tanks ----------------------------------------------------
on count romulan-cloaked-tank == 0
  say command "The Romulan saboteurs are all destroyed. Primary objective 3 is complete."
  complete 17
  signal objective
on hub-destroyed romulan
  ai romulan abort rom-npc-build-con-yard
  ai romulan abort rom-npc-build-veh-yard
  ai romulan abort rom-npc-build-advpwrgen-one
  ai romulan abort rom-npc-build-advpwrgen-two
  ai romulan abort rom-npc-build-shld-gen
  ai romulan abort rom-npc-build-def-weap-one
  ai romulan abort rom-npc-build-def-weap-two
  ai romulan abort rom-npc-build-def-art

# --- The pass: three objectives, a message at once, the win at 20 s ----------------------------
on signal objective*3
  say command "The primary objectives are complete, Sir."
  timer pass 20
on timer pass
  win

# --- Fails: the cause at once, "failed" at 10 s, the end at 20 s ------------------------------
on destroyed colony-hub
  fail 19 20
  say command "Our colony hub has been destroyed."
  signal failing
on signal failing
  timer fail-message 10
on timer fail-message
  say command "We have not met our objectives."

# Not expressible yet:
# - "Two of three Taubat losses" counts monitor events (a 2-relay); written as three
#   self-disarming handlers feeding a signal counter (once-only "on" handlers would be fired by
#   script-check's walkthrough).
# - The Klingons' and Romulans' build timetables (their named build orders and unit builds)
#   are AI order data, not script; the script only cancels the named builds when a hub falls.
# - The Hubrin slot holds only a dummy object (keeps the side alive).

# --- J1 as F ----------------------------------------
# J1 "Into the Arena", played as the Federation (slot 0).
# Rewritten from the game's own mission graph (scripts_out/J1.json).
mission J1 F
site federation-colony 426 69
site taubat-colony 298 205
site temple 394 309
objective 21
objective 23
objective 25 hidden

# --- The other sides' attacks on the colony -------------------------------
# A Klingon strike 10 s in; from 480 s a random Klingon and a random Romulan
# assault every 180 s (each picks one of eight routes).
on start
  timer kli-assault-federation 10
  timer kli-raids-on-fed 480
  timer rom-raids-on-fed 480
on timer kli-assault-federation
  ai klingon kli-assault-federation-1
every timer kli-raids-on-fed
  random kli-assault-fed-random1 kli-assault-fed-random2 kli-assault-fed-random3 kli-assault-fed-random4 kli-assault-fed-random5 kli-assault-fed-random6 kli-assault-fed-random7 kli-assault-fed-random8
  timer kli-raids-on-fed 180
every signal kli-assault-fed-random1
  ai klingon kli-assault-fed-random1
every signal kli-assault-fed-random2
  ai klingon kli-assault-fed-random2
every signal kli-assault-fed-random3
  ai klingon kli-assault-fed-random3
every signal kli-assault-fed-random4
  ai klingon kli-assault-fed-random4
every signal kli-assault-fed-random5
  ai klingon kli-assault-fed-random5
every signal kli-assault-fed-random6
  ai klingon kli-assault-fed-random6
every signal kli-assault-fed-random7
  ai klingon kli-assault-fed-random7
every signal kli-assault-fed-random8
  ai klingon kli-assault-fed-random8
every timer rom-raids-on-fed
  random rom-assault-fed-random1 rom-assault-fed-random2 rom-assault-fed-random3 rom-assault-fed-random4 rom-assault-fed-random5 rom-assault-fed-random6 rom-assault-fed-random7 rom-assault-fed-random8
  timer rom-raids-on-fed 180
every signal rom-assault-fed-random1
  ai romulan rom-assault-fed-random1
every signal rom-assault-fed-random2
  ai romulan rom-assault-fed-random2
every signal rom-assault-fed-random3
  ai romulan rom-assault-fed-random3
every signal rom-assault-fed-random4
  ai romulan rom-assault-fed-random4
every signal rom-assault-fed-random5
  ai romulan rom-assault-fed-random5
every signal rom-assault-fed-random6
  ai romulan rom-assault-fed-random6
every signal rom-assault-fed-random7
  ai romulan rom-assault-fed-random7
every signal rom-assault-fed-random8
  ai romulan rom-assault-fed-random8

# --- Objective 21: expand the colony threefold ----------------------------
# At twice its starting size the Romulans take notice and send two assaults.
on colony-size 200
  ai romulan rom-assault-federation-1
  ai romulan rom-assault-federation-2
on colony-size 300
  say engineering "The colony's development is complete, sir."
  complete 21

# --- Objective 23: scan the radiation source (the temple) -----------------
on scanned science temple
  complete 23
  reveal 25

# --- Objective 25: take the temple and hold it for a minute ---------------
# Our capture monitor reports each entry until the hold succeeds, then stops.
every entered temple as fed-temple-watch
  say engineering "We're inside the Taubat temple, sir."
every vacated temple
  say intelligence "Nobody holds the Taubat temple now."
on held temple 60
  say engineering "The alien power source is stable now, sir."
  complete 25
  disarm fed-temple-watch
  disarm temple-unheld   # a held temple can no longer fail us by being destroyed

# --- What the rivals do at the temple and the settlement -------------------
# (The captures can't come from the computer's sides: they never capture.)
every captured-by klingon temple
  say intelligence "The Klingons have got into the Taubat temple."
every captured-by romulan temple
  say intelligence "The Romulans have got into the Taubat temple."
every captured-by romulan taubat-colony 30
  say intelligence "A Romulan raid has seized part of the Taubat settlement."
)",
R"(on count klingon-disruptor-tank == 14
  say intelligence "You should know, sir: the Klingons are massing a large force."
on destroyed temple
  say intelligence "The Taubat temple has been destroyed."

# --- Fails: the cause at once, a line at 10 s, the mission lost at 20 s ----
fail 22 count colony-hub == 0
  say intelligence "The Federation colony hub has been destroyed."
  signal failing
fail 24 colony-total taubat 50   # the settlement down to half (Klingon work, usually)
  say intelligence "The Taubat settlement has been laid waste."
  signal failing
# The temple destroyed before anyone has held it: any side's hold switches
# this off.
fail 24 destroyed temple as temple-unheld
  signal failing
# A rival's hold on the temple (fail 26); only a human rival can capture.
on held-by klingon temple 60
  fail 26 20
  say intelligence "The Klingons now control the Taubat power source."
  disarm temple-unheld
  signal failing
on held-by romulan temple 60
  fail 26 20
  say intelligence "The Romulans now control the Taubat power source."
  disarm temple-unheld
  signal failing
on signal failing
  timer fail-notice 10
on timer fail-notice
  say intelligence "The Federation is unable to finish its mission."

# --- Mission pass: all three objectives, a line at 10 s, the win at 20 s ----
on all-completed
  timer pass-notice 10
  timer pass 20
on timer pass-notice
  say intelligence "Mission accomplished for the Federation."
on timer pass
  win

# Not expressible yet:
# - captured-by / held-by <side> temple and captured-by romulan taubat-colony
#   30 never fire: the computer's sides never capture (kept for a human rival).
# - The temple scan is also wired to a stance switch (Tau_hostile_to_Federation)
#   that the mission doesn't have, so nothing happens: the Taubat stay neutral.
# - The rivals' own pass/fail lines ("the Klingons have completed their
#   mission", 10 s after their counter fills) and what the game's referee does
#   with a rival's pass; the generic side-failed rule covers only the fail.
# - The Federation's own AI orders (build timetable, aborted when the hub falls;
#   assaults on the Romulans at 1200/1260 s and on the Klingons when they have
#   14 tanks; random raids from 480 s) only run when the side isn't played.

# --- J2 as F ----------------------------------------
# J2 "Absent Friends", played as the Federation (slot 0).
# Rewritten from the game's own mission graph (scripts_out/J2.json).
mission J2 F
site federation-colony 192 351
site alien-structure 112 280   # the Hubrin fortress, as the 120 s message gives it
site taubat-hangar 120 295     # where the staff are held (group Taubat_Hangar)
zone magnetic-storm 244 284 252 292
zone fortress-approach 96 236 148 296
objective 9
objective 10 hidden

# --- The magnetic storm, flashing all mission ------------------------------
on start
  storm magnetic-storm

# --- Romulans near the Hubrin fortress: their Bird of Prey flies its route --
on inside romulan fortress-approach
  move rom-bop 145 246 100 299 172 299 50 222

# --- Timed messages, and the Hubrin turning on everyone at 120 s ------------
on start
  timer storm-warning 60
  timer kidnap 120
  timer defence-advice 180
  timer weapons-advice 300
  timer mining-advice 360
  timer kli-assault-federation 80
  timer kli-raids-on-fed 420
  timer rom-raids-on-fed 420
  timer hubrin-raids-on-fed 420
on timer storm-warning
  say intelligence "Our colony shield buffer is fluctuating. I can't be sure, but the magnetic storm may be the cause."
on timer kidnap
  hostile hubrin
  say intelligence "Two of our staff have gone missing! There's a transporter trace near an alien structure at {site:alien-structure}. Start a rescue right away!"
  complete 9
  reveal 10
on timer defence-advice
  say security "We must defend the colony and build our best technology against beings this strong. No wonder the Taubat left this world!"
on timer weapons-advice
  say security "Reports say photon tanks and turrets on the mountain tops may be the answer to the Hubrin's greater range and firepower."
on timer mining-advice
  say intelligence "To develop the colony fully we should set up remote mining sites, guarded by shield generators and turrets."

# --- The other sides' attacks on the colony -------------------------------
# Two Klingon assaults at 80 s; from 420 s a random Klingon and Romulan
# assault every 180 s; from 420 s a Hubrin raider every 120 s until the
# Hubrin fortress falls (each order beams its raider in near 82:325); and the
# Romulans turn on us once the Klingon colony has been wiped out: four "none
# left" watches, three on the Klingons' small buildings and one on their large
# ones, all of which must report.
on timer kli-assault-federation
  ai klingon kli-federation-assault-one
  ai klingon kli-federation-assault-two
every timer kli-raids-on-fed
  random kli-assault-fed-random1 kli-assault-fed-random2 kli-assault-fed-random3 kli-assault-fed-random4 kli-assault-fed-random5 kli-assault-fed-random6 kli-assault-fed-random7 kli-assault-fed-random8
  timer kli-raids-on-fed 180
every signal kli-assault-fed-random1
  ai klingon kli-assault-fed-random1
every signal kli-assault-fed-random2
  ai klingon kli-assault-fed-random2
every signal kli-assault-fed-random3
  ai klingon kli-assault-fed-random3
every signal kli-assault-fed-random4
  ai klingon kli-assault-fed-random4
every signal kli-assault-fed-random5
  ai klingon kli-assault-fed-random5
every signal kli-assault-fed-random6
  ai klingon kli-assault-fed-random6
every signal kli-assault-fed-random7
  ai klingon kli-assault-fed-random7
every signal kli-assault-fed-random8
  ai klingon kli-assault-fed-random8
every timer rom-raids-on-fed
  random rom-assault-fed-random1 rom-assault-fed-random2 rom-assault-fed-random3 rom-assault-fed-random4 rom-assault-fed-random5 rom-assault-fed-random6 rom-assault-fed-random7 rom-assault-fed-random8
  timer rom-raids-on-fed 180
every signal rom-assault-fed-random1
  ai romulan rom-assault-fed-random1
every signal rom-assault-fed-random2
  ai romulan rom-assault-fed-random2
every signal rom-assault-fed-random3
  ai romulan rom-assault-fed-random3
every signal rom-assault-fed-random4
  ai romulan rom-assault-fed-random4
every signal rom-assault-fed-random5
  ai romulan rom-assault-fed-random5
every signal rom-assault-fed-random6
  ai romulan rom-assault-fed-random6
every signal rom-assault-fed-random7
  ai romulan rom-assault-fed-random7
every signal rom-assault-fed-random8
  ai romulan rom-assault-fed-random8
every timer hubrin-raids-on-fed
  random hubrin-assault-fed-random1 hubrin-assault-fed-random2 hubrin-assault-fed-random3 hubrin-assault-fed-random4 hubrin-assault-fed-random5 hubrin-assault-fed-random6 hubrin-assault-fed-random7 hubrin-assault-fed-random8
  timer hubrin-raids-on-fed 120
every signal hubrin-assault-fed-random1
  ai hubrin hubrin-assault-fed-random1
every signal hubrin-assault-fed-random2
  ai hubrin hubrin-assault-fed-random2
every signal hubrin-assault-fed-random3
  ai hubrin hubrin-assault-fed-random3
every signal hubrin-assault-fed-random4
  ai hubrin hubrin-assault-fed-random4
every signal hubrin-assault-fed-random5
  ai hubrin hubrin-assault-fed-random5
every signal hubrin-assault-fed-random6
  ai hubrin hubrin-assault-fed-random6
every signal hubrin-assault-fed-random7
  ai hubrin hubrin-assault-fed-random7
every signal hubrin-assault-fed-random8
  ai hubrin hubrin-assault-fed-random8
every destroyed hubrin-fortress
  stop hubrin-raids-on-fed
on count klingon-small-buildings <= 0
  signal kli-colony-down
on count klingon-large-buildings <= 0
  signal kli-colony-down
on signal kli-colony-down*2
  ai romulan rom-federation-assault-one
  ai romulan rom-federation-assault-two

# --- Objective 10: rescue the staff from the Taubat hangar ----------------
# The raid: hold the hangar 10 s to start the search, 60 s more to finish it.
every entered taubat-hangar
  say intelligence "Searching the building for our staff."
every capture-lost taubat-hangar
  say intelligence "Something's wrong: the recovery team has stopped answering."
on captured taubat-hangar as fed-raid
  say intelligence "Our people aren't here. Faint bio-traces show they were taken elsewhere a while back."
  complete 10
  timer new-orders 7
  timer pass 10
on timer new-orders
  say communications "New orders from command: we're being moved to a new outpost, and another team will look into the loss of our staff."
on timer pass
  win
# A rival finishing its raid while ours is still running beats us (fail 12);
# once a rival has finished, our own success is futile (nothing happens).
# (A rival's raid here is a capture, which only a human rival can make.)
on beaten taubat-hangar
  fail 12 10
  say intelligence "A garbled message from our staff: it seems the Klingons or the Romulans have raided the building."
every captured-by klingon taubat-hangar
  timer kli-took-staff 7
every captured-by romulan taubat-hangar
  timer rom-took-staff 7
on timer kli-took-staff
  say intelligence "The Klingons have taken the building and our staff with it."
on timer rom-took-staff
  say intelligence "The Romulans have taken the building and our staff with it."

# --- Fails: the cause at once, the mission lost 10 s later ----------------
on count colony-hub == 0
  fail 11 10
  say intelligence "The Federation colony hub has been destroyed."
on destroyed taubat-hangar
  fail 12 10
  say operations "The Taubat hangar holding our staff has been destroyed. The mission has failed."

# Not expressible yet:
# - captured-by <side> taubat-hangar and beaten never fire against the
#   computer's sides: they never capture.
# - The objective sent at 120 s is shown only as the objective change; the
#   game's "new primary objective" mail goes to a distributor, not the panel.
# - The Federation's own AI orders (colony guard at 16 s, assault on the
#   Romulans at 80 s, random raids, an assault on the Klingons when the Romulan
#   colony is gone) only run when the side isn't played.

# --- J3 as F ----------------------------------------
# J3 "The Mask Slips", played as the Federation (slot 0).
# Rewritten from the game's own mission graph (scripts_out/J3.json).
mission J3 F
site federation-colony 96 231
site workshops 168 239          # the eight Taubat workshops (Raid_Building_One..Eight), 152-184:223-255
site metar-scan-building 176 239   # the "major building of the ruined city": the Metar colony core
objective 13
objective 14 hidden

# --- Greetings --------------------------------------------------------------
# (the colony's commodity top-up runs from the start with an empty bundle: nothing to write)
on start
  timer taubat-plea 5
  timer caution 15
  timer kli-assault-federation 80
  timer kli-raids-on-fed 360
  timer kli-heavy-raids-on-fed 540
  timer rom-raids-on-fed 360
  timer rom-heavy-raids-on-fed 540
  timer kli-scouts 60
  timer rom-scouts 60
on timer taubat-plea
  say "Incoming message" "Friends, help us! Invaders are marching over our world, and darkness comes with them."
on timer caution
  say operations "Sir, it seems the Klingons and Romulans came here looking for trouble. Let's stay on guard."

# --- The other sides' attacks on the colony -------------------------------
# Two Klingon assaults at 80 s; Klingon and Romulan raids every 120 s from
# 360 s, heavier ones every 120 s from 540 s; a scout every 60 s from each;
# and the Romulans turn on us once the Klingon colony is wiped out.
on timer kli-assault-federation
  ai klingon kli-federation-assault-one
  ai klingon kli-federation-assault-two
every timer kli-raids-on-fed
  random kli-assault-fed-random1 kli-assault-fed-random2 kli-assault-fed-random3 kli-assault-fed-random1 kli-assault-fed-random2 kli-assault-fed-random3 kli-assault-fed-random2 kli-assault-fed-random3
  timer kli-raids-on-fed 120
every signal kli-assault-fed-random1
  ai klingon kli-assault-fed-random1
every signal kli-assault-fed-random2
  ai klingon kli-assault-fed-random2
every signal kli-assault-fed-random3
  ai klingon kli-assault-fed-random3
every timer kli-heavy-raids-on-fed
  random kli-assault-fed-random1a kli-assault-fed-random2a kli-assault-fed-random3a kli-assault-fed-random4a kli-assault-fed-random1a kli-assault-fed-random2a kli-assault-fed-random3a kli-assault-fed-random4a
  timer kli-heavy-raids-on-fed 120
every signal kli-assault-fed-random1a
  ai klingon kli-assault-fed-random1a
every signal kli-assault-fed-random2a
  ai klingon kli-assault-fed-random2a
every signal kli-assault-fed-random3a
  ai klingon kli-assault-fed-random3a
every signal kli-assault-fed-random4a
  ai klingon kli-assault-fed-random4a
every timer rom-raids-on-fed
  random rom-assault-fed-random4 rom-assault-fed-random5 rom-assault-fed-random6 rom-assault-fed-random4 rom-assault-fed-random5 rom-assault-fed-random6 rom-assault-fed-random5 rom-assault-fed-random6
  timer rom-raids-on-fed 120
every signal rom-assault-fed-random4
  ai romulan rom-assault-fed-random4
every signal rom-assault-fed-random5
  ai romulan rom-assault-fed-random5
every signal rom-assault-fed-random6
  ai romulan rom-assault-fed-random6
every timer rom-heavy-raids-on-fed
  random rom-assault-fed-random5a rom-assault-fed-random6a rom-assault-fed-random7a rom-assault-fed-random8a rom-assault-fed-random5a rom-assault-fed-random6a rom-assault-fed-random7a rom-assault-fed-random8a
  timer rom-heavy-raids-on-fed 120
every signal rom-assault-fed-random5a
  ai romulan rom-assault-fed-random5a
every signal rom-assault-fed-random6a
  ai romulan rom-assault-fed-random6a
every signal rom-assault-fed-random7a
  ai romulan rom-assault-fed-random7a
every signal rom-assault-fed-random8a
  ai romulan rom-assault-fed-random8a
every timer kli-scouts
  random kli-scout-fed-random1 kli-scout-fed-random2 kli-scout-fed-random3 kli-scout-fed-random4 kli-scout-rom-random5 kli-scout-rom-random6 kli-scout-rom-random7 kli-scout-rom-random8
  timer kli-scouts 60
every signal kli-scout-fed-random1
  ai klingon kli-scout-fed-random1
every signal kli-scout-fed-random2
  ai klingon kli-scout-fed-random2
every signal kli-scout-fed-random3
  ai klingon kli-scout-fed-random3
every signal kli-scout-fed-random4
  ai klingon kli-scout-fed-random4
every signal kli-scout-rom-random5
  ai klingon kli-scout-rom-random5
every signal kli-scout-rom-random6
  ai klingon kli-scout-rom-random6
every signal kli-scout-rom-random7
  ai klingon kli-scout-rom-random7
every signal kli-scout-rom-random8
  ai klingon kli-scout-rom-random8
every timer rom-scouts
  random rom-scout-kli-random1 rom-scout-kli-random2 rom-scout-kli-random3 rom-scout-kli-random4 rom-scout-fed-random5 rom-scout-fed-random6 rom-scout-fed-random7 rom-scout-fed-random8
  timer rom-scouts 60
every signal rom-scout-kli-random1
  ai romulan rom-scout-kli-random1
every signal rom-scout-kli-random2
  ai romulan rom-scout-kli-random2
every signal rom-scout-kli-random3
  ai romulan rom-scout-kli-random3
every signal rom-scout-kli-random4
  ai romulan rom-scout-kli-random4
)",
R"(every signal rom-scout-fed-random5
  ai romulan rom-scout-fed-random5
every signal rom-scout-fed-random6
  ai romulan rom-scout-fed-random6
every signal rom-scout-fed-random7
  ai romulan rom-scout-fed-random7
every signal rom-scout-fed-random8
  ai romulan rom-scout-fed-random8
# (Four "none left" watches: three on the Klingons' small buildings, one on
# their large ones; all four must report.)
on count klingon-small-buildings <= 0
  signal kli-colony-down
on count klingon-large-buildings <= 0
  signal kli-colony-down
on signal kli-colony-down*2
  ai romulan rom-federation-assault-one
  ai romulan rom-federation-assault-two

# --- Objective 13: scan three of the Taubat workshops ----------------------
# Each scan must hold 5 s to start and 30 s more to count. Each of the eight
# workshops (raid-building-one .. -eight) has its own watch.
every scan-started science raid-building-one
  signal workshop-scan-on
every scan-started science raid-building-two
  signal workshop-scan-on
every scan-started science raid-building-three
  signal workshop-scan-on
every scan-started science raid-building-four
  signal workshop-scan-on
every scan-started science raid-building-five
  signal workshop-scan-on
every scan-started science raid-building-six
  signal workshop-scan-on
every scan-started science raid-building-seven
  signal workshop-scan-on
every scan-started science raid-building-eight
  signal workshop-scan-on
every scan-lost science raid-building-one
  signal workshop-scan-off
every scan-lost science raid-building-two
  signal workshop-scan-off
every scan-lost science raid-building-three
  signal workshop-scan-off
every scan-lost science raid-building-four
  signal workshop-scan-off
every scan-lost science raid-building-five
  signal workshop-scan-off
every scan-lost science raid-building-six
  signal workshop-scan-off
every scan-lost science raid-building-seven
  signal workshop-scan-off
every scan-lost science raid-building-eight
  signal workshop-scan-off
every scanned science raid-building-one
  signal workshop-scanned
every scanned science raid-building-two
  signal workshop-scanned
every scanned science raid-building-three
  signal workshop-scanned
every scanned science raid-building-four
  signal workshop-scanned
every scanned science raid-building-five
  signal workshop-scanned
every scanned science raid-building-six
  signal workshop-scanned
every scanned science raid-building-seven
  signal workshop-scanned
every scanned science raid-building-eight
  signal workshop-scanned
every signal workshop-scan-on
  say science "Scanners running now, sir."
every signal workshop-scan-off
  say science "The scan has dropped out, sir. It needs another attempt."
every signal workshop-scanned
  say science "Scan of the Taubat hangar complete, sir."
on scanned science raid-building-one     # any three of the eight
on scanned science raid-building-two
on scanned science raid-building-three
on signal workshop-scanned*3
  say science "The Taubat hangar scans are complete."
  complete 13
  reveal 14
  signal survive-step
  signal pass-step
# A workshop a rival got first is lost to us: beaten (a rival finished during
# our scan) or futile (our scan finished after a rival's); six is fail 18.
# The Klingons' destroy watch succeeds whenever a workshop falls, whoever
# destroys it; the Romulans' capture watch only for a human player.
every beaten raid-building-one
  signal workshop-lost
every beaten raid-building-two
  signal workshop-lost
every beaten raid-building-three
  signal workshop-lost
every beaten raid-building-four
  signal workshop-lost
every beaten raid-building-five
  signal workshop-lost
every beaten raid-building-six
  signal workshop-lost
every beaten raid-building-seven
  signal workshop-lost
every beaten raid-building-eight
  signal workshop-lost
every futile raid-building-one
  signal workshop-lost
every futile raid-building-two
  signal workshop-lost
every futile raid-building-three
  signal workshop-lost
every futile raid-building-four
  signal workshop-lost
every futile raid-building-five
  signal workshop-lost
every futile raid-building-six
  signal workshop-lost
every futile raid-building-seven
  signal workshop-lost
every futile raid-building-eight
  signal workshop-lost
on signal workshop-lost*6
  fail 18 10
  say operations "Too many hangars have been destroyed or captured. We can't complete the mission."

# --- Objective 14: scan the major building of the ruined city -------------
# Held 10 s to start, 60 s more to finish. It wakes the Metar.
every scan-started science metar-scan-building
  say science "Scanners running now, sir."
every scan-lost science metar-scan-building
  say science "The scan has dropped out, sir. It needs another attempt."
on scanned science metar-scan-building as fed-core-scan
  say science "Sir, the scan of the ruined city's main building is done."
  reveal 14
  complete 14
  signal pass-step
  signal metar-wake
  disarm core-watch   # once scanned, its destruction no longer fails us

# --- The Metar wake: our scan, a rival's scan, or a portal destroyed --------
# (A rival's core scan, which makes ours futile, needs a human rival: the
# computer's sides never scan.)
every scanned-by klingon metar-scan-building
  signal metar-wake
every scanned-by romulan metar-scan-building
  signal metar-wake
every count metar-portal == 2   # one of the three portals destroyed
  timer portal-surge 5
  timer portal-wake 10
on timer portal-surge
  say operations "An alien structure is down, and surges of power are running through the planet's core."
on timer portal-wake
  signal metar-wake
on signal metar-wake
  timer taubat-turn 5
  timer metar-stirs 10
  timer metar-lifeforms 25
  timer metar-outposts 30
  timer metar-raids-on-fed 60
  timer tau-raids-on-fed 20
  signal survive-step
on timer taubat-turn
  say intelligence "Sir, the Taubat are now hostile to us. We must strike back!"
  hostile taubat
on timer metar-stirs
  say "Incoming message" "See: the Metar sleep no more. Their eyes will turn on you!"
on timer metar-lifeforms
  say operations "Sir, many unknown life forms detected, with energy readings off the scale!"
on timer metar-outposts
  say operations "Our other outposts report the aliens coming up all over the planet and doing heavy damage."
every timer metar-raids-on-fed
  random metar-bur-protect metar-bur-fed-one metar-bur-fed-two metar-nak-fed-one metar-nak-fed-two metar-dea-fed-one metar-dea-fed-two metar-nak-fed-one
  timer metar-raids-on-fed 60
every signal metar-bur-protect
  ai metar metar-bur-protect
every signal metar-bur-fed-one
  ai metar metar-bur-fed-one
every signal metar-bur-fed-two
  ai metar metar-bur-fed-two
every signal metar-nak-fed-one
  ai metar metar-nak-fed-one
every signal metar-nak-fed-two
  ai metar metar-nak-fed-two
every signal metar-dea-fed-one
  ai metar metar-dea-fed-one
every signal metar-dea-fed-two
  ai metar metar-dea-fed-two
every timer tau-raids-on-fed
  random fed-assualt-random-one fed-assualt-random-two federation-assualt-one federation-assualt-two fed-assualt-random-one fed-assualt-random-two federation-assualt-one federation-assualt-two
  timer tau-raids-on-fed 20
every signal fed-assualt-random-one
  ai taubat fed-assualt-random-one
every signal fed-assualt-random-two
  ai taubat fed-assualt-random-two
every signal federation-assualt-one
  ai taubat federation-assualt-one
every signal federation-assualt-two
  ai taubat federation-assualt-two

# --- Hold out for the rescue: after objective 13 and the Metar's waking -----
on signal survive-step*2
  timer explorer-diverted 15
  timer explorer-in-orbit 500
  timer randall-1 510
  timer randall-2 1000
on timer explorer-diverted
  say "Message from Starfleet" "We're re-routing the USS Explorer to your planet. Withdraw to the colony and defend it until she arrives to pick you up. Good luck."
on timer explorer-in-orbit
  say communications "Sir, the USS Explorer is in orbit! A transmission is coming in now..."
on timer randall-1
  say "Message from Captain Randall" "Commander, we pushed the engines to breaking point to reach you, and it paid off. Something is jamming our transporters; we'll get you aboard the moment we can."
on timer randall-2
  say "Message from Captain Randall" "Those creatures emit a strange two-phase harmonic, but our engineer has found a fix. Prepare to beam up..."
  signal pass-step

# --- Mission pass: objectives 13 and 14 and the rescue, then 10 s ------------
on signal pass-step*3
  timer pass 10
on timer pass
  win

# --- Fails: the cause at once, the mission lost 10 s later ----------------
on count colony-hub == 0
  fail 16 10
  say intelligence "The Federation colony hub has been destroyed."
# The Metar core destroyed before we scanned it (fail 17); our scan switches this
# off.
on destroyed metar-scan-building as core-watch
  fail 17 10
  say operations "The ruined city's main building is gone. Our mission can't succeed now."

# Not expressible yet:
# - scanned-by <side> metar-scan-building never fires: the computer's sides
#   never scan (and the Romulans never capture a workshop).
# - The jnt03fed table's string 15 (scan the structures around the city) is
#   not an objective in the game's graph, so it's left out.
# - The Federation's own AI orders (colony guard, assault on the Romulans at
#   80 s, raids and scouts, an assault on the Klingons when the Romulan colony
#   is gone) only run when the side isn't played.

# --- J4 as F ----------------------------------------
# J4, played as the Federation (slot 0).
# Rewritten from the game's own mission graph (scripts_out/J4.json).
mission J4 F
site federation-colony 178 325
zone old-klingon-colony 234 186 262 206
site klingon-science-station 258 189   # group ~Kli Sci Blk, where the hostages are
site science-block 274 101             # group ~Fed Sci Blk, with the computer cores
objective 12
objective 13

# --- The other sides' attacks on the colony -------------------------------
# A Taubat strike at 10 s; three Klingon assaults at 300 s; Romulan, Taubat
# and Metar raids every 120 s from 300 s; and a Taubat strike on whichever
# empire first moves units into the old Klingon colony.
on start
  timer taubat-strike 10
  timer kli-assault-federation 300
  timer rom-raids-on-fed 300
  timer tau-raids-on-fed 300
  timer metar-raids-on-fed 300
on timer taubat-strike
  ai taubat taubat-sat-fed-colony-assault-2
on timer kli-assault-federation
  ai klingon kli-assault-federation-1
  ai klingon kli-assault-federation-2
  ai klingon kli-assault-federation-3
every timer rom-raids-on-fed
  random rom-assault-fed-random1 rom-assault-fed-random2 rom-assault-fed-random3 rom-assault-fed-random4 rom-assault-fed-random5 rom-assault-fed-random6 rom-assault-fed-random7 rom-assault-fed-random8
  timer rom-raids-on-fed 120
every signal rom-assault-fed-random1
  ai romulan rom-assault-fed-random1
every signal rom-assault-fed-random2
  ai romulan rom-assault-fed-random2
every signal rom-assault-fed-random3
  ai romulan rom-assault-fed-random3
every signal rom-assault-fed-random4
  ai romulan rom-assault-fed-random4
every signal rom-assault-fed-random5
  ai romulan rom-assault-fed-random5
every signal rom-assault-fed-random6
  ai romulan rom-assault-fed-random6
every signal rom-assault-fed-random7
  ai romulan rom-assault-fed-random7
every signal rom-assault-fed-random8
  ai romulan rom-assault-fed-random8
every timer tau-raids-on-fed
  random taubat-assault-fed-random1 taubat-assault-fed-random2 taubat-assault-fed-random3 taubat-assault-fed-random4 taubat-assault-fed-random5 taubat-assault-fed-random6 taubat-assault-fed-random7 taubat-assault-fed-random8
  timer tau-raids-on-fed 120
every signal taubat-assault-fed-random1
  ai taubat taubat-assault-fed-random1
every signal taubat-assault-fed-random2
  ai taubat taubat-assault-fed-random2
every signal taubat-assault-fed-random3
  ai taubat taubat-assault-fed-random3
every signal taubat-assault-fed-random4
  ai taubat taubat-assault-fed-random4
every signal taubat-assault-fed-random5
  ai taubat taubat-assault-fed-random5
every signal taubat-assault-fed-random6
  ai taubat taubat-assault-fed-random6
every signal taubat-assault-fed-random7
  ai taubat taubat-assault-fed-random7
every signal taubat-assault-fed-random8
  ai taubat taubat-assault-fed-random8
every timer metar-raids-on-fed
  random metar-assault-fed-random1 metar-assault-fed-random2 metar-assault-fed-random3 metar-assault-fed-random4 metar-assault-fed-random5 metar-assault-fed-random6 metar-assault-fed-random7 metar-assault-fed-random8
  timer metar-raids-on-fed 120
every signal metar-assault-fed-random1
  ai metar metar-assault-fed-random1
every signal metar-assault-fed-random2
  ai metar metar-assault-fed-random2
every signal metar-assault-fed-random3
  ai metar metar-assault-fed-random3
every signal metar-assault-fed-random4
  ai metar metar-assault-fed-random4
every signal metar-assault-fed-random5
  ai metar metar-assault-fed-random5
every signal metar-assault-fed-random6
  ai metar metar-assault-fed-random6
every signal metar-assault-fed-random7
  ai metar metar-assault-fed-random7
every signal metar-assault-fed-random8
  ai metar metar-assault-fed-random8
on inside ours old-klingon-colony
  ai taubat taubat-sat-fed-colony-assault
on inside klingon old-klingon-colony
  ai taubat taubat-sat-kli-colony-assault
on inside romulan old-klingon-colony
  ai taubat taubat-sat-rom-colony-assault

# --- Objective 12: hold the Klingon science station to find the hostages ----
# Held 2 s to start the search, 20 s more to finish it.
on entered klingon-science-station
  say intelligence "We're inside, sir. Searching the building."
on capture-lost klingon-science-station
  say intelligence "The building has been abandoned before the search was finished."
on captured klingon-science-station
  say intelligence "We have the hostages, sir. Beaming them out now."
  complete 12
  timer objective-1-notice 5
on timer objective-1-notice
  say command "Primary objective 1 achieved."
# (Fail 15: a rival taking the station during our search; nothing in the
# mission ever sends that, so it can't happen.)
fail 15 beaten klingon-science-station
  say intelligence "The hostages are dead. The mission has failed."
  signal failing

# --- Objective 13: recover the computer cores from our science block ---------
# Held 1 s to start, 10 s more to finish.
on entered science-block
  say intelligence "We're inside, sir. Unplugging the computer core now."
on capture-lost science-block
  say intelligence "The building has been abandoned before the core was out."
on captured science-block
  say intelligence "The core is unplugged, sir. Beaming it up now."
  complete 13
  timer objective-2-notice 5
on timer objective-2-notice
  say command "Primary objective 2 achieved."
# (Fail 16: likewise never sent in the game.)
fail 16 beaten science-block
  say intelligence "The computer core is lost. The mission has failed."
  signal failing

)",
R"(# --- Mission pass: both objectives, a line at 10 s, the win at 20 s --------
on all-completed
  timer pass-notice 10
  timer pass 20
on timer pass-notice
  say intelligence "Mission accomplished for the Federation."
on timer pass
  win

# --- Fails: the cause at once, a line at 10 s, the mission lost at 20 s ----
fail 14 count colony-hub == 0
  say intelligence "The Federation colony hub has been destroyed."
  signal failing
on signal failing
  timer fail-notice 10
on timer fail-notice
  say intelligence "The Federation is unable to finish its mission."

# Not expressible yet:
# - Both rescues' "beaten" outcomes carry fails 15 and 16, but no object sends
#   the rivals' claim (the J4 raids don't hand off), so the game never fails
#   them; written on the buildings' names, not the raid labels (kli-sci-blk,
#   fed-sci-blk), so Reluva's hand-off can't fire them either.
# - The Federation's own AI orders (colony guard, refugee APC, random raids on
#   the Romulans from 360 s) only run when the side isn't played.

# --- F8 as F ----------------------------------------
# F8 as the Federation, rewritten from the game's own graph (STNW.exe dump F8.json, slot 0,
# with the Hubrin (4) and Metar (5) objects that act against the Federation).
mission F8 F
zone romulan-pad 424 352 432 356
zone tractor-site 301 339 317 356
zone colony 416 272 440 296
site hubrin-drop-west 358 337
site hubrin-drop-east 362 353
objective 13
objective 15

# --- Hubrin and Metar raids: from 240 s, every 120 s each picks one of eight at random -------
on start
  timer hubrin-guard 1
  timer hubrin-raids 240
  timer metar-raids 240
on timer hubrin-guard
  ai hubrin hbr-colony-protection-group-1
  ai hubrin hbr-colony-protection-group-2
every timer hubrin-raids
  random hbr-raid-1 hbr-raid-2 hbr-raid-3 hbr-raid-4 hbr-raid-5 hbr-raid-6 hbr-raid-7 hbr-raid-8
  timer hubrin-raids 120
# each Hubrin raid's order beams its two or three units in at one of the two drop points
every signal hbr-raid-1
  ai hubrin hubrin-assault-fed-random1
every signal hbr-raid-2
  ai hubrin hubrin-assault-fed-random2
every signal hbr-raid-3
  ai hubrin hubrin-assault-fed-random3
every signal hbr-raid-4
  ai hubrin hubrin-assault-fed-random4
every signal hbr-raid-5
  ai hubrin hubrin-assault-fed-random5
every signal hbr-raid-6
  ai hubrin hubrin-assault-fed-random6
every signal hbr-raid-7
  ai hubrin hubrin-assault-fed-random7
every signal hbr-raid-8
  ai hubrin hubrin-assault-fed-random8
every timer metar-raids
  random met-raid-1 met-raid-2 met-raid-3 met-raid-4 met-raid-5 met-raid-6 met-raid-7 met-raid-8
  timer metar-raids 120
every signal met-raid-1
  ai metar met-assault-fed-random1
every signal met-raid-2
  ai metar met-assault-fed-random2
every signal met-raid-3
  ai metar met-assault-fed-random3
every signal met-raid-4
  ai metar met-assault-fed-random4
every signal met-raid-5
  ai metar met-assault-fed-random5
every signal met-raid-6
  ai metar met-assault-fed-random6
every signal met-raid-7
  ai metar met-assault-fed-random7
every signal met-raid-8
  ai metar met-assault-fed-random8

# --- The Romulan trade delegation: our units reach their pad --------------------------------
on inside ours romulan-pad
  spawn us cloaked-tank 3 421 349
  say command "The Romulans have beamed down cloaked tanks and handed them over to us."

# --- Objective 13: escort the tractor home ----------------------------------------------------
# fighting units reach the tractor: it sets off, and the Hubrin and Metar sentries stand down
on inside ours tractor-site
  say command "The tractor is on its way, with its escort."
  ai federation tractor-controller
  arm tractor-route
  ai metar abort met-sentry-lob-1 met-sentry-lob-2 met-sentry-lob-3 met-sentry-lob-4 met-sentry-lob-5 met-sentry-lob-6 met-sentry-lob-7 met-sentry-lob-8
  ai hubrin abort hbr-sentry-lob-1 hbr-sentry-lob-2 hbr-sentry-lob-3 hbr-sentry-lob-4 hbr-sentry-lob-5 hbr-sentry-lob-6 hbr-sentry-lob-7 hbr-sentry-lob-8
# the tractor leaves the site: the natives go for the colony (both watches sleep until the escort comes)
on outside escort-tractor tractor-site as tractor-route dormant
  say command "Odd movements among the Hubrin and Metar units."
  ai metar met-assault-fed-colony-lob-1
  ai metar met-assault-fed-colony-lob-2
  ai metar met-assault-fed-colony-lob-3
  ai hubrin hbr-assault-fed-colony-lob-1
  ai hubrin hbr-assault-fed-colony-lob-2
  ai hubrin hbr-assault-fed-colony-lob-3
  timer route-advice 20
  timer assault-warning 90
on timer route-advice
  say command "The tractor has been told to keep clear of the Hubrin base."
on timer assault-warning
  say command "There are reports that hostile units may be closing on the colony."
on inside escort-tractor colony as tractor-route dormant
  say command "The tractor is inside the colony. Preparing to beam it up."
  timer beam-up 50
on timer beam-up
  say command "Getting a transporter lock on it now."
  disarm tractor-fail
  beam-up escort-tractor
# the tractor's group gone from the map: objective 13, the report 1 s later
on beamed-up escort-tractor
  complete 13
  signal objective
  research dormant-metar-research
  timer recovered 1
on timer recovered
  say command "The dormant Metar has been recovered."

# --- Objective 15: research the dormant Metar --------------------------------------------------
on researched dormant-metar-research
  timer metar-report-1 15
  timer metar-report-2 20
on timer metar-report-1
  say science "The research on the dormant Metar is finished. The data shows a biological flux."
on timer metar-report-2
  say science "This creature seems to live in two states: a physical body here, and pure energy in subspace. That could account for its enormous strength."
  complete 15
  signal objective

# --- The pass: two objectives, a message at 15 s, the win at 20 s ------------------------------
on signal objective*2
  timer pass-message 15
  timer pass 20
on timer pass-message
  say command "The primary objectives are complete. Well done."
on timer pass
  win

# --- Fails: the cause at once, "failed" at 10 s, the end at 20 s ------------------------------
# the tractor is watched until the transporter locks on
on destroyed escort-tractor as tractor-fail
  fail 14 20
  say command "The dormant Metar leader has been destroyed. The mission has failed."
  signal failing
on destroyed colony-hub
  fail 16 20
  say command "Our colony hub is destroyed. The mission has failed."
  signal failing
on signal failing
  timer fail-message 10
on timer fail-message
  say command "We have not met our objectives."

# Not expressible yet:
# - The escort detector at the tractor site counts only fighting units (unit class 256);
#   written as any of ours.
# - The Romulan slot holds only a dummy object.

# --- J6 as F ----------------------------------------
# J6 as the Federation (slot 0), from the game's own mission graph.
# Take the Romulans' anomaly device (scan their hub, then bring down their
# shields so it can be beamed out), and study the Metar's biodata.
mission J6 F
site romulan-colony 194 181
site metar-colony 386 317
objective 13
objective 15
objective 17

# Fail criteria, the game's way: the fail string at once, word to every
# side at 10 s that the Federation can't complete its mission, the end at
# 20 s. Our hub (19); the Romulan hub lost before our strong scan of it
# is done (14: the scan disarms this watch, see objective 13).
on destroyed colony-hub
  fail 19 20
  say intelligence "Our colony hub has been destroyed."
  signal failing
on hub-destroyed romulan as romulan-hub-watch
  fail 14 20
  signal failing
on signal failing
  timer fail-news 10
on timer fail-news
  say intelligence "The Federation's mission can no longer succeed."

# Start: the other sides' attack clocks
on start
  timer kli-raid-fed 240
  timer kli-raid-rom 180
  timer rom-raid-fed 180
  timer rom-raid-kli 180
  timer met-raid-fed 480
  timer met-raid-kli 480
  timer met-raid-rom 480
  timer met-assault-klingons-1 1
  timer met-assault-romulans-1 1
  timer met-assault-romulans-2 900
  timer met-assault-federation-1 1
  timer met-assault-federation-2 1200

# Arrivals: a Romulan greeting at their colony, a warning near the Metar
on seen romulan-colony
  say "Incoming message" "Welcome, friend - visitors are always a pleasure. Purely routine, but please present yourselves to security."
on seen metar-colony
  say intelligence "Sir, I'd advise the research team to tread very carefully here."

# Objective 13: a strong engineering scan of the Romulan hub, held 10 s;
# done, it stops the fail-14 watch on their hub
every scan-started strong-engineering romulan-hub
  say engineering "Starting the survey. Now, where would they hide it?"
every scan-lost strong-engineering romulan-hub
  say engineering "The scan dropped out. Awkward one - give me more time."
on scanned strong-engineering romulan-hub
  say engineering "It's inside the Romulans' hub, and their shields will keep us from beaming it out."
  complete 13
  disarm romulan-hub-watch
# An ordinary engineering scan of the hub only finds that a specialist is needed
every scan-started engineering romulan-hub
  say engineering "Scanners on, Sir."
every scan-lost engineering romulan-hub
  say engineering "Signal gone, Sir. Another attempt coming."
on scanned engineering romulan-hub
  say engineering "Survey done, but I can't see through their screens. Can we get Ruskin over here?"

# Objective 15: every Romulan shield generator destroyed; the device is
# beamed out and the objective met 15 s later
on count romulan-shield-generator == 0
  timer rom-shields-failing 5
  timer device-beamed 15
on timer rom-shields-failing
  say engineering "Their shields are failing. Trying for a transporter lock."
on timer device-beamed
  complete 15

# Objective 17: a strong science scan of the Metar core (1 s to start, 10 s
# held) opens the biodata research; 15 s after it's done, the objective
every scan-started strong-science metar-core
  say science "Scanning now, with a few tricks to clean up the signal."
every scan-lost strong-science metar-core
  say science "It slipped away. I need more time on it."
on scanned strong-science metar-core
  say science "Finished... goodness. The biodata keeps shifting - I'd say the Metar are changing as a species. The medical researchers should see this."
  research metar-biodata
every scan-started science metar-core
  say science "Scan started, Sir."
every scan-lost science metar-core
  say science "The scan dropped, Sir. Trying again."
on scanned science metar-core
  say science "Finished, Sir, but I can't draw any firm conclusion. This needs a specialist."
# The game also offers the research when a sickbay ward (type 300) is built
on built sickbay-ward
  research metar-biodata
on researched metar-biodata
  timer biodata-ready 5
  timer biodata-findings 10
  timer biodata-done 15
on timer biodata-ready
  say medical "The Metar biodata is ready, and it's disturbing."
on timer biodata-findings
  say medical "The change affects every Metar caste. The next ones to hatch will be stronger."
on timer biodata-done
  complete 17

# The Klingons' side of it: with our shields all down they beam the
# Ambassador out, 15 s later
on count shield-generator == 0
  timer ambassador-taken 15
on timer ambassador-taken
  say command "We've lost the Ambassador - the Klingons transported him out from under us!"

# Scans of anything else
every scanned-nothing science
  say science "Standard science scans: nothing out of the ordinary, Sir."
every scanned-nothing strong-science
  say science "Looked closely, Sir - everything is as it should be."
every scanned-nothing medical
  say medical "The routine medical scan shows nothing amiss, Sir."
every scanned-nothing strong-medical
  say medical "A complete medical workup finds nothing of note, Sir."
every scanned-nothing engineering
  say engineering "Every structure and system reads normal, Sir."
every scanned-nothing strong-engineering
  say engineering "No engineering surprises here, Sir."

# Mission passed (all three objectives): the news at 10 s, the win at 20 s
on all-completed
  timer pass-news 10
  timer pass 20
on timer pass-news
  say intelligence "The Federation's mission is accomplished."
on timer pass
  win

# The other sides' AI: attacks on a clock (the Federation's own attack
# orders are left out: the player gives those)
# Klingon raids on the Federation: one of eight attacks every 60 s, the first at 240 s
every timer kli-raid-fed
  timer kli-raid-fed 60
  random kli-raid-fed-1 kli-raid-fed-2 kli-raid-fed-3 kli-raid-fed-4 kli-raid-fed-5 kli-raid-fed-6 kli-raid-fed-7 kli-raid-fed-8
every signal kli-raid-fed-1
  ai klingon kli-assault-fed-random1
every signal kli-raid-fed-2
  ai klingon kli-assault-fed-random2
every signal kli-raid-fed-3
  ai klingon kli-assault-fed-random3
every signal kli-raid-fed-4
  ai klingon kli-assault-fed-random4
every signal kli-raid-fed-5
  ai klingon kli-assault-fed-random5
every signal kli-raid-fed-6
  ai klingon kli-assault-fed-random6
every signal kli-raid-fed-7
  ai klingon kli-assault-fed-random7
every signal kli-raid-fed-8
  ai klingon kli-assault-fed-random8

# Klingon raids on the Romulans: one of eight attacks every 60 s, the first at 180 s
every timer kli-raid-rom
  timer kli-raid-rom 60
  random kli-raid-rom-1 kli-raid-rom-2 kli-raid-rom-3 kli-raid-rom-4 kli-raid-rom-5 kli-raid-rom-6 kli-raid-rom-7 kli-raid-rom-8
every signal kli-raid-rom-1
  ai klingon kli-assault-rom-random1
every signal kli-raid-rom-2
  ai klingon kli-assault-rom-random2
every signal kli-raid-rom-3
  ai klingon kli-assault-rom-random3
every signal kli-raid-rom-4
  ai klingon kli-assault-rom-random4
every signal kli-raid-rom-5
  ai klingon kli-assault-rom-random5
every signal kli-raid-rom-6
  ai klingon kli-assault-rom-random6
every signal kli-raid-rom-7
  ai klingon kli-assault-rom-random7
every signal kli-raid-rom-8
  ai klingon kli-assault-rom-random8

# Romulan raids on the Federation: one of eight attacks every 60 s, the first at 180 s
every timer rom-raid-fed
  timer rom-raid-fed 60
  random rom-raid-fed-1 rom-raid-fed-2 rom-raid-fed-3 rom-raid-fed-4 rom-raid-fed-5 rom-raid-fed-6 rom-raid-fed-7 rom-raid-fed-8
every signal rom-raid-fed-1
  ai romulan rom-assault-fed-random1
every signal rom-raid-fed-2
  ai romulan rom-assault-fed-random2
every signal rom-raid-fed-3
  ai romulan rom-assault-fed-random3
every signal rom-raid-fed-4
  ai romulan rom-assault-fed-random4
every signal rom-raid-fed-5
  ai romulan rom-assault-fed-random5
every signal rom-raid-fed-6
  ai romulan rom-assault-fed-random6
every signal rom-raid-fed-7
  ai romulan rom-assault-fed-random7
every signal rom-raid-fed-8
  ai romulan rom-assault-fed-random8

# Romulan raids on the Klingons: one of eight attacks every 60 s, the first at 180 s
every timer rom-raid-kli
  timer rom-raid-kli 60
  random rom-raid-kli-1 rom-raid-kli-2 rom-raid-kli-3 rom-raid-kli-4 rom-raid-kli-5 rom-raid-kli-6 rom-raid-kli-7 rom-raid-kli-8
)",
R"(every signal rom-raid-kli-1
  ai romulan rom-assault-kli-random1
every signal rom-raid-kli-2
  ai romulan rom-assault-kli-random2
every signal rom-raid-kli-3
  ai romulan rom-assault-kli-random3
every signal rom-raid-kli-4
  ai romulan rom-assault-kli-random4
every signal rom-raid-kli-5
  ai romulan rom-assault-kli-random5
every signal rom-raid-kli-6
  ai romulan rom-assault-kli-random6
every signal rom-raid-kli-7
  ai romulan rom-assault-kli-random7
every signal rom-raid-kli-8
  ai romulan rom-assault-kli-random8

# Metar raids on the Federation: one of eight attacks every 120 s, the first at 480 s
every timer met-raid-fed
  timer met-raid-fed 120
  random met-raid-fed-1 met-raid-fed-2 met-raid-fed-3 met-raid-fed-4 met-raid-fed-5 met-raid-fed-6 met-raid-fed-7 met-raid-fed-8
every signal met-raid-fed-1
  ai metar metar-assault-fed-random1
every signal met-raid-fed-2
  ai metar metar-assault-fed-random2
every signal met-raid-fed-3
  ai metar metar-assault-fed-random3
every signal met-raid-fed-4
  ai metar metar-assault-fed-random4
every signal met-raid-fed-5
  ai metar metar-assault-fed-random5
every signal met-raid-fed-6
  ai metar metar-assault-fed-random6
every signal met-raid-fed-7
  ai metar metar-assault-fed-random7
every signal met-raid-fed-8
  ai metar metar-assault-fed-random8

# Metar raids on the Klingons: one of eight attacks every 120 s, the first at 480 s
every timer met-raid-kli
  timer met-raid-kli 120
  random met-raid-kli-1 met-raid-kli-2 met-raid-kli-3 met-raid-kli-4 met-raid-kli-5 met-raid-kli-6 met-raid-kli-7 met-raid-kli-8
every signal met-raid-kli-1
  ai metar metar-assault-kli-random1
every signal met-raid-kli-2
  ai metar metar-assault-kli-random2
every signal met-raid-kli-3
  ai metar metar-assault-kli-random3
every signal met-raid-kli-4
  ai metar metar-assault-kli-random4
every signal met-raid-kli-5
  ai metar metar-assault-kli-random5
every signal met-raid-kli-6
  ai metar metar-assault-kli-random6
every signal met-raid-kli-7
  ai metar metar-assault-kli-random7
every signal met-raid-kli-8
  ai metar metar-assault-kli-random8

# Metar raids on the Romulans: one of eight attacks every 120 s, the first at 480 s
every timer met-raid-rom
  timer met-raid-rom 120
  random met-raid-rom-1 met-raid-rom-2 met-raid-rom-3 met-raid-rom-4 met-raid-rom-5 met-raid-rom-6 met-raid-rom-7 met-raid-rom-8
every signal met-raid-rom-1
  ai metar metar-assault-rom-random1
every signal met-raid-rom-2
  ai metar metar-assault-rom-random2
every signal met-raid-rom-3
  ai metar metar-assault-rom-random3
every signal met-raid-rom-4
  ai metar metar-assault-rom-random4
every signal met-raid-rom-5
  ai metar metar-assault-rom-random5
every signal met-raid-rom-6
  ai metar metar-assault-rom-random6
every signal met-raid-rom-7
  ai metar metar-assault-rom-random7
every signal met-raid-rom-8
  ai metar metar-assault-rom-random8

# Metar orders on a clock
on timer met-assault-klingons-1
  ai metar met-assault-klingons-1
on timer met-assault-romulans-1
  ai metar met-assault-romulans-1
on timer met-assault-romulans-2
  ai metar met-assault-romulans-2
on timer met-assault-federation-1
  ai metar met-assault-federation-1
on timer met-assault-federation-2
  ai metar met-assault-federation-2

# The Klingons' shields all down: 15 s later the device takes out their hub.
on count klingon-shield-generator == 0
  timer klingon-hub-falls 15
on timer klingon-hub-falls
  damage colony-hub all klingon

# Not expressible yet:
# - The game shows nothing for "Romulan shields down": the line it would send,
#   "device transported", and the Romulans' "the Federation took the device"
#   go to a misspelt target and never appear (left out here).
# - The scan "futile" lines (a repeat success after a rival's) are never
#   reachable in J6: nothing sends the raids Go_Futile.
# - Messages caused by the other sides: their mission passed/failed
#   announcements (sent to every side), and the Klingon science officer's
#   "scanning now" for an ordinary Metar scan, which the game misroutes to the
#   Federation's mail. There's no event for a rival's pass; a rival's scan
#   (scanned-by) can't happen: the computer's sides never scan.
# - Whether the biodata research needs both the scan and the ward, or either:
#   the game sends the topic from both; here either opens it.
# - The AI sides stop their build orders when their hub falls (Abort to the
#   named build orders), and build on their own timetable (npc build orders).

# --- J7 as F ----------------------------------------
# J7 as the Federation (slot 0), from the game's own mission graph.
# Stock up dilithium and transal for the Hubrin (the game takes a share of
# each), and keep our Hubrin guide alive; both objectives are met together.
mission J7 F
objective 13
objective 15

# Fail criteria, the game's way: the cause at once, word to every side at
# 5 s that the Federation can't complete its mission, the end at 10 s.
# Our hub (14); our Hubrin guide killed (16).
on destroyed fed-hubrin
  fail 16 10
  say intelligence "Our Hubrin guide is dead."
  signal failing
on destroyed colony-hub
  fail 14 10
  say intelligence "Our colony hub has been destroyed."
  signal failing
on signal failing
  timer fail-news 5
on timer fail-news
  say intelligence "The Federation's mission can no longer succeed."

# Start: the other sides' attack clocks
on start
  timer kli-raid-fed 180
  timer kli-raid-rom 180
  timer rom-raid-fed 180
  timer rom-raid-kli 180
  timer tau-raid-fed 180
  timer tau-raid-kli 180
  timer tau-raid-rom 180
  timer met-raid-fed 300
  timer met-raid-kli 300
  timer met-raid-rom 300
  timer met-assault-federation 1800
  timer met-assault-klingon 2100
  timer met-assault-romulan 2400

# Objectives 13 and 15: 250 of dilithium in stock (100 is taken), 250 of
# transal in stock (50 is taken); with both delivered the guide's job is done
on stock dilithium 250
  take dilithium 100
  say engineering "The dilithium we need is ready, Sir."
  signal quota
on stock transalumin 250
  take transalumin 50
  say engineering "Sir, the transal we need is ready."
  signal quota
on signal quota*2
  complete 13
  complete 15

# Mission passed: the news at once, the win 10 s later
on all-completed
  say intelligence "The Federation's mission is accomplished."
  timer pass 10
on timer pass
  win

# A colony grown to four times its starting size draws two Metar attacks
# on it: ours, and the other sides' too
on colony-size 400
  ai metar met-assault-leading-player-fed-a
  ai metar met-assault-leading-player-fed-b
on colony-size klingon 400
  ai metar met-assault-leading-player-kli-a
  ai metar met-assault-leading-player-kli-b
on colony-size romulan 400
  ai metar met-assault-leading-player-rom-a
  ai metar met-assault-leading-player-rom-b

# The other sides' AI: attacks on a clock (the Federation's own attack orders
# are left out: the player gives those)
# Klingon raids on the Federation: one of eight attacks every 60 s, the first at 180 s
every timer kli-raid-fed
  timer kli-raid-fed 60
  random kli-raid-fed-1 kli-raid-fed-2 kli-raid-fed-3 kli-raid-fed-4 kli-raid-fed-5 kli-raid-fed-6 kli-raid-fed-7 kli-raid-fed-8
every signal kli-raid-fed-1
  ai klingon kli-assault-fed-random1
every signal kli-raid-fed-2
  ai klingon kli-assault-fed-random2
every signal kli-raid-fed-3
  ai klingon kli-assault-fed-random3
every signal kli-raid-fed-4
  ai klingon kli-assault-fed-random4
every signal kli-raid-fed-5
  ai klingon kli-assault-fed-random5
every signal kli-raid-fed-6
  ai klingon kli-assault-fed-random6
every signal kli-raid-fed-7
  ai klingon kli-assault-fed-random7
every signal kli-raid-fed-8
  ai klingon kli-assault-fed-random8

# Klingon raids on the Romulans: one of eight attacks every 60 s, the first at 180 s
every timer kli-raid-rom
  timer kli-raid-rom 60
  random kli-raid-rom-1 kli-raid-rom-2 kli-raid-rom-3 kli-raid-rom-4 kli-raid-rom-5 kli-raid-rom-6 kli-raid-rom-7 kli-raid-rom-8
every signal kli-raid-rom-1
  ai klingon kli-assault-rom-random1
every signal kli-raid-rom-2
  ai klingon kli-assault-rom-random2
every signal kli-raid-rom-3
  ai klingon kli-assault-rom-random3
every signal kli-raid-rom-4
  ai klingon kli-assault-rom-random4
every signal kli-raid-rom-5
  ai klingon kli-assault-rom-random5
every signal kli-raid-rom-6
  ai klingon kli-assault-rom-random6
every signal kli-raid-rom-7
  ai klingon kli-assault-rom-random7
every signal kli-raid-rom-8
  ai klingon kli-assault-rom-random8

# Romulan raids on the Federation: one of eight attacks every 60 s, the first at 180 s
every timer rom-raid-fed
  timer rom-raid-fed 60
  random rom-raid-fed-1 rom-raid-fed-2 rom-raid-fed-3 rom-raid-fed-4 rom-raid-fed-5 rom-raid-fed-6 rom-raid-fed-7 rom-raid-fed-8
every signal rom-raid-fed-1
  ai romulan rom-assault-fed-random1
every signal rom-raid-fed-2
  ai romulan rom-assault-fed-random2
every signal rom-raid-fed-3
  ai romulan rom-assault-fed-random3
every signal rom-raid-fed-4
  ai romulan rom-assault-fed-random4
every signal rom-raid-fed-5
  ai romulan rom-assault-fed-random5
every signal rom-raid-fed-6
  ai romulan rom-assault-fed-random6
every signal rom-raid-fed-7
  ai romulan rom-assault-fed-random7
every signal rom-raid-fed-8
  ai romulan rom-assault-fed-random8

# Romulan raids on the Klingons: one of eight attacks every 60 s, the first at 180 s
every timer rom-raid-kli
  timer rom-raid-kli 60
  random rom-raid-kli-1 rom-raid-kli-2 rom-raid-kli-3 rom-raid-kli-4 rom-raid-kli-5 rom-raid-kli-6 rom-raid-kli-7 rom-raid-kli-8
every signal rom-raid-kli-1
  ai romulan rom-assault-kli-random1
every signal rom-raid-kli-2
  ai romulan rom-assault-kli-random2
every signal rom-raid-kli-3
  ai romulan rom-assault-kli-random3
every signal rom-raid-kli-4
  ai romulan rom-assault-kli-random4
every signal rom-raid-kli-5
  ai romulan rom-assault-kli-random5
every signal rom-raid-kli-6
  ai romulan rom-assault-kli-random6
every signal rom-raid-kli-7
  ai romulan rom-assault-kli-random7
every signal rom-raid-kli-8
  ai romulan rom-assault-kli-random8

# Taubat raids on the Federation: one of eight attacks every 60 s, the first at 180 s
every timer tau-raid-fed
  timer tau-raid-fed 60
  random tau-raid-fed-1 tau-raid-fed-2 tau-raid-fed-3 tau-raid-fed-4 tau-raid-fed-5 tau-raid-fed-6 tau-raid-fed-7 tau-raid-fed-8
every signal tau-raid-fed-1
  ai taubat taubat-assault-fed-random1
every signal tau-raid-fed-2
  ai taubat taubat-assault-fed-random2
every signal tau-raid-fed-3
  ai taubat taubat-assault-fed-random3
every signal tau-raid-fed-4
  ai taubat taubat-assault-fed-random4
every signal tau-raid-fed-5
  ai taubat taubat-assault-fed-random5
every signal tau-raid-fed-6
  ai taubat taubat-assault-fed-random6
every signal tau-raid-fed-7
  ai taubat taubat-assault-fed-random7
every signal tau-raid-fed-8
  ai taubat taubat-assault-fed-random8

# Taubat raids on the Klingons: one of eight attacks every 60 s, the first at 180 s
every timer tau-raid-kli
  timer tau-raid-kli 60
  random tau-raid-kli-1 tau-raid-kli-2 tau-raid-kli-3 tau-raid-kli-4 tau-raid-kli-5 tau-raid-kli-6 tau-raid-kli-7 tau-raid-kli-8
every signal tau-raid-kli-1
  ai taubat taubat-assault-kli-random1
every signal tau-raid-kli-2
  ai taubat taubat-assault-kli-random2
every signal tau-raid-kli-3
  ai taubat taubat-assault-kli-random3
every signal tau-raid-kli-4
  ai taubat taubat-assault-kli-random4
every signal tau-raid-kli-5
  ai taubat taubat-assault-kli-random5
every signal tau-raid-kli-6
  ai taubat taubat-assault-kli-random6
every signal tau-raid-kli-7
  ai taubat taubat-assault-kli-random7
every signal tau-raid-kli-8
  ai taubat taubat-assault-kli-random8

# Taubat raids on the Romulans: one of eight attacks every 60 s, the first at 180 s
every timer tau-raid-rom
  timer tau-raid-rom 60
  random tau-raid-rom-1 tau-raid-rom-2 tau-raid-rom-3 tau-raid-rom-4 tau-raid-rom-5 tau-raid-rom-6 tau-raid-rom-7 tau-raid-rom-8
every signal tau-raid-rom-1
  ai taubat taubat-assault-rom-random1
every signal tau-raid-rom-2
  ai taubat taubat-assault-rom-random2
every signal tau-raid-rom-3
  ai taubat taubat-assault-rom-random3
every signal tau-raid-rom-4
  ai taubat taubat-assault-rom-random4
every signal tau-raid-rom-5
  ai taubat taubat-assault-rom-random5
every signal tau-raid-rom-6
  ai taubat taubat-assault-rom-random6
every signal tau-raid-rom-7
  ai taubat taubat-assault-rom-random7
every signal tau-raid-rom-8
  ai taubat taubat-assault-rom-random8

# Metar raids on the Federation: one of eight attacks every 60 s, the first at 300 s
every timer met-raid-fed
  timer met-raid-fed 60
  random met-raid-fed-1 met-raid-fed-2 met-raid-fed-3 met-raid-fed-4 met-raid-fed-5 met-raid-fed-6 met-raid-fed-7 met-raid-fed-8
every signal met-raid-fed-1
  ai metar metar-assault-fed-random1
every signal met-raid-fed-2
  ai metar metar-assault-fed-random2
every signal met-raid-fed-3
  ai metar metar-assault-fed-random3
every signal met-raid-fed-4
  ai metar metar-assault-fed-random4
every signal met-raid-fed-5
  ai metar metar-assault-fed-random5
every signal met-raid-fed-6
  ai metar metar-assault-fed-random6
every signal met-raid-fed-7
  ai metar metar-assault-fed-random7
every signal met-raid-fed-8
  ai metar metar-assault-fed-random8

# Metar raids on the Klingons: one of eight attacks every 60 s, the first at 300 s
every timer met-raid-kli
  timer met-raid-kli 60
  random met-raid-kli-1 met-raid-kli-2 met-raid-kli-3 met-raid-kli-4 met-raid-kli-5 met-raid-kli-6 met-raid-kli-7 met-raid-kli-8
every signal met-raid-kli-1
  ai metar metar-assault-kli-random1
every signal met-raid-kli-2
  ai metar metar-assault-kli-random2
every signal met-raid-kli-3
  ai metar metar-assault-kli-random3
every signal met-raid-kli-4
  ai metar metar-assault-kli-random4
every signal met-raid-kli-5
  ai metar metar-assault-kli-random5
every signal met-raid-kli-6
  ai metar metar-assault-kli-random6
every signal met-raid-kli-7
  ai metar metar-assault-kli-random7
every signal met-raid-kli-8
  ai metar metar-assault-kli-random8

# Metar raids on the Romulans: one of eight attacks every 60 s, the first at 300 s
every timer met-raid-rom
  timer met-raid-rom 60
  random met-raid-rom-1 met-raid-rom-2 met-raid-rom-3 met-raid-rom-4 met-raid-rom-5 met-raid-rom-6 met-raid-rom-7 met-raid-rom-8
every signal met-raid-rom-1
  ai metar metar-assault-rom-random1
every signal met-raid-rom-2
  ai metar metar-assault-rom-random2
every signal met-raid-rom-3
  ai metar metar-assault-rom-random3
every signal met-raid-rom-4
  ai metar metar-assault-rom-random4
every signal met-raid-rom-5
  ai metar metar-assault-rom-random5
every signal met-raid-rom-6
  ai metar metar-assault-rom-random6
every signal met-raid-rom-7
  ai metar metar-assault-rom-random7
every signal met-raid-rom-8
  ai metar metar-assault-rom-random8

# Metar orders on a clock
on timer met-assault-federation
  ai metar met-assault-federation
on timer met-assault-klingon
  ai metar met-assault-klingon
on timer met-assault-romulan
  ai metar met-assault-romulan

)",
R"(# Not expressible yet:
# - The Hubrin guides walk to each colony on their own orders (unit orders of
#   the Hubrin side, not started by the script), and the AI sides build on
#   their own timetable and stop when their hub falls.
# - Messages caused by the other sides: their mission passed/failed
#   announcements (sent to every side). There's no event for a rival's pass.

# --- F12 as F ----------------------------------------
# F12 as the Federation, rewritten from the game's own graph (STNW.exe dump F12.json, slot 0,
# with the Taubat (3), Hubrin (4) and Metar (5) objects that act on the Federation).
mission F12 F
site cfd-drop 152 239
site hubrin-saboteur 154 241
site cfd-fault 154 237
objective 13
objective 15

# --- Taubat and Metar raids: from 120 s, every 120 s each picks one of eight at random,
# --- and three set Metar assaults at 120, 180 and 300 s ----------------------------------
on start
  timer tau-raids 120
  timer met-raids 120
  timer met-assault-1 120
  timer met-assault-2 180
  timer met-assault-3 300
every timer tau-raids
  random tau-raid-1 tau-raid-2 tau-raid-3 tau-raid-4 tau-raid-5 tau-raid-6 tau-raid-7 tau-raid-8
  timer tau-raids 120
every signal tau-raid-1
  ai taubat tau-assault-fed-random1
every signal tau-raid-2
  ai taubat tau-assault-fed-random2
every signal tau-raid-3
  ai taubat tau-assault-fed-random3
every signal tau-raid-4
  ai taubat tau-assault-fed-random4
every signal tau-raid-5
  ai taubat tau-assault-fed-random5
every signal tau-raid-6
  ai taubat tau-assault-fed-random6
every signal tau-raid-7
  ai taubat tau-assault-fed-random7
every signal tau-raid-8
  ai taubat tau-assault-fed-random8
every timer met-raids
  random met-raid-1 met-raid-2 met-raid-3 met-raid-4 met-raid-5 met-raid-6 met-raid-7 met-raid-8
  timer met-raids 120
every signal met-raid-1
  ai metar met-assault-fed-random1
every signal met-raid-2
  ai metar met-assault-fed-random2
every signal met-raid-3
  ai metar met-assault-fed-random3
every signal met-raid-4
  ai metar met-assault-fed-random4
every signal met-raid-5
  ai metar met-assault-fed-random5
every signal met-raid-6
  ai metar met-assault-fed-random6
every signal met-raid-7
  ai metar met-assault-fed-random7
every signal met-raid-8
  ai metar met-assault-fed-random8
on timer met-assault-1
  ai metar met-assault-federation-1
on timer met-assault-2
  ai metar met-assault-federation-2
on timer met-assault-3
  ai metar met-assault-federation-3

# --- Objective 13: the colony tripled, with power and farms ----------------------------------
on colony-size 300
  signal infrastructure
# five power generators or three advanced ones, and six farms
on built-all power-generator*5|advanced-generator*3 farm*6
  signal infrastructure
on signal infrastructure*2
  say engineering "Sir, the colony can now support building the containment device. It needs a final research check."
  complete 13
  signal objective
  research cfd-research

# --- The containment field device: made by the game, set down south, attacked --------------------
on researched cfd-research
  timer cfd-built 10
on timer cfd-built
  spawn us containment-device 1 152 239
  say engineering "The device is built. Carrying it to the spot the Hubrin marked, to the south."
# the build-and-hold watch: the device in place, its 600 s begin
on standing containment-device 1
  say engineering "Sir, transport is complete. The device for the field's containment is now in position."
  ai metar metar-attack-on-cfd
  spawn hubrin hubrin 1 154 241
  timer sabotage-1 15
on timer sabotage-1
  say intelligence "Sir, a second Hubrin has turned up by the device. It says a building error has unbalanced the field."
  timer sabotage-2 15
on timer sabotage-2
  say engineering "A remote scan bears that out. A science vehicle will have to get close to pin down the imbalance."
  timer fault-scan-ready 15
on timer fault-scan-ready
  arm fault-scan

# --- Objective 15: find the fault, and keep the device for 10 minutes ---------------------------
# the fault scan sleeps until 15 s after the second sabotage line (45 s after the device stands)
on scanned science cfd-fault as fault-scan dormant
  say science "Trying to isolate the field imbalance. Stand by..."
  timer recalibrate 20
on timer recalibrate
  say engineering "I've isolated the fault. Now recalibrating the device for the containment field..."
  signal cfd-online
on standing containment-device 600
  signal cfd-online
on signal cfd-online*2
  say engineering "That was close, Command. As first set up, it would soon have overloaded our whole grid."
  complete 15
  signal objective

# --- The Hubrin observer ---------------------------------------------------------------------
on count hubrin-observer == 0
  say intelligence "The Hubrin observer is dead."

# --- The pass: both objectives, a message at once, the win at 10 s -----------------------------
on signal objective*2
  say command "All objectives met. Well done."
  timer pass 10
on timer pass
  win

# --- Fails: the cause at once, "failed" at 10 s, the end at 20 s ------------------------------
on destroyed colony-hub
  fail 14 20
  say intelligence "The Federation hub has been destroyed."
  signal failing
on destroyed containment-device
  fail 16 20
  say security "Metar forces have wrecked the field containment device!"
  signal failing
on signal failing
  timer fail-message 10
on timer fail-message
  say command "We have not met our objectives."

# Not expressible yet:
# - The fault scan is of the cell at 154:237.
# - The sabotage timers hang on the saboteur's spawner, which by the spec fires when its
#   units are dead; written (as the lines read) as following the spawn.
# - The pass and fail messages and the Hubrin and hub lines go to all three empires' mail.
# - The Taubat and Hubrin slots otherwise hold a dummy and a guard order (Hubrin_Guard).

# --- J8 as F ----------------------------------------
# J8 as the Federation (slot 0), from the game's own mission graph.
# Research and build the anomaly device, then get it to the detonation site
# while keeping the vehicle yard and the device itself alive.
mission J8 F
zone detonation-site 252 307 260 315
objective 13
objective 15
objective 17

# Fail criteria, the game's way: the cause at once, the end 10 s later (no
# word to the other sides in J8). Our hub (14), our last vehicle yard (16),
# and the device (18), watched only once it's built (see objective 13).
on destroyed colony-hub
  fail 14 10
  say command "Colony hub lost. The mission has failed."
on count vehicle-yard == 0
  fail 16 10
  say command "Vehicle yard lost. The mission has failed."
on destroyed anomaly-device as device-watch dormant
  fail 18 10
  say command "The anomaly device has been destroyed. The mission has failed."

# Start: the device research is open from the start (nothing in the graph
# unlocks it); the other sides' clocks
on start
  research anomaly-device
  move met-starship 202 237 276 410 178 359 202 237 loop   # the Metar starship's patrol
  timer kli-raid-fed 90
  timer kli-raid-rom 90
  timer rom-raid-fed 90
  timer rom-raid-kli 90
  timer met-raid-fed 120
  timer met-raid-kli 120
  timer met-raid-rom 120
  timer metar-colony-protection-relay-one 16
  timer metar-colony-protection-relay-two 16
  timer metar-colony-protection-relay-three 16
  timer metar-colony-protection-relay-four 16
  timer metar-federation-attack 960
  timer metar-klingon-attack 960
  timer metar-romulan-attack 960

# Objective 13: the research done, the device built; a minute after the
# research our device is given its route to the site
on researched anomaly-device
  say science "Our researchers have worked out how to build the device."
  timer device-route 60
on timer device-route
  ai federation fed-anomaly-device-controller
on built anomaly-device
  say engineering "Our anomaly device is built and setting off. Guard it with everything we have."
  arm device-watch
  complete 13

# Objectives 15 and 17: our device inside the detonation area
on inside anomaly-device detonation-site
  say engineering "The device is at its target and starting up. We've done it."
  complete 15
  complete 17

# Mission passed: the win 10 s later (no announcement in J8)
on all-completed
  timer pass 10
on timer pass
  win

# The other sides' AI: orders on a clock (the Federation's own attack orders
# are left out: the player gives those)
# Klingon raids on the Federation: one of eight attacks every 30 s, the first at 90 s
every timer kli-raid-fed
  timer kli-raid-fed 30
  random kli-raid-fed-1 kli-raid-fed-2 kli-raid-fed-3 kli-raid-fed-4 kli-raid-fed-5 kli-raid-fed-6 kli-raid-fed-7 kli-raid-fed-8
every signal kli-raid-fed-1
  ai klingon kli-assault-fed-random1
every signal kli-raid-fed-2
  ai klingon kli-assault-fed-random2
every signal kli-raid-fed-3
  ai klingon kli-assault-fed-random3
every signal kli-raid-fed-4
  ai klingon kli-assault-fed-random4
every signal kli-raid-fed-5
  ai klingon kli-assault-fed-random5
every signal kli-raid-fed-6
  ai klingon kli-assault-fed-random6
every signal kli-raid-fed-7
  ai klingon kli-assault-fed-random7
every signal kli-raid-fed-8
  ai klingon kli-assault-fed-random8

# Klingon raids on the Romulans: one of eight attacks every 30 s, the first at 90 s
every timer kli-raid-rom
  timer kli-raid-rom 30
  random kli-raid-rom-1 kli-raid-rom-2 kli-raid-rom-3 kli-raid-rom-4 kli-raid-rom-5 kli-raid-rom-6 kli-raid-rom-7 kli-raid-rom-8
every signal kli-raid-rom-1
  ai klingon kli-assault-rom-random1
every signal kli-raid-rom-2
  ai klingon kli-assault-rom-random2
every signal kli-raid-rom-3
  ai klingon kli-assault-rom-random3
every signal kli-raid-rom-4
  ai klingon kli-assault-rom-random4
every signal kli-raid-rom-5
  ai klingon kli-assault-rom-random5
every signal kli-raid-rom-6
  ai klingon kli-assault-rom-random6
every signal kli-raid-rom-7
  ai klingon kli-assault-rom-random7
every signal kli-raid-rom-8
  ai klingon kli-assault-rom-random8

# Romulan raids on the Federation: one of eight attacks every 30 s, the first at 90 s
every timer rom-raid-fed
  timer rom-raid-fed 30
  random rom-raid-fed-1 rom-raid-fed-2 rom-raid-fed-3 rom-raid-fed-4 rom-raid-fed-5 rom-raid-fed-6 rom-raid-fed-7 rom-raid-fed-8
every signal rom-raid-fed-1
  ai romulan rom-assault-fed-random1
every signal rom-raid-fed-2
  ai romulan rom-assault-fed-random2
every signal rom-raid-fed-3
  ai romulan rom-assault-fed-random3
every signal rom-raid-fed-4
  ai romulan rom-assault-fed-random4
every signal rom-raid-fed-5
  ai romulan rom-assault-fed-random5
every signal rom-raid-fed-6
  ai romulan rom-assault-fed-random6
every signal rom-raid-fed-7
  ai romulan rom-assault-fed-random7
every signal rom-raid-fed-8
  ai romulan rom-assault-fed-random8

# Romulan raids on the Klingons: one of eight attacks every 30 s, the first at 90 s
every timer rom-raid-kli
  timer rom-raid-kli 30
  random rom-raid-kli-1 rom-raid-kli-2 rom-raid-kli-3 rom-raid-kli-4 rom-raid-kli-5 rom-raid-kli-6 rom-raid-kli-7 rom-raid-kli-8
every signal rom-raid-kli-1
  ai romulan rom-assault-kli-random1
every signal rom-raid-kli-2
  ai romulan rom-assault-kli-random2
every signal rom-raid-kli-3
  ai romulan rom-assault-kli-random3
every signal rom-raid-kli-4
  ai romulan rom-assault-kli-random4
every signal rom-raid-kli-5
  ai romulan rom-assault-kli-random5
every signal rom-raid-kli-6
  ai romulan rom-assault-kli-random6
every signal rom-raid-kli-7
  ai romulan rom-assault-kli-random7
every signal rom-raid-kli-8
  ai romulan rom-assault-kli-random8

# Metar raids on the Federation: one of eight attacks every 60 s, the first at 120 s
every timer met-raid-fed
  timer met-raid-fed 60
  random met-raid-fed-1 met-raid-fed-2 met-raid-fed-3 met-raid-fed-4 met-raid-fed-5 met-raid-fed-6 met-raid-fed-7 met-raid-fed-8
every signal met-raid-fed-1
  ai metar metar-assault-fed-random1
every signal met-raid-fed-2
  ai metar metar-assault-fed-random2
every signal met-raid-fed-3
  ai metar metar-assault-fed-random3
every signal met-raid-fed-4
  ai metar metar-assault-fed-random4
every signal met-raid-fed-5
  ai metar metar-assault-fed-random5
every signal met-raid-fed-6
  ai metar metar-assault-fed-random6
every signal met-raid-fed-7
  ai metar metar-assault-fed-random7
every signal met-raid-fed-8
  ai metar metar-assault-fed-random8

# Metar raids on the Klingons: one of eight attacks every 60 s, the first at 120 s
every timer met-raid-kli
  timer met-raid-kli 60
  random met-raid-kli-1 met-raid-kli-2 met-raid-kli-3 met-raid-kli-4 met-raid-kli-5 met-raid-kli-6 met-raid-kli-7 met-raid-kli-8
every signal met-raid-kli-1
  ai metar metar-assault-kli-random1
every signal met-raid-kli-2
  ai metar metar-assault-kli-random2
every signal met-raid-kli-3
  ai metar metar-assault-kli-random3
every signal met-raid-kli-4
  ai metar metar-assault-kli-random4
every signal met-raid-kli-5
  ai metar metar-assault-kli-random5
every signal met-raid-kli-6
  ai metar metar-assault-kli-random6
every signal met-raid-kli-7
  ai metar metar-assault-kli-random7
every signal met-raid-kli-8
  ai metar metar-assault-kli-random8

# Metar raids on the Romulans: one of eight attacks every 60 s, the first at 120 s
every timer met-raid-rom
  timer met-raid-rom 60
  random met-raid-rom-1 met-raid-rom-2 met-raid-rom-3 met-raid-rom-4 met-raid-rom-5 met-raid-rom-6 met-raid-rom-7 met-raid-rom-8
every signal met-raid-rom-1
  ai metar metar-assault-rom-random1
every signal met-raid-rom-2
  ai metar metar-assault-rom-random2
every signal met-raid-rom-3
  ai metar metar-assault-rom-random3
every signal met-raid-rom-4
  ai metar metar-assault-rom-random4
every signal met-raid-rom-5
  ai metar metar-assault-rom-random5
every signal met-raid-rom-6
  ai metar metar-assault-rom-random6
every signal met-raid-rom-7
  ai metar metar-assault-rom-random7
every signal met-raid-rom-8
  ai metar metar-assault-rom-random8

# Metar orders on a clock
on timer metar-colony-protection-relay-one
  ai metar metar-colony-protection-one
  ai metar metar-colony-protection-two
  ai metar metar-colony-protection-three
  ai metar metar-colony-protection-four
on timer metar-colony-protection-relay-two
  ai metar metar-colony-protection-five
  ai metar metar-colony-protection-six
  ai metar metar-colony-protection-seven
  ai metar metar-colony-protection-eight
on timer metar-colony-protection-relay-three
  ai metar metar-colony-protection-nine
  ai metar metar-colony-protection-ten
  ai metar metar-colony-protection-eleven
  ai metar metar-colony-protection-twelve
on timer metar-colony-protection-relay-four
  ai metar metar-colony-protection-thirteen
on timer metar-federation-attack
  ai metar metar-federation-assault-one
  ai metar metar-federation-assault-two
on timer metar-klingon-attack
)",
R"(  ai metar metar-klingon-assault-one
  ai metar metar-klingon-assault-two
on timer metar-romulan-attack
  ai metar metar-romulan-assault-one
  ai metar metar-romulan-assault-two

# Not expressible yet:
# - The rivals' devices: their research, building and route orders (60 s
#   after their research) happen on their side; there's no event for a
#   rival's research.
# - Objective 15 (protect the vehicle yard) is met only when the device
#   arrives, together with 17; the game words it as its own objective but
#   checks nothing else.

# --- F14 as F ----------------------------------------
# F14 as the Federation, rewritten from the game's own graph (STNW.exe dump F14.json, slot 0,
# with the Taubat (3) and Metar (5) objects that act on the Federation).
mission F14 F
site metar-core 386 285
objective 13
objective 15

# --- Taubat raids from 30 s, every 30 s; Metar raids from 120 s, every 100 s (one of eight at
# --- random each); set Metar assaults at 60, 180 and 300 s, and on the device at 360 s ---------
on start
  timer tau-raids 30
  timer met-raids 120
  timer met-assault-1 60
  timer met-assault-2 180
  timer met-assault-3 300
  timer met-cfd-attack 360
every timer tau-raids
  random tau-raid-1 tau-raid-2 tau-raid-3 tau-raid-4 tau-raid-5 tau-raid-6 tau-raid-7 tau-raid-8
  timer tau-raids 30
every signal tau-raid-1
  ai taubat tau-assault-fed-random1
every signal tau-raid-2
  ai taubat tau-assault-fed-random2
every signal tau-raid-3
  ai taubat tau-assault-fed-random3
every signal tau-raid-4
  ai taubat tau-assault-fed-random4
every signal tau-raid-5
  ai taubat tau-assault-fed-random5
every signal tau-raid-6
  ai taubat tau-assault-fed-random6
every signal tau-raid-7
  ai taubat tau-assault-fed-random7
every signal tau-raid-8
  ai taubat tau-assault-fed-random8
every timer met-raids
  random met-raid-1 met-raid-2 met-raid-3 met-raid-4 met-raid-5 met-raid-6 met-raid-7 met-raid-8
  timer met-raids 100
every signal met-raid-1
  ai metar met-assault-fed-random1
every signal met-raid-2
  ai metar met-assault-fed-random2
every signal met-raid-3
  ai metar met-assault-fed-random3
every signal met-raid-4
  ai metar met-assault-fed-random4
every signal met-raid-5
  ai metar met-assault-fed-random5
every signal met-raid-6
  ai metar met-assault-fed-random6
every signal met-raid-7
  ai metar met-assault-fed-random7
every signal met-raid-8
  ai metar met-assault-fed-random8
on timer met-assault-1
  ai metar met-assault-federation-1
on timer met-assault-2
  ai metar met-assault-federation-2
on timer met-assault-3
  ai metar met-assault-federation-3
on timer met-cfd-attack
  ai metar metar-attack-on-cfd

# --- Objectives 13 and 15: scan the Metar core --------------------------------------------------
on cell-scanned metar-core
  say science "Sir, these readings are strange... Hold on! They resemble the signals the anomaly generator gave off."
  complete 13
  complete 15
  timer pass-message 10
  timer pass 20

# --- The pass: a message at 10 s, the win at 20 s --------------------------------------------------
on timer pass-message
  say intelligence "All objectives met. Well done."
on timer pass
  win

# --- Fails: the cause at once, "failed" at 10 s, the end at 20 s ------------------------------
on destroyed colony-hub
  fail 14 20
  say intelligence "The Federation hub has been destroyed."
  signal failing
on destroyed containment-device
  fail 16 20
  say security "The Metar have wrecked the containment field device."
  signal failing
on signal failing
  timer fail-message 10
on timer fail-message
  say intelligence "We have not met our objectives."

# Not expressible yet:
# - The pass and fail messages and the hub and device lines go to all three empires' mail.
# - The Federation has a "dummy" research topic that nothing makes available.

# --- J9 as F ----------------------------------------
# J9 as the Federation (slot 0), from the game's own mission graph.
# Scan the Metar temple for the sentient anomaly device, research it, and
# capture one of the Metar starships.
mission J9 F
site metar-temple 264 303
objective 13
objective 15

# Fail criteria, the game's way: the cause at once, the end 10 s later (no
# word to the other sides in J9). Our hub (14); all three Metar starships
# destroyed (16; the game's message calls them colony cores). Fail 17 (a
# rival got the technology while we were scanning) is with the scan below.
on destroyed colony-hub
  fail 14 10
  say command "Colony hub lost. The mission has failed."
on count metar-starship == 0
  fail 16 10
  say command "The Metar colony cores are all gone. The mission has failed."

# Start: the other sides' attack clocks
on start
  timer kli-raid-fed 60
  timer rom-raid-fed 60
  timer rom-raid-kli 60
  timer met-raid-fed 220
  timer met-raid-kli 220
  timer met-raid-rom 220

# Objective 13: scan the temple (10 s before the officer reports, then held
# a minute); the technology is beamed home and its research opens; the
# research done meets the objective. The game's watch is "the device's block
# scanned by us" (any of our scanning units nearby), on the group
# Anomaly_Device: a cell scan.
every scan-started cell anomaly-device
  say engineering "Searching the temple for the device's technology."
every scan-lost cell anomaly-device
  say engineering "We lost the scan. Have another try."
on scanned cell anomaly-device
  say engineering "The technology is beamed home to our hub for research."
  research anomaly-device
# The race for the device: the first side to finish takes the technology. A
# rival finishing while ours runs beats us (fail 17); once a rival has
# finished, our later scan is futile (no technology, no research). The
# computer's sides only win it if their own units happen to scan the block.
on beaten anomaly-device
  fail 17 10
  say engineering "The technology is gone from here; the Klingons or the Romulans must have taken it."
every futile anomaly-device
  say engineering "That scan was useless."
on researched anomaly-device
  say science "Our researchers have worked out how to use the device."
  complete 13
  arm starship-watch

# Objective 15: holding a Metar starship; the game starts watching for it
# only once the research is done
on captured metar-starship as starship-watch dormant
  say engineering "The starship is ours."
  complete 15

# Mission passed: the win 10 s later (no announcement in J9)
on all-completed
  timer pass 10
on timer pass
  win

# The other sides' AI: attacks on a clock (the Federation's own attack orders
# are left out: the player gives those)
# Klingon raids on the Federation: one of eight attacks every 30 s, the first at 60 s
every timer kli-raid-fed
  timer kli-raid-fed 30
  random kli-raid-fed-1 kli-raid-fed-2 kli-raid-fed-3 kli-raid-fed-4 kli-raid-fed-5 kli-raid-fed-6 kli-raid-fed-7 kli-raid-fed-8
every signal kli-raid-fed-1
  ai klingon kli-assault-fed-random1
every signal kli-raid-fed-2
  ai klingon kli-assault-fed-random2
every signal kli-raid-fed-3
  ai klingon kli-assault-fed-random3
every signal kli-raid-fed-4
  ai klingon kli-assault-fed-random4
every signal kli-raid-fed-5
  ai klingon kli-assault-fed-random5
every signal kli-raid-fed-6
  ai klingon kli-assault-fed-random6
every signal kli-raid-fed-7
  ai klingon kli-assault-fed-random7
every signal kli-raid-fed-8
  ai klingon kli-assault-fed-random8

# Romulan raids on the Federation: one of eight attacks every 30 s, the first at 60 s
every timer rom-raid-fed
  timer rom-raid-fed 30
  random rom-raid-fed-1 rom-raid-fed-2 rom-raid-fed-3 rom-raid-fed-4 rom-raid-fed-5 rom-raid-fed-6 rom-raid-fed-7 rom-raid-fed-8
every signal rom-raid-fed-1
  ai romulan rom-assault-fed-random1
every signal rom-raid-fed-2
  ai romulan rom-assault-fed-random2
every signal rom-raid-fed-3
  ai romulan rom-assault-fed-random3
every signal rom-raid-fed-4
  ai romulan rom-assault-fed-random4
every signal rom-raid-fed-5
  ai romulan rom-assault-fed-random5
every signal rom-raid-fed-6
  ai romulan rom-assault-fed-random6
every signal rom-raid-fed-7
  ai romulan rom-assault-fed-random7
every signal rom-raid-fed-8
  ai romulan rom-assault-fed-random8

# Romulan raids on the Klingons: one of eight attacks every 30 s, the first at 60 s
every timer rom-raid-kli
  timer rom-raid-kli 30
  random rom-raid-kli-1 rom-raid-kli-2 rom-raid-kli-3 rom-raid-kli-4 rom-raid-kli-5 rom-raid-kli-6 rom-raid-kli-7 rom-raid-kli-8
every signal rom-raid-kli-1
  ai romulan rom-assault-kli-random1
every signal rom-raid-kli-2
  ai romulan rom-assault-kli-random2
every signal rom-raid-kli-3
  ai romulan rom-assault-kli-random3
every signal rom-raid-kli-4
  ai romulan rom-assault-kli-random4
every signal rom-raid-kli-5
  ai romulan rom-assault-kli-random5
every signal rom-raid-kli-6
  ai romulan rom-assault-kli-random6
every signal rom-raid-kli-7
  ai romulan rom-assault-kli-random7
every signal rom-raid-kli-8
  ai romulan rom-assault-kli-random8

# Metar raids on the Federation: one of eight attacks every 100 s, the first at 220 s
every timer met-raid-fed
  timer met-raid-fed 100
  random met-raid-fed-1 met-raid-fed-2 met-raid-fed-3 met-raid-fed-4 met-raid-fed-5 met-raid-fed-6 met-raid-fed-7 met-raid-fed-8
every signal met-raid-fed-1
  ai metar metar-assault-fed-random1
every signal met-raid-fed-2
  ai metar metar-assault-fed-random2
every signal met-raid-fed-3
  ai metar metar-assault-fed-random3
every signal met-raid-fed-4
  ai metar metar-assault-fed-random4
every signal met-raid-fed-5
  ai metar metar-assault-fed-random5
every signal met-raid-fed-6
  ai metar metar-assault-fed-random6
every signal met-raid-fed-7
  ai metar metar-assault-fed-random7
every signal met-raid-fed-8
  ai metar metar-assault-fed-random8

# Metar raids on the Klingons: one of eight attacks every 100 s, the first at 220 s
every timer met-raid-kli
  timer met-raid-kli 100
  random met-raid-kli-1 met-raid-kli-2 met-raid-kli-3 met-raid-kli-4 met-raid-kli-5 met-raid-kli-6 met-raid-kli-7 met-raid-kli-8
every signal met-raid-kli-1
  ai metar metar-assault-kli-random1
every signal met-raid-kli-2
  ai metar metar-assault-kli-random2
every signal met-raid-kli-3
  ai metar metar-assault-kli-random3
every signal met-raid-kli-4
  ai metar metar-assault-kli-random4
every signal met-raid-kli-5
  ai metar metar-assault-kli-random5
every signal met-raid-kli-6
  ai metar metar-assault-kli-random6
every signal met-raid-kli-7
  ai metar metar-assault-kli-random7
every signal met-raid-kli-8
  ai metar metar-assault-kli-random8

# Metar raids on the Romulans: one of eight attacks every 100 s, the first at 220 s
every timer met-raid-rom
  timer met-raid-rom 100
  random met-raid-rom-1 met-raid-rom-2 met-raid-rom-3 met-raid-rom-4 met-raid-rom-5 met-raid-rom-6 met-raid-rom-7 met-raid-rom-8
every signal met-raid-rom-1
  ai metar metar-assault-rom-random1
every signal met-raid-rom-2
  ai metar metar-assault-rom-random2
every signal met-raid-rom-3
  ai metar metar-assault-rom-random3
every signal met-raid-rom-4
  ai metar metar-assault-rom-random4
every signal met-raid-rom-5
  ai metar metar-assault-rom-random5
every signal met-raid-rom-6
  ai metar metar-assault-rom-random6
every signal met-raid-rom-7
  ai metar metar-assault-rom-random7
every signal met-raid-rom-8
  ai metar metar-assault-rom-random8

# Not expressible yet:
# - The game's capture watch counts starships held, so one captured before
#   the research and still held when the watch starts counts at once; here
#   only a capture made after the research does (the event is the capture).
# - The Klingons' random raids on the Romulans never happen in the game: their
#   timer names orders (Kli_Assault_Kli_Random1-8) that don't exist. Left out.
# - The AI sides' own research and captures (no events for rivals).

# --- C9 as F ----------------------------------------
# C9 "Bonus Federation Mission" (Federation, slot 0; map demo1, colony at 120:359, a
# Romulan colony at 312:295). Rewritten from the game's own graph
# (S/ghidra/scripts_out/C9.json). Orion pirates (slot 5) come in waves every 3 minutes
# for half an hour, from two points in the south (434:301 and 434:373); the mission is
# won once the last wave is beaten. The Federation starts friendly to everyone but the
# Orions.
mission C9 F
site colony 120 359
site romulan-colony 312 295
zone orion-approach 240 311 280 371   # between the colonies
objective 60

# --- The Romulan colony is hit first: a cut-off call for help ---
on start
  timer romulan-hub-hit 70
  timer mine1-hit 90
  timer mine2-hit 110
on timer romulan-hub-hit
  damage rom-colony-hub 80      # the Romulan hub loses 80 % of its strength
  timer romulan-call 1
  timer comms-lost 13
  timer invasion-warning 19
on timer romulan-call
  say "Incoming Romulan transmission" "Imperator Travore calling the Federation colony. A large band of Orion pirates is attacking us, and our tactical computer predicts that your colony and ours will both fall unless we can arrange a tempor-"
on timer comms-lost
  say communications "The transmission has broken off, sir. Either their communications are down, or..."
on timer invasion-warning
  say security "Massive energy signatures, north of us. Orions in such numbers can only be invading!"

# --- Our two mines are hit too ---
on timer mine1-hit
  damage mine1 80               # Mine1 at 268:363
on timer mine2-hit
  damage mine2 80               # Mine2 at 256:299

# --- The Orions near the colonies: their demand ---
on inside orion orion-approach
  say "Incoming transmission" "Federation colony: give yourselves up now and spare your colonists a battle you can't win."

# --- The Orion waves (each order beams in its own group at 434:301 or 434:373) ---
on start
  ai orion orion-attack2        # 2 units from 434:373 at once
  timer orion-first-group 7
  timer orion-wave-1 180
  timer orion-wave-2 360
  timer orion-wave-3 540
  timer orion-wave-4 720
  timer orion-wave-5 900
  timer orion-wave-6 1080
  timer orion-wave-7 1260
  timer orion-wave-8 1440
  timer orion-wave-9 1620
  timer orion-wave-10 1800
on timer orion-first-group
  ai orion orion-attack1        # 2 units from 434:301
on timer orion-wave-1
  ai orion orion-attack1a       # 2 units from each point
  ai orion orion-attack2a
on timer orion-wave-2
  ai orion orion-attack1b       # 4 from each
  ai orion orion-attack2b
on timer orion-wave-3
  ai orion orion-attack1c
  ai orion orion-attack2c
on timer orion-wave-4
  ai orion orion-attack1d
  ai orion orion-attack2d
on timer orion-wave-5
  ai orion orion-attack1e       # 6 from each
  ai orion orion-attack2e
on timer orion-wave-6
  ai orion orion-attack1f
  ai orion orion-attack2f
on timer orion-wave-7
  ai orion orion-attack1g
  ai orion orion-attack2g
on timer orion-wave-8
  ai orion orion-attack1h       # 8 from each
  ai orion orion-attack2h
on timer orion-wave-9
  ai orion orion-attack1i
  ai orion orion-attack2i
on timer orion-wave-10
  ai orion orion-attack1j
  ai orion orion-attack2j

)",
R"(# --- The last wave beaten: that wakes the watch for the last Orion; when none is left,
# Starfleet's message and the objective ---
on destroyed orion-attack-group2j
  arm orions-gone
on count orion-pirate == 0 as orions-gone dormant
  say "Incoming transmission from Starfleet" "Commander, an Orion vessel above Lanus II was intercepted by the USS Latrides and destroyed, and the others have withdrawn. It appears you made it through; your actions earn Starfleet's commendation."
  complete 60
on all-completed
  timer pass-message 10
  timer pass 20
on timer pass-message
  say command "All objectives complete. Well done."
on timer pass
  win

# --- The fail: the colony hub destroyed (said at once, the failure 10 s later, over at 20 s) ---
on destroyed colony-hub
  fail 61 20
  say command "We have lost the colony hub."
  timer fail-message 10
on timer fail-message
  say command "You have not achieved your objectives."

# Not expressible yet:
# - The game's armed monitor tests the count at once: if the last Orions die with group 2j,
#   the "no Orion left" watch fires straight away. Reluva's count event comes only when the
#   count changes, so it must arrive after the group's "destroyed" for the watch to hear it.

# --- K1 as K ----------------------------------------
# K1 as the Klingons, rewritten from the game's own mission graph (Klingon slot,
# plus the Taubat and Hubrin slots' monitors that report to the Klingons).
mission K1 K
site landing 176 87
site fortress 192 171
site lookout-one 240 151
site lookout-two 268 91
site temple 368 111
site hangar 368 119
objective 15
objective 17
objective 19 hidden
objective 20
objective 24 optional

# Opening: the Taubat guard their colony at once; their raids start after 6 minutes.
on start
  timer taubat-guard 16
  timer taubat-raids 360
on timer taubat-guard
  ai taubat taubat-colony-protection-one
  ai taubat taubat-colony-protection-two
on seen landing
  say operations "Taal welcomes you, commander. Serving your house is an honour."

# Objective 15: the first vehicle yard, construction yard, generator, processor and mine.
on built-all vehicle-yard construction-yard power-generator resource-processor mining-station
  say engineering "Every building we needed is up, sir."
  complete 15
  signal objective

# Objective 17: hub and yard upgrades, an arsenal and a disruptor turret.
on built-all colony-hub-upgrade construction-yard-upgrade arsenal disruptor-turret
  say engineering "The colony's defences are in place, sir."
  complete 17
  signal objective

# Finding the fortress or any Taubat site; the first one found sets the Taubat on us.
on seen fortress
  say intelligence "We've found the antiproton source at 200:170: some kind of floating structure. A science scan would give us research data."
  signal taubat-alerted
on seen temple
  say intelligence "The Taubat temple is at 372:107."
  signal taubat-alerted
on seen hangar
  say intelligence "The Taubat vehicle factory is at 372:115."
  signal taubat-alerted
on seen lookout-one
  say intelligence "A Taubat lookout post at 271:89."
  signal taubat-alerted
on seen lookout-two
  say intelligence "A Taubat lookout post at 242:149."
  signal taubat-alerted
on signal taubat-alerted
  say "Incoming message" "Defilers! You trespass on our holy places. We will throw you off our land!"
  ai taubat taubat-klingon-colony-assault-three
  timer taubat-attack-1 30
  timer taubat-attack-2 45
on timer taubat-attack-1
  reveal 19
  say operations "We will not stand for this. They must be wiped out."
  ai taubat taubat-klingon-colony-assault-one
on timer taubat-attack-2
  ai taubat taubat-klingon-colony-assault-two

# Objective 20: scan the Hubrin fortress (an ordinary scan is not enough), then research it.
on scan-started science fortress
  say science "Scan under way."
on scan-lost science fortress
  say science "We lost the scan. Try again."
on scanned science fortress
  say science "Scan done, but the readings are unclear. Someone expert has to scan it properly."
on scan-started strong-science fortress
  say science "Scan under way."
on scan-lost strong-science fortress
  say science "We lost the scan. Try again."
on scanned strong-science fortress
  say science "Scan done. The results have to be researched back at the colony hub."
  research fortress-research-topic
on researched fortress-research-topic
  say science "Study of the fortress: it seems to have housed an ancient people who called themselves the Hubrin."
  complete 20
  signal objective

# Objective 19: houses and lookouts, the vehicle factory, then every Taubat vehicle.
on count taubat-house == 0
  say intelligence "The Taubat lookouts and dwellings are destroyed."
  signal taubat-down
on destroyed taubat-hangar
  timer hangar-gone 10
on timer hangar-gone
  say intelligence "The Taubat vehicle factory is gone. Now make sure none of their vehicles survive!"
  arm taubat-tanks
  signal taubat-down
# the vehicle count is only watched once the factory watch has seen it gone for 10 s
on count taubat-fighting-units <= 0 as taubat-tanks dormant
  say intelligence "Every Taubat vehicle is destroyed."
  signal taubat-down
on signal taubat-down*3
  say intelligence "The Taubat won't trouble us much any more."
  complete 19
  signal objective

# Optional objective 24: hold the temple 10 s; its transporter sends minerals home.
on entered taubat-temple
  say security "The temple is ours. Switching on its mineral transporter."
every capture-lost taubat-temple
  say security "The temple has slipped from our grip. Send reinforcements to take it back."
on held taubat-temple 10
  say engineering "The minerals have reached our base. We should pull out of the temple and destroy it."
  complete 24
  give polycomposite 100
  give transaluminium 100
  give duranium 1000
on destroyed taubat-temple
  timer temple-gone 10
on timer temple-gone
  say intelligence "The Taubat temple is destroyed."

# Mission pass: four objective events (15, 17, 20, 19), message at 10 s, win at 20 s.
on signal objective*4
  timer pass-message 10
  timer pass 20
on timer pass-message
  say operations "Every objective is met. Well done."
on timer pass
  win

# Mission fail: the colony hub lost. Message now, verdict at 10 s, the end at 20 s.
on count colony-hub == 0
  fail 25 20
  say operations "The colony hub is destroyed. The mission has failed."
  timer fail-message 10
on timer fail-message
  say operations "We have not achieved our objectives."

# Taubat raids: from 6 minutes, one of eight assaults every 4 minutes.
every timer taubat-raids
  random raid-1 raid-2 raid-3 raid-4 raid-5 raid-6 raid-7 raid-8
  timer taubat-raids 240
every signal raid-1
  ai taubat tau-assault-kli-random1
every signal raid-2
  ai taubat tau-assault-kli-random2
every signal raid-3
  ai taubat tau-assault-kli-random3
every signal raid-4
  ai taubat tau-assault-kli-random4
every signal raid-5
  ai taubat tau-assault-kli-random5
every signal raid-6
  ai taubat tau-assault-kli-random6
every signal raid-7
  ai taubat tau-assault-kli-random7
every signal raid-8
  ai taubat tau-assault-kli-random8

# Scans that find nothing.
every scanned-nothing science
  say science "Ordinary science scans show nothing unusual, sir."
every scanned-nothing strong-science
  say science "A detailed scan, but everything is as expected."
every scanned-nothing medical
  say medical "Medical scans find nothing abnormal."
every scanned-nothing strong-medical
  say medical "Full diagnostics: nothing of interest here."
every scanned-nothing engineering
  say engineering "Structures and systems are within normal limits."
every scanned-nothing strong-engineering
  say engineering "Nothing here is worth our attention."

# Not expressible yet:
# - "Scan again" lines for the fortress (weak: send Rok; strong: we already have the data)
#   hang off the raids' futile output, which nothing in K1 triggers: they never play.
# - The fail criterion goes to (the game's line), a name the game doesn't know, so
#   fail 25 is never ticked; the mission still ends 20 s after the hub is lost.
# - The briefing (phase 2): the shuttle flight over the map and the briefing units (left out).
# - The Taubat's ten unnamed patrols run from the start (AI orders without a script).

# --- K2 as K ----------------------------------------
# K2 as the Klingons, rewritten from the game's own mission graph (Klingon slot, the
# allied Taubat in slot 3 and the rebel Taubat in slot 4).
mission K2 K
site colony 296 247
site ally-base 280 135
site enemy-base 152 207
zone east-approach 292 287 300 299
zone south-approach 276 203 300 215
zone ally-approach 248 95 256 167
zone rebel-front 176 123 192 215
zone rebel-flank 132 231 152 251
objective 13
objective 14
objective 15
objective 16
objective 17

# Opening: the allies call for help; both Taubat sides take up guard, the rebels
# send three assaults at the colony after 16 s and start raiding at 6 minutes.
on start
  timer start-message 1
  timer taubat-guard 16
  timer rebel-raids 360
  timer rebel-strike 640
on timer start-message
  say "Incoming message" "The heretics are rising against us. Now keep your promises and help us purge our world of them."
on timer taubat-guard
  ai taubat allied-taubat-protection-one
  ai taubat allied-taubat-protection-two
  ai taubat enemy-taubat-protection-one
  ai taubat enemy-taubat-protection-two
  ai taubat enemy-taubat-protection-three
  ai taubat taubat-klingon-colony-assault-one
  ai taubat taubat-klingon-colony-assault-two
  ai taubat taubat-klingon-colony-assault-three
on seen ally-base
  say intelligence "The allied Taubat base is at 279:126."

# Warnings: rebels nearing the colony (said once) or the allied base.
on inside enemy-taubat east-approach
  signal colony-threatened
on inside enemy-taubat south-approach
  signal colony-threatened
on signal colony-threatened
  say intelligence "Enemy units are closing on our colony, sir."
on inside enemy-taubat ally-approach
  say intelligence "Enemy units are closing on the allied Taubat base, sir."

# The rebels fall on the allies when our tanks or artillery push towards them,
# or after 640 s, whichever comes first.
on inside disruptor-tank rebel-front
  signal rebels-strike
on inside disruptor-tank rebel-flank
  signal rebels-strike
on inside photon-artillery rebel-front
  signal rebels-strike
on inside photon-artillery rebel-flank
  signal rebels-strike
on timer rebel-strike
  signal rebels-strike
on signal rebels-strike
  stop rebel-strike
  ai taubat taubat-taubat-colony-assault-one
  ai taubat taubat-taubat-colony-assault-two
  ai taubat taubat-taubat-colony-assault-three
  ai taubat taubat-taubat-colony-assault-four
  ai taubat taubat-taubat-colony-assault-five
  ai taubat taubat-taubat-colony-assault-six
  ai taubat taubat-taubat-colony-assault-seven

# Objectives 13 and 14: five disruptor turrets and eight disruptor tanks.
on count disruptor-turret == 5
  say engineering "The defensive weapon systems are finished."
  complete 13
  signal defences
on count disruptor-tank == 8
  say engineering "The disruptor tanks are ready."
  complete 14
  signal defences
on signal defences*2
  say engineering "The colony defences we needed are all in place."
  signal objective

# Objective 17: finding the rebel base shows who is behind them.
on seen enemy-base
  say intelligence "There are Orions in the rebel Taubat base. They are plainly behind the uprising."
  complete 17
  signal objective

# Objective 16: rebel houses, temple, both vehicle factories, then their gun platforms.
on count enemy-taubat-colony == 0   # the rebels' houses carry this label
  say intelligence "The rebel Taubat houses are destroyed."
  signal rebels-down
on destroyed enemy-taubat-temple
  timer rebel-temple-gone 10
on timer rebel-temple-gone
  say intelligence "The rebel Taubat temple is destroyed."
  signal rebels-down
on destroyed enemy-taubat-hangar-one
  timer rebel-hangar-one-gone 10
on timer rebel-hangar-one-gone
  say intelligence "A rebel Taubat vehicle factory is destroyed."
  signal rebels-down
  signal rebel-hangars
on destroyed enemy-taubat-hangar-two
  timer rebel-hangar-two-gone 10
on timer rebel-hangar-two-gone
  say intelligence "A rebel Taubat vehicle factory is destroyed."
  signal rebels-down
  signal rebel-hangars
# the gun-platform count is only watched once both factory watches have reported
on signal rebel-hangars*2
  arm rebel-vehicles
on count enemy-taubat-fighting-units <= 0 as rebel-vehicles dormant
  say intelligence "The rebel Taubat gun platforms are destroyed."
  signal rebels-down
on signal rebels-down*5
  say intelligence "Every rebel Taubat force has been crushed."
  complete 16
  signal objective

# Mission pass: defences, base found, rebels crushed; then objective 15 (the allies
# survived), a message at 10 s and the win at 20 s.
on signal objective*3
  complete 15
  timer pass-message 10
  timer pass 20
on timer pass-message
  say operations "Every objective is met. Well done."
on timer pass
  win

# The allies suffering (three blows lose the mission). Each watch fires once and then
# sleeps ("every", so the walkthrough doesn't play them); the allies' vehicle count is
# only watched from 10 s after their factory falls.
every destroyed taubat-hangar as ally-hangar
  disarm ally-hangar
  timer ally-hangar-gone 10
every timer ally-hangar-gone
  say "Incoming message" "They are wiping us out. We need help!"
  signal allies-hit
  arm ally-vehicles
every count taubat-fighting-units <= 1 as ally-vehicles dormant
  disarm ally-vehicles
  say "Incoming message" "We are being overrun."
  signal allies-hit
every count taubat-house == 0 as ally-houses
  disarm ally-houses
  say "Incoming message" "Our homes are on fire. Why will you not help us?"
  signal allies-hit

# Mission fail: the colony hub, the allies (three blows) or the allied temple; the
# cause's message now, the verdict at 10 s, the end at 20 s.
on count colony-hub == 0
  fail 19 20
  say operations "The colony hub is destroyed. The mission has failed."
  signal failing
on signal allies-hit*3
  fail 20 20
  say operations "The allied Taubat are destroyed. The mission has failed."
  signal failing
on count allied-temple == 0
  fail 21 20
  say "Incoming message" "The temple lies in ruins. Everything is lost! Now the gods will swallow us whole."
  say operations "The allied Taubat temple is destroyed. The mission has failed."
  signal failing
on signal failing
  timer fail-message 10
on timer fail-message
  say operations "We have not achieved our objectives."

# Rebel raids: from 6 minutes, one of eight assaults every 4 minutes.
every timer rebel-raids
  random raid-1 raid-2 raid-3 raid-4 raid-5 raid-6 raid-7 raid-8
  timer rebel-raids 240
every signal raid-1
  ai taubat taubat-assault-kli-random1
every signal raid-2
  ai taubat taubat-assault-kli-random2
every signal raid-3
  ai taubat taubat-assault-kli-random3
every signal raid-4
  ai taubat taubat-assault-kli-random4
every signal raid-5
)",
R"(  ai taubat taubat-assault-kli-random5
every signal raid-6
  ai taubat taubat-assault-kli-random6
every signal raid-7
  ai taubat taubat-assault-kli-random7
every signal raid-8
  ai taubat taubat-assault-kli-random8

# Scans that find nothing.
every scanned-nothing science
  say science "Ordinary science scans show nothing unusual."
every scanned-nothing strong-science
  say science "A detailed scan... nothing out of the ordinary."
every scanned-nothing medical
  say medical "Ordinary medical scans find nothing abnormal."
every scanned-nothing strong-medical
  say medical "Full diagnostics: nothing of interest here."
every scanned-nothing engineering
  say engineering "Structures and systems are within normal limits."
every scanned-nothing strong-engineering
  say engineering "Nothing of interest here."

# Not expressible yet:
# - The approach warnings filter by unit class (tanks; near the allies a wider mask);
#   here any rebel unit counts.
# - Both factions' houses and hangars share Reluva's type names (taubat-house,
#   taubat-hangar), so "count taubat-house == 0" (the allies' houses) also waits for
#   the rebels', and a rebel hangar destroyed also reads as "destroyed taubat-hangar".
# - A research topic about the allies' reactors (150000 points, science) is defined but
#   nothing in the graph makes it available, and its result goes to an object that
#   doesn't exist: it never plays.
# - The briefing (phase 2) and the unnamed patrols.

# --- J1 as K ----------------------------------------
# J1 "Into the Arena", played as the Klingons (slot 1).
# Rewritten from the game's own mission graph (scripts_out/J1.json).
mission J1 K
site klingon-colony 74 93
site taubat-colony 298 205
site temple 394 309
objective 20
objective 21
objective 22 hidden

# --- The other sides' attacks on the colony -------------------------------
# From 480 s a random Federation, Romulan and Taubat assault every 180 s
# each; two Romulan assaults at 900 s; and when we field 14 disruptor tanks
# the Federation answers with two assaults of its own.
on start
  timer rom-assault-klingons 900
  timer fed-raids-on-kli 480
  timer rom-raids-on-kli 480
  timer tau-raids-on-kli 480
on timer rom-assault-klingons
  ai romulan rom-assault-klingons-1
  ai romulan rom-assault-klingons-2
every timer fed-raids-on-kli
  random fed-assault-kli-random1 fed-assault-kli-random2 fed-assault-kli-random3 fed-assault-kli-random4 fed-assault-kli-random5 fed-assault-kli-random6 fed-assault-kli-random7 fed-assault-kli-random8
  timer fed-raids-on-kli 180
every signal fed-assault-kli-random1
  ai federation fed-assault-kli-random1
every signal fed-assault-kli-random2
  ai federation fed-assault-kli-random2
every signal fed-assault-kli-random3
  ai federation fed-assault-kli-random3
every signal fed-assault-kli-random4
  ai federation fed-assault-kli-random4
every signal fed-assault-kli-random5
  ai federation fed-assault-kli-random5
every signal fed-assault-kli-random6
  ai federation fed-assault-kli-random6
every signal fed-assault-kli-random7
  ai federation fed-assault-kli-random7
every signal fed-assault-kli-random8
  ai federation fed-assault-kli-random8
every timer rom-raids-on-kli
  random rom-assault-kli-random1 rom-assault-kli-random2 rom-assault-kli-random3 rom-assault-kli-random4 rom-assault-kli-random5 rom-assault-kli-random6 rom-assault-kli-random7 rom-assault-kli-random8
  timer rom-raids-on-kli 180
every signal rom-assault-kli-random1
  ai romulan rom-assault-kli-random1
every signal rom-assault-kli-random2
  ai romulan rom-assault-kli-random2
every signal rom-assault-kli-random3
  ai romulan rom-assault-kli-random3
every signal rom-assault-kli-random4
  ai romulan rom-assault-kli-random4
every signal rom-assault-kli-random5
  ai romulan rom-assault-kli-random5
every signal rom-assault-kli-random6
  ai romulan rom-assault-kli-random6
every signal rom-assault-kli-random7
  ai romulan rom-assault-kli-random7
every signal rom-assault-kli-random8
  ai romulan rom-assault-kli-random8
every timer tau-raids-on-kli
  random taubat-assault-kli-random1 taubat-assault-kli-random2 taubat-assault-kli-random3 taubat-assault-kli-random4 taubat-assault-kli-random5 taubat-assault-kli-random6 taubat-assault-kli-random7 taubat-assault-kli-random8
  timer tau-raids-on-kli 180
every signal taubat-assault-kli-random1
  ai taubat taubat-assault-kli-random1
every signal taubat-assault-kli-random2
  ai taubat taubat-assault-kli-random2
every signal taubat-assault-kli-random3
  ai taubat taubat-assault-kli-random3
every signal taubat-assault-kli-random4
  ai taubat taubat-assault-kli-random4
every signal taubat-assault-kli-random5
  ai taubat taubat-assault-kli-random5
every signal taubat-assault-kli-random6
  ai taubat taubat-assault-kli-random6
every signal taubat-assault-kli-random7
  ai taubat taubat-assault-kli-random7
every signal taubat-assault-kli-random8
  ai taubat taubat-assault-kli-random8
on count disruptor-tank == 14
  ai federation fed-assault-klingons-1
  ai federation fed-assault-klingons-2

# --- Objective 20: lay waste to the Taubat settlement ---------------------
on colony-total taubat 50   # the settlement down to half its starting size
  say security "Victory: the Taubat settlement lies in ruins."
  complete 20

# --- What the rivals are up to --------------------------------------------
every count romulan-armed-apc == 4
  say intelligence "Warning: the Romulans are getting ready to seize the Taubat settlement."
every captured-by romulan taubat-colony 30
  say intelligence "A Romulan raid has seized part of the Taubat settlement."

# --- Objective 21: take the temple and hold it for a minute ---------------
# Our capture monitor reports each entry until the hold succeeds, then stops.
every entered temple as kli-temple-watch
  say engineering "The Taubat temple is ours. Tracing the power source now."
every vacated temple
  say intelligence "Nobody holds the Taubat temple now."
on held temple 60
  say engineering "The power source is located, yet there's no way to remove it. We should level this temple now."
  complete 21
  reveal 22
  disarm kli-temple-watch
  disarm temple-unheld   # a held temple no longer fails us by being destroyed
  signal temple-step

# --- Objective 22: then destroy the temple ---------------------------------
# The game's counter wants two reports: our hold and the temple's end.
on destroyed taubat-temple
  say intelligence "The Taubat temple has been destroyed."
  signal temple-step
on signal temple-step*2
  complete 22

# --- Rivals at the temple ---------------------------------------------------
# (Captures can't come from the computer's sides: they never capture.)
every captured-by federation temple
  say intelligence "The Federation has got into the Taubat temple."
every captured-by romulan temple
  say intelligence "The Romulans have got into the Taubat temple."

# --- Fails: the cause at once, a line at 10 s, the mission lost at 20 s ----
fail 23 count colony-hub == 0
  say intelligence "The Klingon colony hub has been destroyed."
  signal failing
# A rival's hold on the temple beats us (fail 24); only a human rival can capture.
on held-by federation temple 60
  fail 24 20
  say intelligence "The Federation now controls the Taubat power source."
  disarm temple-unheld
  signal failing
on held-by romulan temple 60
  fail 24 20
  say intelligence "The Romulans now control the Taubat power source."
  disarm temple-unheld
  signal failing
# The temple destroyed before anyone has held it (fail 25): any side's hold
# switches this off.
fail 25 destroyed taubat-temple as temple-unheld
  signal failing
on signal failing
  timer fail-notice 10
on timer fail-notice
  say intelligence "The Klingons are unable to finish their mission."

# --- Mission pass: all three objectives, a line at 10 s, the win at 20 s ----
on all-completed
  timer pass-notice 10
  timer pass 20
on timer pass-notice
  say intelligence "Mission accomplished for the Klingons."
on timer pass
  win

# Not expressible yet:
# - captured-by / held-by <side> temple and captured-by romulan taubat-colony
#   30 never fire: the computer's sides never capture (kept for a human rival).
# - The temple's "vacated" goes to every side each time it's let go.
# - The rivals' own pass/fail lines and what the referee does with a rival's
#   pass; the generic side-failed rule covers only the fail.
# - The Klingons' own AI orders (strike on the Federation at 10 s, build
#   timetable, random raids, assaults on the Romulans when they field 4 armed
#   APCs) only run when the side isn't played.

# --- J2 as K ----------------------------------------
# J2 "Absent Friends", played as the Klingons (slot 1).
# Rewritten from the game's own mission graph (scripts_out/J2.json).
mission J2 K
site klingon-colony 128 447
site alien-structure 112 280   # the Hubrin fortress, as the 120 s message gives it
site taubat-hangar 120 295     # where the staff are held (group Taubat_Hangar)
zone magnetic-storm 244 284 252 292
zone romulan-ridge 96 236 148 296     # the Romulans' Bird of Prey takes off when theirs come here
objective 9
objective 10 hidden

# --- Timed messages, and the Hubrin turning on everyone at 120 s ------------
on start
  storm magnetic-storm
  timer storm-warning 60
  timer kidnap 120
  timer defence-advice 180
  timer weapons-advice 300
  timer mining-advice 360
  timer rom-assault-klingons 65
  timer fed-raids-on-kli 480
  timer rom-raids-on-kli 480
  timer hubrin-raids-on-kli 420
on timer storm-warning
  say intelligence "Our colony shield buffer is fluctuating. I can't be sure, but the magnetic storm may be the cause."
on timer kidnap
  hostile hubrin
  say intelligence "Two of our staff have gone missing! There's a transporter trace near an alien structure at {site:alien-structure}. Start a rescue right away!"
  complete 9
  reveal 10
on timer defence-advice
  say security "We must defend the colony and build our best technology against beings this strong. No wonder the Taubat left this world!"
on timer weapons-advice
  say security "Reports say photon tanks and turrets on the mountain tops may be the answer to the Hubrin's greater range and firepower."
on timer mining-advice
  say intelligence "To develop the colony fully we should set up remote mining sites, guarded by shield generators and turrets."

# --- The other sides' attacks on the colony -------------------------------
# Two Romulan assaults at 65 s; from 480 s a random Federation and Romulan
# assault every 180 s; from 420 s a Hubrin raider every 120 s until the
# Hubrin fortress falls; and the Federation turns on us once the Romulan
# colony has been wiped out.
on timer rom-assault-klingons
  ai romulan rom-klingon-assault-one
  ai romulan rom-klingon-assault-two
every timer fed-raids-on-kli
  random fed-assault-kli-random1 fed-assault-kli-random2 fed-assault-kli-random3 fed-assault-kli-random4 fed-assault-kli-random5 fed-assault-kli-random6 fed-assault-kli-random7 fed-assault-kli-random8
  timer fed-raids-on-kli 180
every signal fed-assault-kli-random1
  ai federation fed-assault-kli-random1
every signal fed-assault-kli-random2
  ai federation fed-assault-kli-random2
every signal fed-assault-kli-random3
  ai federation fed-assault-kli-random3
every signal fed-assault-kli-random4
  ai federation fed-assault-kli-random4
every signal fed-assault-kli-random5
  ai federation fed-assault-kli-random5
every signal fed-assault-kli-random6
  ai federation fed-assault-kli-random6
every signal fed-assault-kli-random7
  ai federation fed-assault-kli-random7
every signal fed-assault-kli-random8
  ai federation fed-assault-kli-random8
every timer rom-raids-on-kli
  random rom-assault-kli-random1 rom-assault-kli-random2 rom-assault-kli-random3 rom-assault-kli-random4 rom-assault-kli-random5 rom-assault-kli-random6 rom-assault-kli-random7 rom-assault-kli-random8
  timer rom-raids-on-kli 180
every signal rom-assault-kli-random1
  ai romulan rom-assault-kli-random1
every signal rom-assault-kli-random2
  ai romulan rom-assault-kli-random2
every signal rom-assault-kli-random3
  ai romulan rom-assault-kli-random3
every signal rom-assault-kli-random4
  ai romulan rom-assault-kli-random4
every signal rom-assault-kli-random5
  ai romulan rom-assault-kli-random5
every signal rom-assault-kli-random6
  ai romulan rom-assault-kli-random6
every signal rom-assault-kli-random7
  ai romulan rom-assault-kli-random7
every signal rom-assault-kli-random8
  ai romulan rom-assault-kli-random8
every timer hubrin-raids-on-kli
  random hubrin-assault-kli-random1 hubrin-assault-kli-random2 hubrin-assault-kli-random3 hubrin-assault-kli-random4 hubrin-assault-kli-random5 hubrin-assault-kli-random6 hubrin-assault-kli-random7 hubrin-assault-kli-random8
  timer hubrin-raids-on-kli 120
every signal hubrin-assault-kli-random1
  ai hubrin hubrin-assault-kli-random1
every signal hubrin-assault-kli-random2
  ai hubrin hubrin-assault-kli-random2
every signal hubrin-assault-kli-random3
  ai hubrin hubrin-assault-kli-random3
every signal hubrin-assault-kli-random4
  ai hubrin hubrin-assault-kli-random4
every signal hubrin-assault-kli-random5
  ai hubrin hubrin-assault-kli-random5
every signal hubrin-assault-kli-random6
  ai hubrin hubrin-assault-kli-random6
every signal hubrin-assault-kli-random7
  ai hubrin hubrin-assault-kli-random7
every signal hubrin-assault-kli-random8
  ai hubrin hubrin-assault-kli-random8
every destroyed hubrin-fortress
  stop hubrin-raids-on-kli
# Any Romulan unit on the ridge sends their Bird of Prey on its flight (it lands at the end).
on inside romulan romulan-ridge
  move rom-bop 145 246 100 299 172 299 50 222
# (Four "none left" watches: three on the Romulans' small buildings, one on
# their large ones; all four must report.)
on count romulan-small-buildings <= 0
  signal rom-colony-down
on count romulan-large-buildings <= 0
  signal rom-colony-down
on signal rom-colony-down*2
  ai federation fed-klingon-assault-one
  ai federation fed-klingon-assault-two

# --- Objective 10: rescue the staff from the Taubat hangar ----------------
# The raid: hold the hangar 10 s to start the search, 60 s more to finish it.
every entered taubat-hangar
  say intelligence "Searching the building for our staff."
every capture-lost taubat-hangar
  say intelligence "Something's wrong: the recovery team has stopped answering."
on captured taubat-hangar as kli-raid
  say intelligence "Our people aren't here. Faint bio-traces show they were taken elsewhere a while back."
  complete 10
  timer new-orders 7
  timer pass 10
on timer new-orders
  say communications "New orders from command: we're being moved to a new outpost, and another team will look into the loss of our staff."
on timer pass
  win
# A rival finishing its raid while ours is still running beats us (fail 12);
# once a rival has finished, our own success is futile (nothing happens).
# (A rival's raid here is a capture, which only a human rival can make.)
on beaten taubat-hangar
  fail 12 10
)",
R"(  say intelligence "A garbled message from our staff: it seems the Federation or the Romulans have raided the building."
every captured-by federation taubat-hangar
  timer fed-took-staff 7
every captured-by romulan taubat-hangar
  timer rom-took-staff 7
on timer fed-took-staff
  say intelligence "The Federation has taken the building and our staff with it."
on timer rom-took-staff
  say intelligence "The Romulans have taken the building and our staff with it."

# --- Fails: the cause at once, the mission lost 10 s later ----------------
on count colony-hub == 0
  fail 11 10
  say operations "The Klingon colony hub has been destroyed."
on destroyed taubat-hangar
  fail 12 10
  say operations "The Taubat hangar holding our staff has been destroyed. The mission has failed."

# Not expressible yet:
# - captured-by <side> taubat-hangar and beaten never fire against the
#   computer's sides: they never capture.
# - The objective sent at 120 s is shown only as the objective change; the
#   game's "new primary objective" mail goes to a distributor, not the panel.
# - The Klingons' own AI orders (colony guard at 16 s, assault on the
#   Federation at 80 s, random raids, an assault on the Romulans when the
#   Federation colony is gone) only run when the side isn't played.

# --- J3 as K ----------------------------------------
# J3 "The Mask Slips", played as the Klingons (slot 1).
# Rewritten from the game's own mission graph (scripts_out/J3.json).
mission J3 K
site klingon-colony 224 191
site workshops 168 239          # the eight Taubat workshops (Raid_Building_One..Eight), 152-184:223-255
site metar-scan-building 176 239   # the anti-Hubrin weapon's target: the Metar colony core
zone metar-valley 160 227 195 248
objective 11
objective 12 hidden
objective 13 hidden

# --- Greeting ---------------------------------------------------------------
on start
  timer greeting 10
  timer rom-assault-klingons 80
  timer fed-raids-on-kli 420
  timer fed-heavy-raids-on-kli 600
  timer rom-raids-on-kli 420
  timer rom-heavy-raids-on-kli 600
  timer fed-scouts 60
  timer rom-scouts 60
  timer tau-raids-on-kli 60
on timer greeting
  say operations "Contacts all over the scanners, east and south: this world is crawling with activity!"

# --- The other sides' attacks on the colony -------------------------------
# Two Romulan assaults at 80 s; Federation and Romulan raids every 180 s
# from 420 s, heavier ones every 180 s from 600 s; a scout every 60 s from
# each; Taubat raids every 60 s from the start; and the Federation turns on
# us once the Romulan colony is wiped out.
on timer rom-assault-klingons
  ai romulan rom-klingon-assault-one
  ai romulan rom-klingon-assault-two
every timer fed-raids-on-kli
  random fed-assault-kli-random1 fed-assault-kli-random2 fed-assault-kli-random3 fed-assault-kli-random1 fed-assault-kli-random2 fed-assault-kli-random3 fed-assault-kli-random2 fed-assault-kli-random3
  timer fed-raids-on-kli 180
every signal fed-assault-kli-random1
  ai federation fed-assault-kli-random1
every signal fed-assault-kli-random2
  ai federation fed-assault-kli-random2
every signal fed-assault-kli-random3
  ai federation fed-assault-kli-random3
every timer fed-heavy-raids-on-kli
  random fed-assault-kli-random1a fed-assault-kli-random2a fed-assault-kli-random3a fed-assault-kli-random4a fed-assault-kli-random1a fed-assault-kli-random2a fed-assault-kli-random3a fed-assault-kli-random4a
  timer fed-heavy-raids-on-kli 180
every signal fed-assault-kli-random1a
  ai federation fed-assault-kli-random1a
every signal fed-assault-kli-random2a
  ai federation fed-assault-kli-random2a
every signal fed-assault-kli-random3a
  ai federation fed-assault-kli-random3a
every signal fed-assault-kli-random4a
  ai federation fed-assault-kli-random4a
every timer rom-raids-on-kli
  random rom-assault-kli-random1 rom-assault-kli-random2 rom-assault-kli-random3 rom-assault-kli-random1 rom-assault-kli-random2 rom-assault-kli-random3 rom-assault-kli-random2 rom-assault-kli-random3
  timer rom-raids-on-kli 180
every signal rom-assault-kli-random1
  ai romulan rom-assault-kli-random1
every signal rom-assault-kli-random2
  ai romulan rom-assault-kli-random2
every signal rom-assault-kli-random3
  ai romulan rom-assault-kli-random3
every timer rom-heavy-raids-on-kli
  random rom-assault-kli-random1a rom-assault-kli-random2a rom-assault-kli-random3a rom-assault-kli-random4a rom-assault-kli-random1a rom-assault-kli-random2a rom-assault-kli-random3a rom-assault-kli-random4a
  timer rom-heavy-raids-on-kli 180
every signal rom-assault-kli-random1a
  ai romulan rom-assault-kli-random1a
every signal rom-assault-kli-random2a
  ai romulan rom-assault-kli-random2a
every signal rom-assault-kli-random3a
  ai romulan rom-assault-kli-random3a
every signal rom-assault-kli-random4a
  ai romulan rom-assault-kli-random4a
every timer fed-scouts
  random fed-scout-kli-random1 fed-scout-kli-random2 fed-scout-kli-random3 fed-scout-kli-random4 fed-scout-rom-random5 fed-scout-rom-random6 fed-scout-rom-random7 fed-scout-rom-random8
  timer fed-scouts 60
every signal fed-scout-kli-random1
  ai federation fed-scout-kli-random1
every signal fed-scout-kli-random2
  ai federation fed-scout-kli-random2
every signal fed-scout-kli-random3
  ai federation fed-scout-kli-random3
every signal fed-scout-kli-random4
  ai federation fed-scout-kli-random4
every signal fed-scout-rom-random5
  ai federation fed-scout-rom-random5
every signal fed-scout-rom-random6
  ai federation fed-scout-rom-random6
every signal fed-scout-rom-random7
  ai federation fed-scout-rom-random7
every signal fed-scout-rom-random8
  ai federation fed-scout-rom-random8
every timer rom-scouts
  random rom-scout-kli-random1 rom-scout-kli-random2 rom-scout-kli-random3 rom-scout-kli-random4 rom-scout-fed-random5 rom-scout-fed-random6 rom-scout-fed-random7 rom-scout-fed-random8
  timer rom-scouts 60
every signal rom-scout-kli-random1
  ai romulan rom-scout-kli-random1
every signal rom-scout-kli-random2
  ai romulan rom-scout-kli-random2
every signal rom-scout-kli-random3
  ai romulan rom-scout-kli-random3
every signal rom-scout-kli-random4
  ai romulan rom-scout-kli-random4
every signal rom-scout-fed-random5
  ai romulan rom-scout-fed-random5
every signal rom-scout-fed-random6
  ai romulan rom-scout-fed-random6
every signal rom-scout-fed-random7
  ai romulan rom-scout-fed-random7
every signal rom-scout-fed-random8
  ai romulan rom-scout-fed-random8
every timer tau-raids-on-kli
  random kli-assualt-random-one kli-assualt-random-two klingon-assualt-one klingon-assualt-two kli-assualt-random-one kli-assualt-random-two klingon-assualt-one klingon-assualt-two
  timer tau-raids-on-kli 60
every signal kli-assualt-random-one
  ai taubat kli-assualt-random-one
every signal kli-assualt-random-two
  ai taubat kli-assualt-random-two
every signal klingon-assualt-one
  ai taubat klingon-assualt-one
every signal klingon-assualt-two
  ai taubat klingon-assualt-two
# (Four "none left" watches: three on the Romulans' small buildings, one on
# their large ones; all four must report.)
on count romulan-small-buildings <= 0
  signal rom-colony-down
on count romulan-large-buildings <= 0
  signal rom-colony-down
on signal rom-colony-down*2
  ai federation fed-klingon-assault-one
  ai federation fed-klingon-assault-two

# --- Objective 11: destroy three of the Taubat workshops ------------------
# The game counts each one 35 s after it falls (5 s, then 30 s more).
on destroyed raid-building-one
  say security "One more Taubat workshop is in ruins!"
  timer workshop-one-counted 35
on timer workshop-one-counted
  signal workshop-destroyed
on destroyed raid-building-two
  say security "One more Taubat workshop is in ruins!"
  timer workshop-two-counted 35
on timer workshop-two-counted
  signal workshop-destroyed
on destroyed raid-building-three
  say security "One more Taubat workshop is in ruins!"
  timer workshop-three-counted 35
on timer workshop-three-counted
  signal workshop-destroyed
on destroyed raid-building-four
  say security "One more Taubat workshop is in ruins!"
  timer workshop-four-counted 35
on timer workshop-four-counted
  signal workshop-destroyed
on destroyed raid-building-five
  say security "One more Taubat workshop is in ruins!"
  timer workshop-five-counted 35
on timer workshop-five-counted
  signal workshop-destroyed
on destroyed raid-building-six
  say security "One more Taubat workshop is in ruins!"
  timer workshop-six-counted 35
on timer workshop-six-counted
  signal workshop-destroyed
on destroyed raid-building-seven
  say security "One more Taubat workshop is in ruins!"
  timer workshop-seven-counted 35
on timer workshop-seven-counted
  signal workshop-destroyed
on destroyed raid-building-eight
  say security "One more Taubat workshop is in ruins!"
  timer workshop-eight-counted 35
on timer workshop-eight-counted
  signal workshop-destroyed
on signal workshop-destroyed*3
  say engineering "Too late! A huge charge of energy is building up, aimed at the central tower!"
  complete 11
  reveal 12
  signal core-go
  signal survive-step
  signal pass-step
# A workshop a rival claims (captured or scanned) while ours is counting
# (beaten), or one that falls after a rival's claim (futile), is lost to us;
# six of them is fail 16. Only a human rival captures or scans, so against
# the computer's sides this never happens.
every beaten raid-building-one
  signal workshop-lost
every beaten raid-building-two
  signal workshop-lost
every beaten raid-building-three
  signal workshop-lost
every beaten raid-building-four
  signal workshop-lost
every beaten raid-building-five
  signal workshop-lost
every beaten raid-building-six
  signal workshop-lost
every beaten raid-building-seven
  signal workshop-lost
every beaten raid-building-eight
  signal workshop-lost
every futile raid-building-one
  signal workshop-lost
every futile raid-building-two
  signal workshop-lost
every futile raid-building-three
  signal workshop-lost
every futile raid-building-four
  signal workshop-lost
every futile raid-building-five
  signal workshop-lost
every futile raid-building-six
  signal workshop-lost
every futile raid-building-seven
  signal workshop-lost
every futile raid-building-eight
  signal workshop-lost
on signal workshop-lost*6
  fail 16 10
  say operations "Too few hangars fell in time. Our mission can't succeed now."

# --- Objective 12: scan the anti-Hubrin weapon's target -------------------
# Held 10 s to start, 60 s more to finish. It wakes the Metar.
every scan-started science metar-scan-building
  say science "Scanners running now, sir."
every scan-lost science metar-scan-building
  say science "The scan has dropped out, sir. It needs another attempt."
on scanned science metar-scan-building as kli-core-scan
  say science "We've finished scanning the anti-Hubrin weapon's target, sir."
  reveal 12
  complete 12
  reveal 13
  timer bop-promise 10
  signal pass-step
  signal metar-wake
  disarm core-watch   # once scanned, its destruction no longer fails us
  signal core-go
on timer bop-promise
  say operations "When an assault group gets into the valley of the Metar, a bird-of-prey will come to support it."

# --- Objective 13: destroy the Metar colony core --------------------------
# The core monitor and the valley detector wake once 11 and 12 are both done.
on signal core-go*2
  arm core-attack
on inside ours metar-valley as core-attack dormant
  say operations "The bird-of-prey is coming to help you finish off the Metar colony core."
  move kli-bop 138 269 162 189 254 177 138 269 loop
on destroyed metar-scan-building as core-attack dormant
  say operations "The Metar colony core is destroyed. Objective complete."
  complete 13
  signal survive-step
  signal pass-step

# --- The Metar wake: our scan, a rival's scan, or a portal destroyed --------
# (A rival's core scan, which makes ours futile, needs a human rival: the
# computer's sides never scan.)
every scanned-by federation metar-scan-building
  signal metar-wake
every scanned-by romulan metar-scan-building
  signal metar-wake
every count metar-portal == 2   # one of the three portals destroyed
  timer portal-surge 5
  timer portal-wake 10
on timer portal-surge
  say operations "An alien structure is down, and surges of power are running through the planet's core."
on timer portal-wake
  signal metar-wake
on signal metar-wake
  timer metar-stirs 10
  timer metar-lifeforms 25
  timer metar-outposts 30
  timer metar-raids-on-kli 60
  signal survive-step
on timer metar-stirs
  say "Incoming message" "See: the Metar sleep no more. Their eyes will turn on you!"
on timer metar-lifeforms
  say operations "Sir, many unknown life forms detected, with energy readings off the scale!"
on timer metar-outposts
  say operations "Our other outposts report the aliens coming up all over the planet and doing heavy damage."
every timer metar-raids-on-kli
  random metar-nak-protect metar-bur-kli-one metar-bur-kli-two metar-nak-kli-one metar-nak-kli-two metar-dea-kli-one metar-dea-kli-two metar-nak-kli-one
  timer metar-raids-on-kli 60
every signal metar-nak-protect
  ai metar metar-nak-protect
every signal metar-bur-kli-one
  ai metar metar-bur-kli-one
every signal metar-bur-kli-two
  ai metar metar-bur-kli-two
every signal metar-nak-kli-one
  ai metar metar-nak-kli-one
every signal metar-nak-kli-two
  ai metar metar-nak-kli-two
every signal metar-dea-kli-one
  ai metar metar-dea-kli-one
every signal metar-dea-kli-two
  ai metar metar-dea-kli-two

# --- Hold out for the bird-of-prey: after 11, 13 and the Metar's waking -----
on signal survive-step*3
  timer council 15
  timer ves-jach-orbit 500
  timer ves-jach-beam 1000
on timer council
  say "Message from the High Council" "Hold your ground: a battle worth songs lies ahead! A ship is coming to take you and your warriors off... should you survive until then."
on timer ves-jach-orbit
  say communications "The Ves Jach has reached low orbit and her captain is calling. Interference from the planet will delay our leaving."
on timer ves-jach-beam
  say "Message from the Ves Jach" "Closing on your position. Stand by to beam out."
  signal pass-step

# --- Mission pass: objectives 11, 12, 13 and the pick-up, then 10 s ---------
on signal pass-step*4
  timer pass 10
on timer pass
  win

# --- Fails: the cause at once, the mission lost 10 s later ----------------
on count colony-hub == 0
  fail 15 10
  say operations "The Klingon colony hub has been destroyed."
# The core destroyed before we scanned it (fail 17); our scan switches this
# off.
on destroyed metar-scan-building as core-watch
  fail 17 10
  say operations "The anti-Hubrin weapon's target has been destroyed. We can't complete the mission."

# Not expressible yet:
# - scanned-by <side> metar-scan-building and the workshops' beaten / futile
#   never fire: the computer's sides never capture or scan.
# - A workshop destroyed after a rival's claim counts for the game only as
#   lost; Reluva still reports it as destroyed (to objective 11) as well.
# - Objective 13's core monitor wakes once 11 and 12 are both done, and a
#   core destroyed before then counts at once; an armed handler here only
#   hears events from then on.
# - The valley detector looks for our units of certain classes (the game's
#   mask), and polls: units already in the valley when it wakes set it off;
#   "inside ours" only hears units coming in after it's armed.
)",
R"(# - The jnt03klg table's string 14 (scan the small alien structures) is not an
#   objective in the game's graph, so it's left out.
# - A resource top-up object for the colony is active but has an empty bundle
#   (top-up needs at least one material; it does nothing anyway).
# - The Klingons' own AI orders (colony guard, assault on the Federation at
#   80 s, raids and scouts, an assault on the Romulans when the Federation
#   colony is gone) only run when the side isn't played.

# --- K7 as K ----------------------------------------
# K7 as the Klingons, rewritten from the game's own mission graph (Klingon slot, with
# the Taubat (slot 3) and Metar (slot 5) raid clocks).
mission K7 K
site colony 176 407
site escape-pod-1 52 307
site escape-pod-2 180 139
objective 10
objective 11
objective 12

# Opening: Taubat raids from 4 minutes (one a minute), Metar raids from 4 minutes
# (one every 2 minutes).
on start
  timer taubat-raids 240
  timer metar-raids 240

# Objective 10: two photon torpedo launchers and the science block's special weapons wing.
on count defensive-artillery == 1
  say engineering "The first photon torpedo launcher is ready."
  signal expansion
on count defensive-artillery == 2
  say engineering "The second photon torpedo launcher is ready."
  signal expansion
on count science-station-upgrade == 1
  say engineering "The science block's special weapons wing is finished."
  signal expansion
on signal expansion*3
  say engineering "The colony expansion we needed is done, sir."
  complete 10
  signal objective

# Objective 11: scan the first escape pod (2 s to lock on, 20 s to finish) to beam the
# scientist out.
on scan-started * escape-pod-1
  say intelligence "Life signs, sir. Scanning for a transporter lock."
on scan-lost * escape-pod-1
  say intelligence "We lost the scan before we had a lock, sir."
on scanned * escape-pod-1
  say intelligence "The scientist has been beamed to the colony, sir."
  complete 11
  signal objective

# Objective 12: scan the second escape pod (same timings) to recover the data core.
on scan-started * escape-pod-2
  say intelligence "No life signs, sir. Scanning for the data core."
on scan-lost * escape-pod-2
  say intelligence "The scan was cut off, sir. Nothing conclusive."
on scanned * escape-pod-2
  say intelligence "We have the starship's data core, sir."
  complete 12
  signal objective

# Mission pass: three objectives, message at 10 s, win at 20 s.
on signal objective*3
  timer pass-message 10
  timer pass 20
on timer pass-message
  say operations "Mission complete."
on timer pass
  win

# Mission fail: the colony hub, or an escape pod (see below); the cause now, the
# verdict at 10 s, the end at 20 s.
on count colony-hub == 0
  fail 8 20
  say operations "The colony hub is destroyed. The mission has failed."
  signal failing
on destroyed escape-pod-1
  fail 9 20
  say intelligence "The escape pod is destroyed. The mission has failed."
  signal failing
on destroyed escape-pod-2
  fail 9 20
  say intelligence "The escape pod is destroyed. The mission has failed."
  signal failing
on signal failing
  timer fail-message 10
on timer fail-message
  say operations "We have not achieved our objectives."

# Raids: one of eight Taubat assaults every 60 s, one of eight Metar assaults every 120 s.
every timer taubat-raids
  random taubat-raid-1 taubat-raid-2 taubat-raid-3 taubat-raid-4 taubat-raid-5 taubat-raid-6 taubat-raid-7 taubat-raid-8
  timer taubat-raids 60
every signal taubat-raid-1
  ai taubat taubat-assault-kli-random1
every signal taubat-raid-2
  ai taubat taubat-assault-kli-random2
every signal taubat-raid-3
  ai taubat taubat-assault-kli-random3
every signal taubat-raid-4
  ai taubat taubat-assault-kli-random4
every signal taubat-raid-5
  ai taubat taubat-assault-kli-random5
every signal taubat-raid-6
  ai taubat taubat-assault-kli-random6
every signal taubat-raid-7
  ai taubat taubat-assault-kli-random7
every signal taubat-raid-8
  ai taubat taubat-assault-kli-random8
every timer metar-raids
  random metar-raid-1 metar-raid-2 metar-raid-3 metar-raid-4 metar-raid-5 metar-raid-6 metar-raid-7 metar-raid-8
  timer metar-raids 120
every signal metar-raid-1
  ai metar metar-assault-kli-random1
every signal metar-raid-2
  ai metar metar-assault-kli-random2
every signal metar-raid-3
  ai metar metar-assault-kli-random3
every signal metar-raid-4
  ai metar metar-assault-kli-random4
every signal metar-raid-5
  ai metar metar-assault-kli-random5
every signal metar-raid-6
  ai metar metar-assault-kli-random6
every signal metar-raid-7
  ai metar metar-assault-kli-random7
every signal metar-raid-8
  ai metar metar-assault-kli-random8

# Not expressible yet:
# - Fail 9 (escape pod destroyed) hangs off the pod raids' "beaten to it" output, which
#   only a Go_Futile message reaches; nothing in K7 sends one. Whether the engine treats
#   a destroyed pod that way is unknown, so "destroyed escape-pod-N" is a guess.
# - A second data-core line (no sign of the general, search the area) exists but nothing
#   sends it.
# - The briefing (phase 2) shuttle flight; the Taubat and Metar patrols and the Taubat's
#   three build orders run without the script.

# --- J4 as K ----------------------------------------
# J4, played as the Klingons (slot 1).
# Rewritten from the game's own mission graph (scripts_out/J4.json).
mission J4 K
site klingon-colony 330 181
zone old-klingon-colony 234 186 262 206
site science-station 258 189      # group ~Kli Sci Blk
site arsenal 234 205              # group ~Kli Sec Cen
site construction-yard 234 189    # group ~Kli Con Yard
site colonial-house 258 173       # group ~Kli Acm Blk
objective 16
objective 17
objective 18
objective 19

# --- Attacks: the Taubat strike the first time a side enters the old colony
on inside ours old-klingon-colony
  ai taubat taubat-sat-kli-colony-assault
on inside federation old-klingon-colony
  ai taubat taubat-sat-fed-colony-assault
on inside romulan old-klingon-colony
  ai taubat taubat-sat-rom-colony-assault

# --- The four recaptures: each held 1 s to start, 5 s more to count --------
# Objective 16: the science station.
on entered science-station
  say intelligence "We're in! Clearing out the Metar."
on capture-lost science-station
  say intelligence "We can't take the building back."
on captured science-station
  say intelligence "Success! The science station is ours again."
  complete 16
  signal recaptured
# Objective 17: the arsenal.
on entered arsenal
  say intelligence "We're in! Clearing out the Metar."
on capture-lost arsenal
  say intelligence "The recapture has failed: they're too strong for us."
on captured arsenal
  say intelligence "Success! The arsenal is ours again."
  complete 17
  signal recaptured
# Objective 18: the construction yard and the colonial house, both.
on entered construction-yard
  say intelligence "We're in! Wiping out the captors."
on capture-lost construction-yard
  say intelligence "We can't take the construction yard back."
on captured construction-yard
  say intelligence "Success! The construction yard is ours again."
  signal yard-and-house
  signal recaptured
on entered colonial-house
  say intelligence "We're in! Clearing out the Metar."
on capture-lost colonial-house
  say intelligence "The recapture has failed: they're too strong for us."
on captured colonial-house
  say intelligence "Success! The colonial house is ours again."
  signal yard-and-house
  signal recaptured
on signal yard-and-house*2
  complete 18
on signal recaptured*4
  timer primaries-notice 10
  signal pass-step
on timer primaries-notice
  say intelligence "The primary objectives are complete."

# --- Objective 19: no Metar or Taubat fighting units left -------------------
on count metar-fighting-units <= 0
  signal natives-gone
on count taubat-fighting-units <= 0
  signal natives-gone
on signal natives-gone*2
  timer secondary-notice 5
  signal pass-step
on timer secondary-notice
  say intelligence "Secondary objective met: every Metar and Taubat unit has been destroyed."
  complete 19

# --- Mission pass: the four recaptures and the natives gone, then 10 s / 20 s
on signal pass-step*2
  timer pass-notice 10
  timer pass 20
on timer pass-notice
  say intelligence "Mission accomplished for the Klingons."
on timer pass
  win

# --- Fails: the cause at once, a line at 10 s, the mission lost at 20 s ----
# (The four buildings' fails come from their raids being "beaten" by a rival;
# nothing in the mission ever sends that, so they can't happen in the game.)
fail 20 beaten science-station
  say intelligence "The science station has been destroyed. The mission has failed."
  signal failing
fail 21 beaten arsenal
  say intelligence "The arsenal has been destroyed. The mission has failed."
  signal failing
fail 22 beaten colonial-house
  say intelligence "The colonial house has been destroyed. The mission has failed."
  signal failing
fail 23 beaten construction-yard
  say intelligence "The construction yard has been destroyed. The objective has failed."
  signal failing
fail 24 count colony-hub == 0
  say intelligence "The Klingon colony hub has been destroyed."
  signal failing
on signal failing
  timer fail-notice 10
on timer fail-notice
  say intelligence "The Klingons are unable to finish their mission."

# Not expressible yet:
# - The recaptures' fail lines hang on their "beaten" outcome, which no
#   object sends (the J4 raids don't hand off); written on the buildings'
#   names, not the raid labels, so Reluva's hand-off can't fire them either.
# - Fail 25 (all Klingon forces destroyed) and the table's string 15 (hold all
#   four at once) have nothing in the game's graph; the pass only counts each
#   recapture once, not simultaneous control.
# - The Klingons' own AI orders (colony guard, assaults on the Federation at
#   300 s and the Romulans at 360 s) only run when the side isn't played.

# --- K8 as K ----------------------------------------
# K8 as the Klingons, rewritten from the game's own mission graph (Klingon slot, with
# the Taubat (slot 3) and Metar (slot 5) monitors and attack clocks).
mission K8 K
site colony 152 303
site taubat-temple 78 293
zone bop-zone 152 235 193 264
zone storm-field 124 228 132 236
site metar-hub 162 213
objective 12
objective 14

# Opening: the Metar send two assaults at 3 minutes and start raiding; the Taubat
# start raiding at 3 minutes and send four assaults at 5 minutes.
on start
  storm storm-field
  timer metar-assault 180
  timer metar-raids 180
  timer taubat-raids 180
  timer taubat-assault 300
  arm metar-units
on timer metar-assault
  ai metar met-assault-klingon-1
  ai metar met-assault-klingon-2
on timer taubat-assault
  ai taubat tau-assault-klingon-1
  ai taubat tau-assault-klingon-2
  ai taubat tau-assault-klingon-3
  ai taubat tau-assault-klingon-4

# Our units reaching the pass east of the Metar base call in the Bird of Prey,
# which flies four legs and lands.
on inside ours bop-zone
  say operations "Our Bird of Prey is on its way to you."
  move kli-bop 182 194 162 254 204 230 87 174

# Coming in sight of the Taubat temple puts the Taubat on guard.
on seen taubat-temple
  ai taubat taubat-colony-protection-one
  ai taubat taubat-colony-protection-two

# Objective 12: the Metar core, the second core and every portal destroyed.
on count metar-hub == 0
  signal metar-sites
on count metar-starship == 0   # the second core: type 223, a grounded starship
  signal metar-sites
on count metar-portal == 0
  signal metar-sites
on signal metar-sites*3
  complete 12
  signal objective

# Objective 14: the Metar's fighting units down to four or fewer (a watch the game wakes
# on its first tick).
on count metar-fighting-units <= 4 as metar-units dormant
  complete 14
  signal objective

# Mission pass: both objectives, message at 10 s, win at 20 s.
on signal objective*2
  timer pass-message 10
  timer pass 20
on timer pass-message
  say intelligence "Victory to the Klingons!"
on timer pass
  win

# Mission fail: the colony hub. Here the game ends at once.
on count colony-hub == 0
  say intelligence "Our hub has been destroyed."
  say intelligence "We have not achieved our objectives."
  lose 13

# Raids: one of eight Taubat and one of eight Metar assaults every 2 minutes.
every timer taubat-raids
  random taubat-raid-1 taubat-raid-2 taubat-raid-3 taubat-raid-4 taubat-raid-5 taubat-raid-6 taubat-raid-7 taubat-raid-8
  timer taubat-raids 120
every signal taubat-raid-1
  ai taubat taubat-assault-kli-random1
every signal taubat-raid-2
  ai taubat taubat-assault-kli-random2
every signal taubat-raid-3
  ai taubat taubat-assault-kli-random3
every signal taubat-raid-4
  ai taubat taubat-assault-kli-random4
every signal taubat-raid-5
  ai taubat taubat-assault-kli-random5
every signal taubat-raid-6
  ai taubat taubat-assault-kli-random6
every signal taubat-raid-7
  ai taubat taubat-assault-kli-random7
every signal taubat-raid-8
  ai taubat taubat-assault-kli-random8
every timer metar-raids
  random metar-raid-1 metar-raid-2 metar-raid-3 metar-raid-4 metar-raid-5 metar-raid-6 metar-raid-7 metar-raid-8
  timer metar-raids 120
every signal metar-raid-1
  ai metar metar-assault-kli-random1
every signal metar-raid-2
  ai metar metar-assault-kli-random2
every signal metar-raid-3
  ai metar metar-assault-kli-random3
every signal metar-raid-4
  ai metar metar-assault-kli-random4
every signal metar-raid-5
  ai metar metar-assault-kli-random5
every signal metar-raid-6
  ai metar metar-assault-kli-random6
every signal metar-raid-7
  ai metar metar-assault-kli-random7
every signal metar-raid-8
  ai metar metar-assault-kli-random8

# Not expressible yet:
# - The objectives' own lines ((the game's line), "75% of the Metar
#   destroyed") sit behind relays that wait for a 10th message and only ever get one:
#   the game never shows them, so they are left out here.
# - Two objects share the name of the pass message (a 10 s timer and an instant relay);
#   which one the game resolves is unknown. Written as the timer.
# - The Taubat and Metar patrols run without the script.

# --- J6 as K ----------------------------------------
# J6 as the Klingons (slot 1), from the game's own mission graph.
# Seize the Vulcan ambassador (scan the Federation hub, then bring down
# their shields and beam him out), and scan the Metar core.
mission J6 K
site federation-colony 274 205
site metar-colony 386 317
objective 13
objective 15
objective 17

# Fail criteria, the game's way: the fail string at once, word to every
# side at 10 s that the Klingons can't complete their mission, the end at
# 20 s. Our hub (19); the Federation hub lost before our strong scan of it
# is done (14: the scan disarms this watch, see objective 13).
on destroyed colony-hub
  fail 19 20
  say intelligence "Our colony hub has been destroyed."
  signal failing
on hub-destroyed federation as federation-hub-watch
  fail 14 20
  signal failing
on signal failing
  timer fail-news 10
on timer fail-news
)",
R"(  say intelligence "The Klingons' mission can no longer succeed."

# Start: the other sides' attack clocks
on start
  timer fed-raid-kli 180
  timer fed-raid-rom 180
  timer rom-raid-fed 180
  timer rom-raid-kli 180
  timer met-raid-fed 480
  timer met-raid-kli 480
  timer met-raid-rom 480
  timer met-assault-klingons-1 1
  timer met-assault-romulans-1 1
  timer met-assault-romulans-2 900
  timer met-assault-federation-1 1
  timer met-assault-federation-2 1200

# Arrivals: the Federation complains at their colony; a reminder near the Metar
on seen federation-colony
  say "Incoming message" "Your advance units have crossed into our protected zone. Neither of us wants an incident - withdraw them at once, please."
on seen metar-colony
  say intelligence "Scan the creatures' hive before you wreck it."

# Objective 13: a strong engineering scan of the Federation hub, held 10 s;
# done, it stops the fail-14 watch on their hub
every scan-started strong-engineering federation-hub
  say engineering "Starting the survey. Now, where has the Vulcan gone to ground?"
every scan-lost strong-engineering federation-hub
  say engineering "Lost the scan. I need more time."
on scanned strong-engineering federation-hub
  say engineering "The Vulcan is in the Federation colony hub, but its shields block the transporter."
  complete 13
  disarm federation-hub-watch
# An ordinary engineering scan of the hub only finds that K'San is needed
every scan-started engineering federation-hub
  say engineering "Scan started."
every scan-lost engineering federation-hub
  say engineering "Lost the scan. Starting over."
on scanned engineering federation-hub
  say engineering "Survey done. Our gear can't get through their screens - bring K'San."

# Objective 15: every Federation shield generator destroyed; the Vulcan is
# beamed over and the objective met 15 s later
on count federation-shield-generator == 0
  timer fed-shields-down 5
  timer ambassador-beamed 15
on timer fed-shields-down
  say engineering "Their shields are down. Transporting."
on timer ambassador-beamed
  say engineering "The Vulcan is ours!"
  complete 15

# Objective 17: a strong science scan of the Metar core (1 s to start, 10 s
# held); it also opens the biodata research
every scan-started strong-science metar-core
  say science "Scan under way. You'll get your answer."
every scan-lost strong-science metar-core
  say science "Lost the scan. I need longer."
on scanned strong-science metar-core
  say science "The biodata says these creatures are changing - mutations. Medical research should study it."
  complete 17
  research metar-biodata
every scan-lost science metar-core
  say science "Lost the scan."
on scanned science metar-core
  say science "Scan done. Nothing we didn't know."
# The game also offers the research when a sickbay ward (type 306) is built
on built sickbay-ward
  research metar-biodata
on researched metar-biodata
  say medical "The biodata analysis is finished, and it's worth a look."

# The Romulans' side of it: with our shields all down they beam their
# device into our hub; 15 s later it goes off and destroys it (the hub's
# loss is fail 19)
on count shield-generator == 0
  timer alien-device 15
on timer alien-device
  say command "Commander, some alien contraption has turned up in the great hall. Orders?"
  damage colony-hub all klingon

# Scans of anything else
every scanned-nothing science
  say science "Routine scan: nothing."
every scanned-nothing strong-science
  say science "Close scan complete. Nothing worth reporting."
every scanned-nothing medical
  say medical "Routine medical scan: all clear."
every scanned-nothing strong-medical
  say medical "Complete diagnostics: nothing to report."
every scanned-nothing engineering
  say engineering "Engineering sweep: nothing."
every scanned-nothing strong-engineering
  say engineering "Detailed engineering sweep: nothing notable."

# Mission passed (all three objectives): the news at 10 s, the win at 20 s
on all-completed
  timer pass-news 10
  timer pass 20
on timer pass-news
  say intelligence "The Klingons' mission is accomplished."
on timer pass
  win

# The other sides' AI: attacks on a clock (the Klingons' own attack orders
# are left out: the player gives those)
# Federation raids on the Klingons: one of eight attacks every 60 s, the first at 180 s
every timer fed-raid-kli
  timer fed-raid-kli 60
  random fed-raid-kli-1 fed-raid-kli-2 fed-raid-kli-3 fed-raid-kli-4 fed-raid-kli-5 fed-raid-kli-6 fed-raid-kli-7 fed-raid-kli-8
every signal fed-raid-kli-1
  ai federation fed-assault-kli-random1
every signal fed-raid-kli-2
  ai federation fed-assault-kli-random2
every signal fed-raid-kli-3
  ai federation fed-assault-kli-random3
every signal fed-raid-kli-4
  ai federation fed-assault-kli-random4
every signal fed-raid-kli-5
  ai federation fed-assault-kli-random5
every signal fed-raid-kli-6
  ai federation fed-assault-kli-random6
every signal fed-raid-kli-7
  ai federation fed-assault-kli-random7
every signal fed-raid-kli-8
  ai federation fed-assault-kli-random8

# Federation raids on the Romulans: one of eight attacks every 60 s, the first at 180 s
every timer fed-raid-rom
  timer fed-raid-rom 60
  random fed-raid-rom-1 fed-raid-rom-2 fed-raid-rom-3 fed-raid-rom-4 fed-raid-rom-5 fed-raid-rom-6 fed-raid-rom-7 fed-raid-rom-8
every signal fed-raid-rom-1
  ai federation fed-assault-rom-random1
every signal fed-raid-rom-2
  ai federation fed-assault-rom-random2
every signal fed-raid-rom-3
  ai federation fed-assault-rom-random3
every signal fed-raid-rom-4
  ai federation fed-assault-rom-random4
every signal fed-raid-rom-5
  ai federation fed-assault-rom-random5
every signal fed-raid-rom-6
  ai federation fed-assault-rom-random6
every signal fed-raid-rom-7
  ai federation fed-assault-rom-random7
every signal fed-raid-rom-8
  ai federation fed-assault-rom-random8

# Romulan raids on the Federation: one of eight attacks every 60 s, the first at 180 s
every timer rom-raid-fed
  timer rom-raid-fed 60
  random rom-raid-fed-1 rom-raid-fed-2 rom-raid-fed-3 rom-raid-fed-4 rom-raid-fed-5 rom-raid-fed-6 rom-raid-fed-7 rom-raid-fed-8
every signal rom-raid-fed-1
  ai romulan rom-assault-fed-random1
every signal rom-raid-fed-2
  ai romulan rom-assault-fed-random2
every signal rom-raid-fed-3
  ai romulan rom-assault-fed-random3
every signal rom-raid-fed-4
  ai romulan rom-assault-fed-random4
every signal rom-raid-fed-5
  ai romulan rom-assault-fed-random5
every signal rom-raid-fed-6
  ai romulan rom-assault-fed-random6
every signal rom-raid-fed-7
  ai romulan rom-assault-fed-random7
every signal rom-raid-fed-8
  ai romulan rom-assault-fed-random8

# Romulan raids on the Klingons: one of eight attacks every 60 s, the first at 180 s
every timer rom-raid-kli
  timer rom-raid-kli 60
  random rom-raid-kli-1 rom-raid-kli-2 rom-raid-kli-3 rom-raid-kli-4 rom-raid-kli-5 rom-raid-kli-6 rom-raid-kli-7 rom-raid-kli-8
every signal rom-raid-kli-1
  ai romulan rom-assault-kli-random1
every signal rom-raid-kli-2
  ai romulan rom-assault-kli-random2
every signal rom-raid-kli-3
  ai romulan rom-assault-kli-random3
every signal rom-raid-kli-4
  ai romulan rom-assault-kli-random4
every signal rom-raid-kli-5
  ai romulan rom-assault-kli-random5
every signal rom-raid-kli-6
  ai romulan rom-assault-kli-random6
every signal rom-raid-kli-7
  ai romulan rom-assault-kli-random7
every signal rom-raid-kli-8
  ai romulan rom-assault-kli-random8

# Metar raids on the Federation: one of eight attacks every 120 s, the first at 480 s
every timer met-raid-fed
  timer met-raid-fed 120
  random met-raid-fed-1 met-raid-fed-2 met-raid-fed-3 met-raid-fed-4 met-raid-fed-5 met-raid-fed-6 met-raid-fed-7 met-raid-fed-8
every signal met-raid-fed-1
  ai metar metar-assault-fed-random1
every signal met-raid-fed-2
  ai metar metar-assault-fed-random2
every signal met-raid-fed-3
  ai metar metar-assault-fed-random3
every signal met-raid-fed-4
  ai metar metar-assault-fed-random4
every signal met-raid-fed-5
  ai metar metar-assault-fed-random5
every signal met-raid-fed-6
  ai metar metar-assault-fed-random6
every signal met-raid-fed-7
  ai metar metar-assault-fed-random7
every signal met-raid-fed-8
  ai metar metar-assault-fed-random8

# Metar raids on the Klingons: one of eight attacks every 120 s, the first at 480 s
every timer met-raid-kli
  timer met-raid-kli 120
  random met-raid-kli-1 met-raid-kli-2 met-raid-kli-3 met-raid-kli-4 met-raid-kli-5 met-raid-kli-6 met-raid-kli-7 met-raid-kli-8
every signal met-raid-kli-1
  ai metar metar-assault-kli-random1
every signal met-raid-kli-2
  ai metar metar-assault-kli-random2
every signal met-raid-kli-3
  ai metar metar-assault-kli-random3
every signal met-raid-kli-4
  ai metar metar-assault-kli-random4
every signal met-raid-kli-5
  ai metar metar-assault-kli-random5
every signal met-raid-kli-6
  ai metar metar-assault-kli-random6
every signal met-raid-kli-7
  ai metar metar-assault-kli-random7
every signal met-raid-kli-8
  ai metar metar-assault-kli-random8

# Metar raids on the Romulans: one of eight attacks every 120 s, the first at 480 s
every timer met-raid-rom
  timer met-raid-rom 120
  random met-raid-rom-1 met-raid-rom-2 met-raid-rom-3 met-raid-rom-4 met-raid-rom-5 met-raid-rom-6 met-raid-rom-7 met-raid-rom-8
every signal met-raid-rom-1
  ai metar metar-assault-rom-random1
every signal met-raid-rom-2
  ai metar metar-assault-rom-random2
every signal met-raid-rom-3
  ai metar metar-assault-rom-random3
every signal met-raid-rom-4
  ai metar metar-assault-rom-random4
every signal met-raid-rom-5
  ai metar metar-assault-rom-random5
every signal met-raid-rom-6
  ai metar metar-assault-rom-random6
every signal met-raid-rom-7
  ai metar metar-assault-rom-random7
every signal met-raid-rom-8
  ai metar metar-assault-rom-random8

# Metar orders on a clock
on timer met-assault-klingons-1
  ai metar met-assault-klingons-1
on timer met-assault-romulans-1
  ai metar met-assault-romulans-1
on timer met-assault-romulans-2
  ai metar met-assault-romulans-2
on timer met-assault-federation-1
  ai metar met-assault-federation-1
on timer met-assault-federation-2
  ai metar met-assault-federation-2

# Not expressible yet:
# - The science officer's "scanning now" for an ordinary Metar scan: the game
#   sends it to the Federation's mail by mistake, so the Klingon player never
#   sees it (left out here).
# - The scan "futile" lines are unreachable in J6 (nothing sends Go_Futile).
# - Messages caused by the other sides: their mission passed/failed
#   announcements, the Federation's "Federation shields down" (swallowed by a
#   relay in the game too). There's no event for a rival's pass.
# - Whether the biodata research needs both the scan and the ward, or either:
#   the game sends the topic from both; here either opens it.
# - The AI sides stop their build orders when their hub falls, and build on
#   their own timetable (npc build orders).

# --- J7 as K ----------------------------------------
# J7 as the Klingons (slot 1), from the game's own mission graph.
# Stock up dilithium and transal for the Hubrin (the game takes a share of
# each), and keep our Hubrin guide alive; both objectives are met together.
mission J7 K
objective 13
objective 15

# Fail criteria, the game's way: the cause at once, word to every side at
# 5 s that the Klingons can't complete their mission, the end at 10 s.
# Our hub (14); our Hubrin guide killed (16).
on destroyed kli-hubrin
  fail 16 10
  say intelligence "Our Hubrin guide is dead."
  signal failing
on destroyed colony-hub
  fail 14 10
  say intelligence "Our colony hub has been destroyed."
  signal failing
on signal failing
  timer fail-news 5
on timer fail-news
  say intelligence "The Klingons' mission can no longer succeed."

# Start: the other sides' attack clocks
on start
  timer fed-raid-kli 180
  timer fed-raid-rom 180
  timer rom-raid-fed 180
  timer rom-raid-kli 180
  timer tau-raid-fed 180
  timer tau-raid-kli 180
  timer tau-raid-rom 180
  timer met-raid-fed 300
  timer met-raid-kli 300
  timer met-raid-rom 300
  timer met-assault-federation 1800
  timer met-assault-klingon 2100
  timer met-assault-romulan 2400

# Objectives 13 and 15: 250 of dilithium in stock (100 is taken), 250 of
# transal in stock (50 is taken); with both delivered the guide's job is done
on stock dilithium 250
  take dilithium 100
  say engineering "Your dilithium is ready."
  signal quota
on stock transalumin 250
  take transalumin 50
  say engineering "The transal consignment is ready to go."
  signal quota
on signal quota*2
  complete 13
  complete 15

# Mission passed: the news at once, the win 10 s later
on all-completed
  say intelligence "The Klingons' mission is accomplished."
  timer pass 10
on timer pass
  win

# A colony grown to four times its starting size draws two Metar attacks
# on it: ours, and the other sides' too
on colony-size 400
  ai metar met-assault-leading-player-kli-a
  ai metar met-assault-leading-player-kli-b
on colony-size federation 400
  ai metar met-assault-leading-player-fed-a
  ai metar met-assault-leading-player-fed-b
on colony-size romulan 400
  ai metar met-assault-leading-player-rom-a
  ai metar met-assault-leading-player-rom-b

# The other sides' AI: attacks on a clock (the Klingons' own attack orders
# are left out: the player gives those)
# Federation raids on the Klingons: one of eight attacks every 60 s, the first at 180 s
every timer fed-raid-kli
  timer fed-raid-kli 60
  random fed-raid-kli-1 fed-raid-kli-2 fed-raid-kli-3 fed-raid-kli-4 fed-raid-kli-5 fed-raid-kli-6 fed-raid-kli-7 fed-raid-kli-8
every signal fed-raid-kli-1
  ai federation fed-assault-kli-random1
every signal fed-raid-kli-2
  ai federation fed-assault-kli-random2
every signal fed-raid-kli-3
  ai federation fed-assault-kli-random3
every signal fed-raid-kli-4
  ai federation fed-assault-kli-random4
every signal fed-raid-kli-5
  ai federation fed-assault-kli-random5
every signal fed-raid-kli-6
  ai federation fed-assault-kli-random6
every signal fed-raid-kli-7
  ai federation fed-assault-kli-random7
every signal fed-raid-kli-8
  ai federation fed-assault-kli-random8

# Federation raids on the Romulans: one of eight attacks every 60 s, the first at 180 s
every timer fed-raid-rom
  timer fed-raid-rom 60
  random fed-raid-rom-1 fed-raid-rom-2 fed-raid-rom-3 fed-raid-rom-4 fed-raid-rom-5 fed-raid-rom-6 fed-raid-rom-7 fed-raid-rom-8
every signal fed-raid-rom-1
  ai federation fed-assault-rom-random1
every signal fed-raid-rom-2
  ai federation fed-assault-rom-random2
every signal fed-raid-rom-3
  ai federation fed-assault-rom-random3
every signal fed-raid-rom-4
  ai federation fed-assault-rom-random4
every signal fed-raid-rom-5
  ai federation fed-assault-rom-random5
every signal fed-raid-rom-6
  ai federation fed-assault-rom-random6
every signal fed-raid-rom-7
  ai federation fed-assault-rom-random7
every signal fed-raid-rom-8
  ai federation fed-assault-rom-random8

)",
R"(# Romulan raids on the Federation: one of eight attacks every 60 s, the first at 180 s
every timer rom-raid-fed
  timer rom-raid-fed 60
  random rom-raid-fed-1 rom-raid-fed-2 rom-raid-fed-3 rom-raid-fed-4 rom-raid-fed-5 rom-raid-fed-6 rom-raid-fed-7 rom-raid-fed-8
every signal rom-raid-fed-1
  ai romulan rom-assault-fed-random1
every signal rom-raid-fed-2
  ai romulan rom-assault-fed-random2
every signal rom-raid-fed-3
  ai romulan rom-assault-fed-random3
every signal rom-raid-fed-4
  ai romulan rom-assault-fed-random4
every signal rom-raid-fed-5
  ai romulan rom-assault-fed-random5
every signal rom-raid-fed-6
  ai romulan rom-assault-fed-random6
every signal rom-raid-fed-7
  ai romulan rom-assault-fed-random7
every signal rom-raid-fed-8
  ai romulan rom-assault-fed-random8

# Romulan raids on the Klingons: one of eight attacks every 60 s, the first at 180 s
every timer rom-raid-kli
  timer rom-raid-kli 60
  random rom-raid-kli-1 rom-raid-kli-2 rom-raid-kli-3 rom-raid-kli-4 rom-raid-kli-5 rom-raid-kli-6 rom-raid-kli-7 rom-raid-kli-8
every signal rom-raid-kli-1
  ai romulan rom-assault-kli-random1
every signal rom-raid-kli-2
  ai romulan rom-assault-kli-random2
every signal rom-raid-kli-3
  ai romulan rom-assault-kli-random3
every signal rom-raid-kli-4
  ai romulan rom-assault-kli-random4
every signal rom-raid-kli-5
  ai romulan rom-assault-kli-random5
every signal rom-raid-kli-6
  ai romulan rom-assault-kli-random6
every signal rom-raid-kli-7
  ai romulan rom-assault-kli-random7
every signal rom-raid-kli-8
  ai romulan rom-assault-kli-random8

# Taubat raids on the Federation: one of eight attacks every 60 s, the first at 180 s
every timer tau-raid-fed
  timer tau-raid-fed 60
  random tau-raid-fed-1 tau-raid-fed-2 tau-raid-fed-3 tau-raid-fed-4 tau-raid-fed-5 tau-raid-fed-6 tau-raid-fed-7 tau-raid-fed-8
every signal tau-raid-fed-1
  ai taubat taubat-assault-fed-random1
every signal tau-raid-fed-2
  ai taubat taubat-assault-fed-random2
every signal tau-raid-fed-3
  ai taubat taubat-assault-fed-random3
every signal tau-raid-fed-4
  ai taubat taubat-assault-fed-random4
every signal tau-raid-fed-5
  ai taubat taubat-assault-fed-random5
every signal tau-raid-fed-6
  ai taubat taubat-assault-fed-random6
every signal tau-raid-fed-7
  ai taubat taubat-assault-fed-random7
every signal tau-raid-fed-8
  ai taubat taubat-assault-fed-random8

# Taubat raids on the Klingons: one of eight attacks every 60 s, the first at 180 s
every timer tau-raid-kli
  timer tau-raid-kli 60
  random tau-raid-kli-1 tau-raid-kli-2 tau-raid-kli-3 tau-raid-kli-4 tau-raid-kli-5 tau-raid-kli-6 tau-raid-kli-7 tau-raid-kli-8
every signal tau-raid-kli-1
  ai taubat taubat-assault-kli-random1
every signal tau-raid-kli-2
  ai taubat taubat-assault-kli-random2
every signal tau-raid-kli-3
  ai taubat taubat-assault-kli-random3
every signal tau-raid-kli-4
  ai taubat taubat-assault-kli-random4
every signal tau-raid-kli-5
  ai taubat taubat-assault-kli-random5
every signal tau-raid-kli-6
  ai taubat taubat-assault-kli-random6
every signal tau-raid-kli-7
  ai taubat taubat-assault-kli-random7
every signal tau-raid-kli-8
  ai taubat taubat-assault-kli-random8

# Taubat raids on the Romulans: one of eight attacks every 60 s, the first at 180 s
every timer tau-raid-rom
  timer tau-raid-rom 60
  random tau-raid-rom-1 tau-raid-rom-2 tau-raid-rom-3 tau-raid-rom-4 tau-raid-rom-5 tau-raid-rom-6 tau-raid-rom-7 tau-raid-rom-8
every signal tau-raid-rom-1
  ai taubat taubat-assault-rom-random1
every signal tau-raid-rom-2
  ai taubat taubat-assault-rom-random2
every signal tau-raid-rom-3
  ai taubat taubat-assault-rom-random3
every signal tau-raid-rom-4
  ai taubat taubat-assault-rom-random4
every signal tau-raid-rom-5
  ai taubat taubat-assault-rom-random5
every signal tau-raid-rom-6
  ai taubat taubat-assault-rom-random6
every signal tau-raid-rom-7
  ai taubat taubat-assault-rom-random7
every signal tau-raid-rom-8
  ai taubat taubat-assault-rom-random8

# Metar raids on the Federation: one of eight attacks every 60 s, the first at 300 s
every timer met-raid-fed
  timer met-raid-fed 60
  random met-raid-fed-1 met-raid-fed-2 met-raid-fed-3 met-raid-fed-4 met-raid-fed-5 met-raid-fed-6 met-raid-fed-7 met-raid-fed-8
every signal met-raid-fed-1
  ai metar metar-assault-fed-random1
every signal met-raid-fed-2
  ai metar metar-assault-fed-random2
every signal met-raid-fed-3
  ai metar metar-assault-fed-random3
every signal met-raid-fed-4
  ai metar metar-assault-fed-random4
every signal met-raid-fed-5
  ai metar metar-assault-fed-random5
every signal met-raid-fed-6
  ai metar metar-assault-fed-random6
every signal met-raid-fed-7
  ai metar metar-assault-fed-random7
every signal met-raid-fed-8
  ai metar metar-assault-fed-random8

# Metar raids on the Klingons: one of eight attacks every 60 s, the first at 300 s
every timer met-raid-kli
  timer met-raid-kli 60
  random met-raid-kli-1 met-raid-kli-2 met-raid-kli-3 met-raid-kli-4 met-raid-kli-5 met-raid-kli-6 met-raid-kli-7 met-raid-kli-8
every signal met-raid-kli-1
  ai metar metar-assault-kli-random1
every signal met-raid-kli-2
  ai metar metar-assault-kli-random2
every signal met-raid-kli-3
  ai metar metar-assault-kli-random3
every signal met-raid-kli-4
  ai metar metar-assault-kli-random4
every signal met-raid-kli-5
  ai metar metar-assault-kli-random5
every signal met-raid-kli-6
  ai metar metar-assault-kli-random6
every signal met-raid-kli-7
  ai metar metar-assault-kli-random7
every signal met-raid-kli-8
  ai metar metar-assault-kli-random8

# Metar raids on the Romulans: one of eight attacks every 60 s, the first at 300 s
every timer met-raid-rom
  timer met-raid-rom 60
  random met-raid-rom-1 met-raid-rom-2 met-raid-rom-3 met-raid-rom-4 met-raid-rom-5 met-raid-rom-6 met-raid-rom-7 met-raid-rom-8
every signal met-raid-rom-1
  ai metar metar-assault-rom-random1
every signal met-raid-rom-2
  ai metar metar-assault-rom-random2
every signal met-raid-rom-3
  ai metar metar-assault-rom-random3
every signal met-raid-rom-4
  ai metar metar-assault-rom-random4
every signal met-raid-rom-5
  ai metar metar-assault-rom-random5
every signal met-raid-rom-6
  ai metar metar-assault-rom-random6
every signal met-raid-rom-7
  ai metar metar-assault-rom-random7
every signal met-raid-rom-8
  ai metar metar-assault-rom-random8

# Metar orders on a clock
on timer met-assault-federation
  ai metar met-assault-federation
on timer met-assault-klingon
  ai metar met-assault-klingon
on timer met-assault-romulan
  ai metar met-assault-romulan

# Not expressible yet:
# - Fail 14: the Klingons' hub monitor names its fail criterion
#   "Colony_hub_destroyed" but the table calls it "Colony_hub-destroyed", so
#   the game ends the mission without ticking the fail string.
# - The Hubrin guides walk to each colony on their own orders (unit orders of
#   the Hubrin side, not started by the script), and the AI sides build on
#   their own timetable and stop when their hub falls.
# - Messages caused by the other sides: their mission passed/failed
#   announcements (sent to every side). There's no event for a rival's pass.

# --- K12 as K ----------------------------------------
# K12 as the Klingons, rewritten from the game's own mission graph (Klingon slot, with
# the Taubat (3), Hubrin (4) and Metar (5) slots' clocks and monitors).
mission K12 K
site colony 82 181
site device-site 152 239
objective 12
objective 14

# Opening: Taubat and Metar raids from the first minute, one each a minute; three
# named Metar assaults at 1, 2 and 3 minutes.
on start
  timer taubat-raids 60
  timer metar-raids 60
  timer metar-assault-1 60
  timer metar-assault-3 120
  timer metar-assault-2 180
on timer metar-assault-1
  ai metar met-assault-klingon-1
on timer metar-assault-3
  ai metar met-assault-klingon-3
on timer metar-assault-2
  ai metar met-assault-klingon-2

# Objective 12: the colony three times its starting size, five generators (or three
# advanced ones) and four vehicle yards; that opens the device's last research.
on colony-size 300
  signal infrastructure
on count power-generator == 5
  signal power
on count advanced-generator == 3
  signal power
on signal power
  signal power-and-yards
on count vehicle-yard == 4
  signal power-and-yards
on signal power-and-yards*2
  signal infrastructure
on signal infrastructure*2
  say engineering "Sir, the colony can now support building the containment device. It needs one last research check."
  complete 12
  signal objective
  research cfd-research

# The device: made by the game 10 s after the research, set down at the Hubrin's site.
on researched cfd-research
  timer device-built 10
on timer device-built
  spawn us containment-device 1 152 239
  say engineering "The device is built. We're taking it to the site the Hubrin marked, to the south."

# The device in place (the build-and-hold watch sees it standing): the Metar go for it
# and a Hubrin is beamed in beside it with a warning.
on standing containment-device 1
  say engineering "Sir, transport is complete. The device for the field's containment is in place now."
  ai metar metar-attack-on-cfd
  spawn hubrin hubrin 1 154 241
  timer sabotage-1 10
  timer sabotage-3 30
on timer sabotage-1
  say intelligence "Sir, another Hubrin has appeared by the device. It says a mistake in the construction has thrown the field off balance."
  timer sabotage-2 10
on timer sabotage-2
  say engineering "A remote scan confirms the fault. A science vehicle must get close to find the imbalance."
on timer sabotage-3
  arm site-scan

# Objective 14: scan the device site and recalibrate (20 s), and keep the device for
# 10 minutes; both are needed. The site scan sleeps until a third sabotage timer,
# 30 s after the device is in place.
on scanned * device-site as site-scan dormant
  say science "Trying to isolate the imbalance. Stand by..."
  timer recalibrate 20
on timer recalibrate
  say engineering "The fault has been located. Now recalibrating the device for the containment field..."
  signal device-online
on standing containment-device 600
  signal device-online
on signal device-online*2
  say engineering "A narrow escape, commander. As first built it would soon have overloaded our whole grid."
  complete 14
  signal objective

# Mission pass: both objectives; the message at once, the win at 10 s.
on signal objective*2
  say intelligence "The Klingons' mission is accomplished."
  timer pass 10
on timer pass
  win

# Mission fail: the colony hub or the device; the cause now, the verdict at 10 s, the
# end at 20 s.
on count colony-hub == 0
  fail 13 20
  say intelligence "Our hub has been destroyed."
  signal failing
on destroyed containment-device
  fail 15 20
  say security "The device for the containment field has been destroyed by the Metar!"
  signal failing
on signal failing
  timer fail-message 10
on timer fail-message
  say intelligence "We have not achieved our objectives."

# The Hubrin observer killed (a message only).
on count hubrin == 0
  say intelligence "The Hubrin observer is dead."

# Raids: one of eight Taubat and one of eight Metar assaults every minute.
every timer taubat-raids
  random taubat-raid-1 taubat-raid-2 taubat-raid-3 taubat-raid-4 taubat-raid-5 taubat-raid-6 taubat-raid-7 taubat-raid-8
  timer taubat-raids 60
every signal taubat-raid-1
  ai taubat taubat-assault-kli-random1
every signal taubat-raid-2
  ai taubat taubat-assault-kli-random2
every signal taubat-raid-3
  ai taubat taubat-assault-kli-random3
every signal taubat-raid-4
  ai taubat taubat-assault-kli-random4
every signal taubat-raid-5
  ai taubat taubat-assault-kli-random5
every signal taubat-raid-6
  ai taubat taubat-assault-kli-random6
every signal taubat-raid-7
  ai taubat taubat-assault-kli-random7
every signal taubat-raid-8
  ai taubat taubat-assault-kli-random8
every timer metar-raids
  random metar-raid-1 metar-raid-2 metar-raid-3 metar-raid-4 metar-raid-5 metar-raid-6 metar-raid-7 metar-raid-8
  timer metar-raids 60
every signal metar-raid-1
  ai metar metar-assault-kli-random1
every signal metar-raid-2
  ai metar metar-assault-kli-random2
every signal metar-raid-3
  ai metar metar-assault-kli-random3
every signal metar-raid-4
  ai metar metar-assault-kli-random4
every signal metar-raid-5
  ai metar metar-assault-kli-random5
every signal metar-raid-6
  ai metar metar-assault-kli-random6
every signal metar-raid-7
  ai metar metar-assault-kli-random7
every signal metar-raid-8
  ai metar metar-assault-kli-random8

# Not expressible yet:
# - The spawner kind is read as firing its message
#   (and the Hubrin's, the sabotage timers) when its units die; the recording shows the
#   lines in sequence without anyone dying, so they are written as following the spawn.
# - The success and failure lines also go to the other two sides' mail (all sides).

# --- J8 as K ----------------------------------------
# J8 as the Klingons (slot 1), from the game's own mission graph.
# Research and build the anomaly device, then get it to the detonation site
# while keeping the vehicle yard and the device itself alive.
mission J8 K
zone detonation-site 252 307 260 315
objective 13
objective 15
objective 17 hidden

# Fail criteria, the game's way: the cause at once, the end 10 s later (no
# word to the other sides in J8). Our hub (14), our last vehicle yard (16),
# and the device (18), watched only once it's built (see objective 13).
on destroyed colony-hub
  fail 14 10
  say command "Colony hub lost. The mission has failed."
on count vehicle-yard == 0
  fail 16 10
  say command "Vehicle yard lost. The mission has failed."
on destroyed anomaly-device as device-watch dormant
  fail 18 10
  say command "The anomaly device has been destroyed. The mission has failed."

# Start: the device research is open from the start (nothing in the graph
# unlocks it); the other sides' clocks
on start
  research anomaly-device
  move met-starship 202 237 276 410 178 359 202 237 loop
  timer fed-raid-kli 90
  timer fed-raid-rom 90
  timer federation-colony-protection-relay 16
  timer rom-raid-fed 90
  timer rom-raid-kli 90
  timer met-raid-fed 120
  timer met-raid-kli 120
  timer met-raid-rom 120
  timer metar-colony-protection-relay-one 16
  timer metar-colony-protection-relay-two 16
  timer metar-colony-protection-relay-three 16
  timer metar-colony-protection-relay-four 16
  timer metar-federation-attack 960
  timer metar-klingon-attack 960
  timer metar-romulan-attack 960

# Objective 13: the research done, the device built; a minute after the
# research our device is given its route to the site
on researched anomaly-device
  say science "Our researchers have worked out how to build the device."
  timer device-route 60
on timer device-route
)",
R"(  ai klingon kli-anomaly-device-controller
# (Objective 17 is hidden until the device is built.)
on built anomaly-device
  say engineering "Our anomaly device is built and setting off. Guard it with everything we have."
  arm device-watch
  complete 13
  reveal 17

# Objectives 15 and 17: the device enters the detonation area
on inside anomaly-device detonation-site
  say engineering "The device is at its target and starting up. We've done it."
  complete 15
  complete 17

# Mission passed: the win 10 s later (no announcement in J8)
on all-completed
  timer pass 10
on timer pass
  win

# The other sides' AI: orders on a clock (the Klingons' own attack orders
# are left out: the player gives those)
# Federation raids on the Klingons: one of eight attacks every 30 s, the first at 90 s
every timer fed-raid-kli
  timer fed-raid-kli 30
  random fed-raid-kli-1 fed-raid-kli-2 fed-raid-kli-3 fed-raid-kli-4 fed-raid-kli-5 fed-raid-kli-6 fed-raid-kli-7 fed-raid-kli-8
every signal fed-raid-kli-1
  ai federation fed-assault-kli-random1
every signal fed-raid-kli-2
  ai federation fed-assault-kli-random2
every signal fed-raid-kli-3
  ai federation fed-assault-kli-random3
every signal fed-raid-kli-4
  ai federation fed-assault-kli-random4
every signal fed-raid-kli-5
  ai federation fed-assault-kli-random5
every signal fed-raid-kli-6
  ai federation fed-assault-kli-random6
every signal fed-raid-kli-7
  ai federation fed-assault-kli-random7
every signal fed-raid-kli-8
  ai federation fed-assault-kli-random8

# Federation raids on the Romulans: one of eight attacks every 30 s, the first at 90 s
every timer fed-raid-rom
  timer fed-raid-rom 30
  random fed-raid-rom-1 fed-raid-rom-2 fed-raid-rom-3 fed-raid-rom-4 fed-raid-rom-5 fed-raid-rom-6 fed-raid-rom-7 fed-raid-rom-8
every signal fed-raid-rom-1
  ai federation fed-assault-rom-random1
every signal fed-raid-rom-2
  ai federation fed-assault-rom-random2
every signal fed-raid-rom-3
  ai federation fed-assault-rom-random3
every signal fed-raid-rom-4
  ai federation fed-assault-rom-random4
every signal fed-raid-rom-5
  ai federation fed-assault-rom-random5
every signal fed-raid-rom-6
  ai federation fed-assault-rom-random6
every signal fed-raid-rom-7
  ai federation fed-assault-rom-random7
every signal fed-raid-rom-8
  ai federation fed-assault-rom-random8

# Federation orders on a clock
on timer federation-colony-protection-relay
  ai federation federation-colony-protection-one
  ai federation federation-colony-protection-two
  ai federation federation-colony-protection-three
  ai federation federation-colony-protection-four

# Romulan raids on the Federation: one of eight attacks every 30 s, the first at 90 s
every timer rom-raid-fed
  timer rom-raid-fed 30
  random rom-raid-fed-1 rom-raid-fed-2 rom-raid-fed-3 rom-raid-fed-4 rom-raid-fed-5 rom-raid-fed-6 rom-raid-fed-7 rom-raid-fed-8
every signal rom-raid-fed-1
  ai romulan rom-assault-fed-random1
every signal rom-raid-fed-2
  ai romulan rom-assault-fed-random2
every signal rom-raid-fed-3
  ai romulan rom-assault-fed-random3
every signal rom-raid-fed-4
  ai romulan rom-assault-fed-random4
every signal rom-raid-fed-5
  ai romulan rom-assault-fed-random5
every signal rom-raid-fed-6
  ai romulan rom-assault-fed-random6
every signal rom-raid-fed-7
  ai romulan rom-assault-fed-random7
every signal rom-raid-fed-8
  ai romulan rom-assault-fed-random8

# Romulan raids on the Klingons: one of eight attacks every 30 s, the first at 90 s
every timer rom-raid-kli
  timer rom-raid-kli 30
  random rom-raid-kli-1 rom-raid-kli-2 rom-raid-kli-3 rom-raid-kli-4 rom-raid-kli-5 rom-raid-kli-6 rom-raid-kli-7 rom-raid-kli-8
every signal rom-raid-kli-1
  ai romulan rom-assault-kli-random1
every signal rom-raid-kli-2
  ai romulan rom-assault-kli-random2
every signal rom-raid-kli-3
  ai romulan rom-assault-kli-random3
every signal rom-raid-kli-4
  ai romulan rom-assault-kli-random4
every signal rom-raid-kli-5
  ai romulan rom-assault-kli-random5
every signal rom-raid-kli-6
  ai romulan rom-assault-kli-random6
every signal rom-raid-kli-7
  ai romulan rom-assault-kli-random7
every signal rom-raid-kli-8
  ai romulan rom-assault-kli-random8

# Metar raids on the Federation: one of eight attacks every 60 s, the first at 120 s
every timer met-raid-fed
  timer met-raid-fed 60
  random met-raid-fed-1 met-raid-fed-2 met-raid-fed-3 met-raid-fed-4 met-raid-fed-5 met-raid-fed-6 met-raid-fed-7 met-raid-fed-8
every signal met-raid-fed-1
  ai metar metar-assault-fed-random1
every signal met-raid-fed-2
  ai metar metar-assault-fed-random2
every signal met-raid-fed-3
  ai metar metar-assault-fed-random3
every signal met-raid-fed-4
  ai metar metar-assault-fed-random4
every signal met-raid-fed-5
  ai metar metar-assault-fed-random5
every signal met-raid-fed-6
  ai metar metar-assault-fed-random6
every signal met-raid-fed-7
  ai metar metar-assault-fed-random7
every signal met-raid-fed-8
  ai metar metar-assault-fed-random8

# Metar raids on the Klingons: one of eight attacks every 60 s, the first at 120 s
every timer met-raid-kli
  timer met-raid-kli 60
  random met-raid-kli-1 met-raid-kli-2 met-raid-kli-3 met-raid-kli-4 met-raid-kli-5 met-raid-kli-6 met-raid-kli-7 met-raid-kli-8
every signal met-raid-kli-1
  ai metar metar-assault-kli-random1
every signal met-raid-kli-2
  ai metar metar-assault-kli-random2
every signal met-raid-kli-3
  ai metar metar-assault-kli-random3
every signal met-raid-kli-4
  ai metar metar-assault-kli-random4
every signal met-raid-kli-5
  ai metar metar-assault-kli-random5
every signal met-raid-kli-6
  ai metar metar-assault-kli-random6
every signal met-raid-kli-7
  ai metar metar-assault-kli-random7
every signal met-raid-kli-8
  ai metar metar-assault-kli-random8

# Metar raids on the Romulans: one of eight attacks every 60 s, the first at 120 s
every timer met-raid-rom
  timer met-raid-rom 60
  random met-raid-rom-1 met-raid-rom-2 met-raid-rom-3 met-raid-rom-4 met-raid-rom-5 met-raid-rom-6 met-raid-rom-7 met-raid-rom-8
every signal met-raid-rom-1
  ai metar metar-assault-rom-random1
every signal met-raid-rom-2
  ai metar metar-assault-rom-random2
every signal met-raid-rom-3
  ai metar metar-assault-rom-random3
every signal met-raid-rom-4
  ai metar metar-assault-rom-random4
every signal met-raid-rom-5
  ai metar metar-assault-rom-random5
every signal met-raid-rom-6
  ai metar metar-assault-rom-random6
every signal met-raid-rom-7
  ai metar metar-assault-rom-random7
every signal met-raid-rom-8
  ai metar metar-assault-rom-random8

# Metar orders on a clock
on timer metar-colony-protection-relay-one
  ai metar metar-colony-protection-one
  ai metar metar-colony-protection-two
  ai metar metar-colony-protection-three
  ai metar metar-colony-protection-four
on timer metar-colony-protection-relay-two
  ai metar metar-colony-protection-five
  ai metar metar-colony-protection-six
  ai metar metar-colony-protection-seven
  ai metar metar-colony-protection-eight
on timer metar-colony-protection-relay-three
  ai metar metar-colony-protection-nine
  ai metar metar-colony-protection-ten
  ai metar metar-colony-protection-eleven
  ai metar metar-colony-protection-twelve
on timer metar-colony-protection-relay-four
  ai metar metar-colony-protection-thirteen
on timer metar-federation-attack
  ai metar metar-federation-assault-one
  ai metar metar-federation-assault-two
on timer metar-klingon-attack
  ai metar metar-klingon-assault-one
  ai metar metar-klingon-assault-two
on timer metar-romulan-attack
  ai metar metar-romulan-assault-one
  ai metar metar-romulan-assault-two

# Not expressible yet:
# - The arrival detector filters by unit class (the device's); written as
#   our unit kind anomaly-device.
# - The rivals' devices: their research, building and route orders (60 s
#   after their research) happen on their side; there's no event for a
#   rival's research.
# - Objective 15 (protect the vehicle yard) is met only when the device
#   arrives, together with 17; the game words it as its own objective but
#   checks nothing else.

# --- K14 as K ----------------------------------------
# K14 as the Klingons, rewritten from the game's own mission graph (Klingon slot, with
# the Taubat (3) and Metar (5) attack clocks). The containment device (CFD) starts in
# place at 154:237.
mission K14 K
site colony 82 181
site device 154 237
site metar-core 330 189
objective 12
objective 14

# Opening: Taubat raids from 30 s (one every 30 s), Metar raids from 2 minutes (one
# every 100 s), named Metar assaults at 1, 2 and 3 minutes.
on start
  timer taubat-raids 30
  timer metar-raids 120
  timer metar-assault-1 60
  timer metar-assault-3 120
  timer metar-assault-2 180
on timer metar-assault-1
  ai metar met-assault-klingon-1
on timer metar-assault-3
  ai metar met-assault-klingon-3
on timer metar-assault-2
  ai metar met-assault-klingon-2

# Objectives 12 and 14: scanning the Metar core settles both (the device has held).
on cell-scanned metar-core
  say science "These readings are nonsense... unless the aliens are building an anomaly generator."
  complete 12
  complete 14
  timer pass-message 10

# Mission pass: message 10 s after the scan, win 10 s after that.
on timer pass-message
  say intelligence "Every objective is met. Well done."
  timer pass 10
on timer pass
  win

# Mission fail: the colony hub or the device; message now, verdict at 10 s, end at
# 20 s. (A pass already under way still wins if it gets there first, as in the game.)
on count colony-hub == 0
  fail 13 20
  say intelligence "Our hub has been destroyed."
  signal failing
on count containment-device == 0
  fail 15 20
  say security "The device for the containment field has been destroyed by the Metar."
  signal failing
on signal failing
  timer fail-message 10
on timer fail-message
  say intelligence "We have not achieved our objectives."

# Raids: one of eight Taubat assaults every 30 s, one of eight Metar assaults every 100 s.
every timer taubat-raids
  random taubat-raid-1 taubat-raid-2 taubat-raid-3 taubat-raid-4 taubat-raid-5 taubat-raid-6 taubat-raid-7 taubat-raid-8
  timer taubat-raids 30
every signal taubat-raid-1
  ai taubat taubat-assault-kli-random1
every signal taubat-raid-2
  ai taubat taubat-assault-kli-random2
every signal taubat-raid-3
  ai taubat taubat-assault-kli-random3
every signal taubat-raid-4
  ai taubat taubat-assault-kli-random4
every signal taubat-raid-5
  ai taubat taubat-assault-kli-random5
every signal taubat-raid-6
  ai taubat taubat-assault-kli-random6
every signal taubat-raid-7
  ai taubat taubat-assault-kli-random7
every signal taubat-raid-8
  ai taubat taubat-assault-kli-random8
every timer metar-raids
  random metar-raid-1 metar-raid-2 metar-raid-3 metar-raid-4 metar-raid-5 metar-raid-6 metar-raid-7 metar-raid-8
  timer metar-raids 100
every signal metar-raid-1
  ai metar metar-assault-kli-random1
every signal metar-raid-2
  ai metar metar-assault-kli-random2
every signal metar-raid-3
  ai metar metar-assault-kli-random3
every signal metar-raid-4
  ai metar metar-assault-kli-random4
every signal metar-raid-5
  ai metar metar-assault-kli-random5
every signal metar-raid-6
  ai metar metar-assault-kli-random6
every signal metar-raid-7
  ai metar metar-assault-kli-random7
every signal metar-raid-8
  ai metar metar-assault-kli-random8

# Not expressible yet:
# - A Metar order to attack the device exists but nothing in the graph starts it; the
#   Klingons' four unnamed patrols and the Taubat's build orders run without the script.
# - The pass and fail lines go to every side's mail, not only the Klingons'.

# --- J9 as K ----------------------------------------
# J9 as the Klingons (slot 1), from the game's own mission graph.
# Scan the Metar temple for the sentient anomaly device, research it, and
# capture one of the Metar starships.
mission J9 K
site metar-temple 264 303
objective 13
objective 15

# Fail criteria, the game's way: the cause at once, the end 10 s later (no
# word to the other sides in J9). Our hub (14); all three Metar starships
# destroyed (16; the game's message calls them colony cores). Fail 17 (a
# rival got the technology while we were scanning) is with the scan below.
on destroyed colony-hub
  fail 14 10
  say command "Colony hub lost. The mission has failed."
on count metar-starship == 0
  fail 16 10
  say command "The Metar colony cores are all gone. The mission has failed."

# Start: the other sides' attack clocks
on start
  timer fed-raid-kli 60
  timer fed-raid-rom 60
  timer rom-raid-fed 60
  timer rom-raid-kli 60
  timer met-raid-fed 220
  timer met-raid-kli 220
  timer met-raid-rom 220

# Objective 13: scan the temple (10 s before the officer reports, then held
# a minute); the technology is beamed home and its research opens; the
# research done meets the objective. The game's watch is "the device's block
# scanned by us" (any of our scanning units nearby), on the group
# Anomaly_Device: a cell scan.
every scan-started cell anomaly-device
  say engineering "Searching the temple for the device's technology."
every scan-lost cell anomaly-device
  say engineering "We lost the scan. Have another try."
on scanned cell anomaly-device
  say engineering "The technology is beamed home to our hub for research."
  research anomaly-device
# The race for the device: the first side to finish takes the technology. A
# rival finishing while ours runs beats us (fail 17); once a rival has
# finished, our later scan is futile (no technology, no research). The
# computer's sides only win it if their own units happen to scan the block.
on beaten anomaly-device
  fail 17 10
  say engineering "The technology is gone from here; the Federation or the Romulans must have taken it."
every futile anomaly-device
  say engineering "That scan was useless."
on researched anomaly-device
  say science "Our researchers have worked out how to use the device."
  complete 13
  arm starship-watch

# Objective 15: holding a Metar starship; the game starts watching for it
# only once the research is done
on captured metar-starship as starship-watch dormant
  say engineering "The starship is ours."
  complete 15

# Mission passed: the win 10 s later (no announcement in J9)
on all-completed
  timer pass 10
on timer pass
  win

# The other sides' AI: attacks on a clock (the Klingons' own attack orders
# are left out: the player gives those)
# Federation raids on the Klingons: one of eight attacks every 30 s, the first at 60 s
every timer fed-raid-kli
  timer fed-raid-kli 30
  random fed-raid-kli-1 fed-raid-kli-2 fed-raid-kli-3 fed-raid-kli-4 fed-raid-kli-5 fed-raid-kli-6 fed-raid-kli-7 fed-raid-kli-8
every signal fed-raid-kli-1
  ai federation fed-assault-kli-random1
every signal fed-raid-kli-2
  ai federation fed-assault-kli-random2
every signal fed-raid-kli-3
  ai federation fed-assault-kli-random3
every signal fed-raid-kli-4
  ai federation fed-assault-kli-random4
every signal fed-raid-kli-5
  ai federation fed-assault-kli-random5
every signal fed-raid-kli-6
)",
R"(  ai federation fed-assault-kli-random6
every signal fed-raid-kli-7
  ai federation fed-assault-kli-random7
every signal fed-raid-kli-8
  ai federation fed-assault-kli-random8

# Federation raids on the Romulans: one of eight attacks every 30 s, the first at 60 s
every timer fed-raid-rom
  timer fed-raid-rom 30
  random fed-raid-rom-1 fed-raid-rom-2 fed-raid-rom-3 fed-raid-rom-4 fed-raid-rom-5 fed-raid-rom-6 fed-raid-rom-7 fed-raid-rom-8
every signal fed-raid-rom-1
  ai federation fed-assault-rom-random1
every signal fed-raid-rom-2
  ai federation fed-assault-rom-random2
every signal fed-raid-rom-3
  ai federation fed-assault-rom-random3
every signal fed-raid-rom-4
  ai federation fed-assault-rom-random4
every signal fed-raid-rom-5
  ai federation fed-assault-rom-random5
every signal fed-raid-rom-6
  ai federation fed-assault-rom-random6
every signal fed-raid-rom-7
  ai federation fed-assault-rom-random7
every signal fed-raid-rom-8
  ai federation fed-assault-rom-random8

# Romulan raids on the Federation: one of eight attacks every 30 s, the first at 60 s
every timer rom-raid-fed
  timer rom-raid-fed 30
  random rom-raid-fed-1 rom-raid-fed-2 rom-raid-fed-3 rom-raid-fed-4 rom-raid-fed-5 rom-raid-fed-6 rom-raid-fed-7 rom-raid-fed-8
every signal rom-raid-fed-1
  ai romulan rom-assault-fed-random1
every signal rom-raid-fed-2
  ai romulan rom-assault-fed-random2
every signal rom-raid-fed-3
  ai romulan rom-assault-fed-random3
every signal rom-raid-fed-4
  ai romulan rom-assault-fed-random4
every signal rom-raid-fed-5
  ai romulan rom-assault-fed-random5
every signal rom-raid-fed-6
  ai romulan rom-assault-fed-random6
every signal rom-raid-fed-7
  ai romulan rom-assault-fed-random7
every signal rom-raid-fed-8
  ai romulan rom-assault-fed-random8

# Romulan raids on the Klingons: one of eight attacks every 30 s, the first at 60 s
every timer rom-raid-kli
  timer rom-raid-kli 30
  random rom-raid-kli-1 rom-raid-kli-2 rom-raid-kli-3 rom-raid-kli-4 rom-raid-kli-5 rom-raid-kli-6 rom-raid-kli-7 rom-raid-kli-8
every signal rom-raid-kli-1
  ai romulan rom-assault-kli-random1
every signal rom-raid-kli-2
  ai romulan rom-assault-kli-random2
every signal rom-raid-kli-3
  ai romulan rom-assault-kli-random3
every signal rom-raid-kli-4
  ai romulan rom-assault-kli-random4
every signal rom-raid-kli-5
  ai romulan rom-assault-kli-random5
every signal rom-raid-kli-6
  ai romulan rom-assault-kli-random6
every signal rom-raid-kli-7
  ai romulan rom-assault-kli-random7
every signal rom-raid-kli-8
  ai romulan rom-assault-kli-random8

# Metar raids on the Federation: one of eight attacks every 100 s, the first at 220 s
every timer met-raid-fed
  timer met-raid-fed 100
  random met-raid-fed-1 met-raid-fed-2 met-raid-fed-3 met-raid-fed-4 met-raid-fed-5 met-raid-fed-6 met-raid-fed-7 met-raid-fed-8
every signal met-raid-fed-1
  ai metar metar-assault-fed-random1
every signal met-raid-fed-2
  ai metar metar-assault-fed-random2
every signal met-raid-fed-3
  ai metar metar-assault-fed-random3
every signal met-raid-fed-4
  ai metar metar-assault-fed-random4
every signal met-raid-fed-5
  ai metar metar-assault-fed-random5
every signal met-raid-fed-6
  ai metar metar-assault-fed-random6
every signal met-raid-fed-7
  ai metar metar-assault-fed-random7
every signal met-raid-fed-8
  ai metar metar-assault-fed-random8

# Metar raids on the Klingons: one of eight attacks every 100 s, the first at 220 s
every timer met-raid-kli
  timer met-raid-kli 100
  random met-raid-kli-1 met-raid-kli-2 met-raid-kli-3 met-raid-kli-4 met-raid-kli-5 met-raid-kli-6 met-raid-kli-7 met-raid-kli-8
every signal met-raid-kli-1
  ai metar metar-assault-kli-random1
every signal met-raid-kli-2
  ai metar metar-assault-kli-random2
every signal met-raid-kli-3
  ai metar metar-assault-kli-random3
every signal met-raid-kli-4
  ai metar metar-assault-kli-random4
every signal met-raid-kli-5
  ai metar metar-assault-kli-random5
every signal met-raid-kli-6
  ai metar metar-assault-kli-random6
every signal met-raid-kli-7
  ai metar metar-assault-kli-random7
every signal met-raid-kli-8
  ai metar metar-assault-kli-random8

# Metar raids on the Romulans: one of eight attacks every 100 s, the first at 220 s
every timer met-raid-rom
  timer met-raid-rom 100
  random met-raid-rom-1 met-raid-rom-2 met-raid-rom-3 met-raid-rom-4 met-raid-rom-5 met-raid-rom-6 met-raid-rom-7 met-raid-rom-8
every signal met-raid-rom-1
  ai metar metar-assault-rom-random1
every signal met-raid-rom-2
  ai metar metar-assault-rom-random2
every signal met-raid-rom-3
  ai metar metar-assault-rom-random3
every signal met-raid-rom-4
  ai metar metar-assault-rom-random4
every signal met-raid-rom-5
  ai metar metar-assault-rom-random5
every signal met-raid-rom-6
  ai metar metar-assault-rom-random6
every signal met-raid-rom-7
  ai metar metar-assault-rom-random7
every signal met-raid-rom-8
  ai metar metar-assault-rom-random8

# Not expressible yet:
# - The game's capture watch counts starships held, so one captured before
#   the research and still held when the watch starts counts at once; here
#   only a capture made after the research does (the event is the capture).
# - The AI sides' own research and captures (no events for rivals).

# --- C10 as K ----------------------------------------
# C10 "Bonus Klingon Mission" (Klingon, slot 1; map coverd1, colony at 344:431). Rewritten
# from the game's own graph (S/ghidra/scripts_out/C10.json). A Romulan colony (slot 2,
# hub 144:359) and a Taubat settlement (slot 3, round 200:339-236:411) both raid the
# Klingons every 3 minutes, and a Romulan Bird of Prey circles the map. Objectives: wipe
# out both colonies (75) and destroy the Bird of Prey (76).
mission C10 K
site colony 344 431
site romulan-colony 144 359
site taubat-settlement 216 375
objective 75
objective 76

# --- The raids: every 180 s one of the side's assault orders, at random ---
on start
  timer romulan-raid 180
  timer taubat-raid 180
  move rom-bop 146 369 346 429 294 365 146 369 loop   # the Romulan Bird of Prey's patrol
every timer romulan-raid
  random rom-raid-1 rom-raid-2 rom-raid-3 rom-raid-4
  timer romulan-raid 180
every signal rom-raid-1
  ai romulan rom-assault-kli-random1
every signal rom-raid-2
  ai romulan rom-assault-kli-random2
every signal rom-raid-3
  ai romulan rom-assault-kli-random3
every signal rom-raid-4
  ai romulan rom-assault-kli-random4
every timer taubat-raid
  random tau-raid-1 tau-raid-2 tau-raid-3 tau-raid-1 tau-raid-2 tau-raid-3 tau-raid-1 tau-raid-2
  timer taubat-raid 180
every signal tau-raid-1
  ai taubat tau-assault-kli-random1
every signal tau-raid-2
  ai taubat tau-assault-kli-random2
every signal tau-raid-3
  ai taubat tau-assault-kli-random3

# --- The Bird of Prey ---
on destroyed rom-bop
  say operations "Even Kahless would salute such a triumph. Placing this command in your hands was a wise choice by the council. This day will live on in our songs!"
  complete 76

# --- The Romulan colony: every one of its nine kinds of building gone ---
on count romulan-hub == 0
  signal romulan-kind-gone
on count romulan-storage-facility == 0
  signal romulan-kind-gone
on count romulan-construction-yard == 0
  signal romulan-kind-gone
on count romulan-vehicle-yard == 0
  signal romulan-kind-gone
on count romulan-resource-processor == 0
  signal romulan-kind-gone
on count romulan-accommodation == 0
  signal romulan-kind-gone
on count romulan-power-generator == 0
  signal romulan-kind-gone
on count romulan-defensive-weapon == 0
  signal romulan-kind-gone
on count romulan-defensive-artillery == 0
  signal romulan-kind-gone
on signal romulan-kind-gone*9
  say intelligence "Sir, the Romulan colony is torn apart! A glorious fight!"
  signal colony-razed

# --- The Taubat: hangars, houses and guns all gone ---
on count taubat-hangar == 0
  signal taubat-kind-gone
on count taubat-house == 0
  signal taubat-kind-gone
on count taubat-gun == 0
  signal taubat-kind-gone
on signal taubat-kind-gone*3
  say intelligence "Sir, every Taubat structure has fallen to our warriors!"
  signal colony-razed

# --- Both colonies razed: objective 75; then the pass ---
on signal colony-razed*2
  complete 75
on all-completed
  timer pass-message 10
  timer pass 20
on timer pass-message
  say operations "You are done here, Commander. By order of the High Council, proceed to Tancred IV; our warriors gather there to strike at the Metar. Today honour is ours. Qapla'!"
on timer pass
  win

# --- The fail: the colony hub destroyed (said at once, the failure 10 s later, over at 20 s) ---
on destroyed colony-hub
  fail 77 20
  say command "We have lost the colony hub."
  timer fail-message 10
on timer fail-message
  say command "You have not achieved your objectives."

# Not expressible yet:
# - The Bird of Prey's flight height (12000) on its loop.
# - The Taubat also have four patrol orders of their tanks (type 111) the game starts by
#   itself.

# --- R1 as R ----------------------------------------
# R1 as the Romulans, rewritten from the game's own mission graph (STNW.exe).
mission R1 R
site taubat-temple 256 151
site ruins-nw 280 159
site ruins-ne 280 183
site ruins-sw 304 159
site ruins-se 304 183
site ruins 292 171
site rival-base 160 327
zone storm-field 292 220 300 228
objective 13
objective 16
objective 17
objective 19 optional
objective 20 hidden optional

# Opening: the staff's greeting a second after the start; the raid clocks
# start (Taubat and the rival Romulans each begin random raids at 6 min,
# then every 4 min); the rivals open with an assault and guard their colony.
# A storm flashes over a small field east of the ruins all mission long.
on start
  say operations "Greetings from the command staff, commander. We have every confidence in you."
  ai rival riv-rom-initial-assault
  ai rival rival-romulans-colony-protection
  timer taubat-raids 360
  timer rival-raids 360
  storm storm-field

# Objective 13: the first vehicle yard, construction yard, power generator,
# resource processor, transporter pad and accommodation block. (Each one's
# own "finished" line is the generic rule's.)
on built-all vehicle-yard construction-yard power-generator resource-processor transporter-pad accommodation
  say engineering "Every building we needed is now finished."
  complete 13

# Objective 16: the rivals' fighting units down from twelve to nine.
on count rival-fighting-units <= 9
  say intelligence "Three of the rival Romulans' cloaked tanks are destroyed, sir."
  complete 16

# Objective 17: whichever of the temple or the ruins is sighted first gets
# its line and its research topic (the other then stays silent); finishing it
# meets the objective.
on seen taubat-temple as first-find
  disarm first-find
  say intelligence "An alien building at {site:taubat-temple}. Its purpose is worth researching."
  research temple
on seen ruins-nw
  signal ruins-sighted
on seen ruins-ne
  signal ruins-sighted
on seen ruins-sw
  signal ruins-sighted
on seen ruins-se
  signal ruins-sighted
on signal ruins-sighted as first-find
  disarm first-find
  say intelligence "Strange ruins at {site:ruins}. We should research what they were for."
  research portal
on researched temple
  say science "The Taubat temple seems to feed a stasis field lying under the four groups of ruins to the north."
  complete 17
on researched portal
  say science "Under the four clusters of ruins there's a stasis field, drawing power from the Taubat temple further south."
  complete 17

# Optional objectives 19 and 20: find the rival base, destroy its hub and
# power generator; then its research topic (and hidden objective 20) opens.
on seen rival-base
  say intelligence "Sir, our rivals' base is at {site:rival-base}."
  signal rival-base-step
on destroyed rival-colony-hub
  say intelligence "The rival Romulans' colony hub is destroyed."
  signal rival-base-down
on destroyed rival-power-generator
  say intelligence "The rival Romulans' power generator is destroyed."
  signal rival-base-down
on signal rival-base-down*2
  say intelligence "With the rival base gone, sir, we should find out who was behind it."
  complete 19
  signal rival-base-step
on signal rival-base-step*2
  reveal 20
  research rival-base
on researched rival-base
  say science "This site has all the marks of one of Admiral Narok's operations."
  complete 20
on count rival-fighting-units <= 3
  stop rival-raid
on count rival-fighting-units <= 0
  say intelligence "Every one of the rival Romulans' tanks is destroyed."

# Scans that find nothing.
every scanned-nothing science
  say science "Ordinary science scans show nothing unusual, sir."
every scanned-nothing strong-science
  say science "A detailed scan, sir, but it's all as I'd expect."
every scanned-nothing medical
  say medical "Ordinary medical scans find nothing abnormal, sir."
every scanned-nothing strong-medical
  say medical "Full diagnostics turn up nothing of interest, sir."
every scanned-nothing engineering
  say engineering "Structure and systems scan within normal limits, sir."
every scanned-nothing strong-engineering
  say engineering "Not much here of interest to engineering, sir."

# The raids: after the 6-minute wait, every 4 minutes one of eight
# assaults at random. The rivals' stop once they're down to three units.
on timer taubat-raids
  timer taubat-raid 240
every timer taubat-raid
  random taubat-raid-1 taubat-raid-2 taubat-raid-3 taubat-raid-4 taubat-raid-5 taubat-raid-6 taubat-raid-7 taubat-raid-8
  timer taubat-raid 240
every signal taubat-raid-1
  ai taubat taubat-assault-rom-random1
every signal taubat-raid-2
  ai taubat taubat-assault-rom-random2
every signal taubat-raid-3
  ai taubat taubat-assault-rom-random3
every signal taubat-raid-4
  ai taubat taubat-assault-rom-random4
every signal taubat-raid-5
  ai taubat taubat-assault-rom-random5
every signal taubat-raid-6
  ai taubat taubat-assault-rom-random6
every signal taubat-raid-7
  ai taubat taubat-assault-rom-random7
every signal taubat-raid-8
  ai taubat taubat-assault-rom-random8
on timer rival-raids
  timer rival-raid 240
every timer rival-raid
  random rival-raid-1 rival-raid-2 rival-raid-3 rival-raid-4 rival-raid-5 rival-raid-6 rival-raid-7 rival-raid-8
  timer rival-raid 240
every signal rival-raid-1
  ai rival riv-rom-assault-rom-random1
every signal rival-raid-2
  ai rival riv-rom-assault-rom-random2
every signal rival-raid-3
  ai rival riv-rom-assault-rom-random3
every signal rival-raid-4
  ai rival riv-rom-assault-rom-random4
every signal rival-raid-5
  ai rival riv-rom-assault-rom-random5
every signal rival-raid-6
  ai rival riv-rom-assault-rom-random6
every signal rival-raid-7
  ai rival riv-rom-assault-rom-random7
every signal rival-raid-8
  ai rival riv-rom-assault-rom-random8

# Mission fail: the colony hub. The cause now, "failed" at 10 s, the end at 20 s.
on destroyed colony-hub
  fail 21 20
  say operations "The colony hub is destroyed. The mission has failed."
  timer fail-message 10
on timer fail-message
)",
R"(  say operations "We have failed."

# The pass: the objective counter fills, the message 10 s later, the win at 20 s.
on all-completed
  timer mission-pass 10
  timer mission-won 20
on timer mission-pass
  say operations "Mission complete."
on timer mission-won
  win

# Not expressible yet:
# - The game's pass counter counts the first sighting (temple or ruins) as
#   one of its four steps; here the pass waits for the objectives instead.
# - Each building's first-completion line (only the first of each type) is
#   left to the generic "every built" rule, which speaks for every one.
# - The game's text gives the sites as <143:260> (temple), <302:162> (ruins)
#   and <165:323> (rival base); the sites here are the monitored cells
#   ("ruins" is the middle of the four).

# --- R2 as R ----------------------------------------
# R2 as the Romulans, rewritten from the game's own mission graph (STNW.exe).
mission R2 R
site colony 254 347
site taubat-temple 264 159
site taubat-hangar-one 264 167
site taubat-hangar-two 264 175
zone colony-approach 236 335 272 359
objective 13
objective 14
objective 15
objective 16 optional
objective 17 optional

# Opening: the greeting after a second; the Orions (in the Taubat colony)
# send two assaults at once, guard their colony from 10 s, and start random
# raids at 6 min, then one every 4 min. Our Bird of Prey flies its patrol
# loop all mission long.
on start
  say operations "Welcome to Taphet. All is in order."
  move rom-bop 344 295 236 423 204 339 344 295 loop
  ai orion orion-romulan-colony-assualt-one
  ai orion orion-romulan-colony-assualt-two
  timer orion-guard 10
  timer orion-raids 360
on timer orion-guard
  ai orion orion-colony-protection-one
  ai orion orion-colony-protection-two
  ai orion orion-colony-protection-three
# The first Taubat-side unit inside the rectangle round our colony.
on inside taubat colony-approach
  say intelligence "Enemy units are closing on our colony, sir."

# Objective 13: the first sickbay, vehicle yard, construction yard and
# science block. (Each one's own line is the generic rule's.)
on built-all sickbay vehicle-yard construction-yard science-station
  say engineering "Every building we needed is finished, sir."
  complete 13
  signal backdoor

# Sightings.
on seen taubat-temple
  say intelligence "A Taubat temple at {site:taubat-temple}."
on seen taubat-hangar-one
  say intelligence "A Taubat vehicle factory at {site:taubat-hangar-one}."
on seen taubat-hangar-two
  say intelligence "A Taubat vehicle factory at {site:taubat-hangar-two}."

# Optional objective 16: hold the temple 10 s; its mineral transporter
# sends 100 T polycomposite, 100 T transaluminium and 1000 KG duranium.
# Losing it before then is reported each time.
on entered taubat-temple
  say security "The temple is ours. It holds technology for transporting minerals; we're switching it on."
every capture-lost taubat-temple
  say security "The Taubat temple is lost to us. We should go back in with more troops."
on captured taubat-temple
  say engineering "The minerals are at our base now. We should pull out of the temple and destroy it."
  complete 16
  give polycomposite 100
  give transaluminium 100
  give duranium 1000

# Optional objective 17: hold the first vehicle factory 10 s (a loss
# before then reported each time).
on entered taubat-hangar-one
  say security "The vehicle factory is ours. Looking for the Taubat prisoners."
every capture-lost taubat-hangar-one
  say security "The Taubat vehicle factory is lost to us. We should go back in with more troops."
on captured taubat-hangar-one
  say security "The Taubat prisoners are safe at our base. We should pull out of the factory and destroy it."
  complete 17

# Objective 14: five steps - the temple, both factories (each reported once
# it has been gone 10 s), the houses and every fighting unit (the side's
# tanks, artillery and Orion pirates; watched from the first factory's
# report on).
on destroyed taubat-temple
  timer temple-gone 10
on timer temple-gone
  say intelligence "The Taubat temple is destroyed."
  signal orion-step
on destroyed taubat-hangar-one
  timer hangar-one-gone 10
on timer hangar-one-gone
  say intelligence "One of the vehicle factories the Orions hold is destroyed."
  signal orion-step
  arm orion-units
on destroyed taubat-hangar-two
  timer hangar-two-gone 10
on timer hangar-two-gone
  say "Incoming message" "Curse you! You will pay for meddling in this."
  say communications "That was the Orions. An idle threat, I expect."
  signal orion-step
  arm orion-units
on count taubat-small-buildings <= 0   # the houses
  say intelligence "The houses the Orions hold are destroyed."
  signal orion-step
on count taubat-fighting-units <= 0 as orion-units dormant
  say intelligence "Every Orion and Taubat unit is destroyed."
  signal orion-step
on signal orion-step*5
  say intelligence "The Orions are utterly beaten, sir."
  complete 14
  signal backdoor

# Objective 15: 50 T of dilithium beamed to the starship, as soon as the
# stock reaches 250 T - or, once objectives 13 and 14 are met, at 50 T (a
# second check, asleep until then). The beam-up happens once.
on stock dilithium 250 as rich-check
  signal beam-up
on signal backdoor*2 as low-check-wake
  arm low-check
on stock dilithium 50 as low-check dormant
  signal beam-up
on signal beam-up
  disarm rich-check
  disarm low-check
  disarm low-check-wake
  say communications "Beaming 50 units of dilithium up to the starship."
  take dilithium 50
  complete 15

# Scans that find nothing.
every scanned-nothing science
  say science "Ordinary science scans show nothing unusual, sir."
every scanned-nothing strong-science
  say science "A detailed scan, sir, but it's all as I'd expect."
every scanned-nothing medical
  say medical "Ordinary medical scans find nothing abnormal, sir."
every scanned-nothing strong-medical
  say medical "Full diagnostics turn up nothing of interest."
every scanned-nothing engineering
  say engineering "Structure and systems scan within normal limits, sir."
every scanned-nothing strong-engineering
  say engineering "Nothing here that would interest you, sir."

# The Orions' raids: every 4 minutes after the first 6, one of eight at random.
on timer orion-raids
  timer orion-raid 240
every timer orion-raid
  random orion-raid-1 orion-raid-2 orion-raid-3 orion-raid-4 orion-raid-5 orion-raid-6 orion-raid-7 orion-raid-8
  timer orion-raid 240
every signal orion-raid-1
  ai orion orion-assault-rom-random1
every signal orion-raid-2
  ai orion orion-assault-rom-random2
every signal orion-raid-3
  ai orion orion-assault-rom-random3
every signal orion-raid-4
  ai orion orion-assault-rom-random4
every signal orion-raid-5
  ai orion orion-assault-rom-random5
every signal orion-raid-6
  ai orion orion-assault-rom-random6
every signal orion-raid-7
  ai orion orion-assault-rom-random7
every signal orion-raid-8
  ai orion orion-assault-rom-random8

# Mission fail: the colony hub. The cause now, "failed" at 10 s, the end at 20 s.
on destroyed colony-hub
  fail 18 20
  say operations "The colony hub is destroyed."
  timer fail-message 10
on timer fail-message
  say operations "We have failed."

# The pass: the counter (objectives 13, 14, 15) fills, the message 10 s
# later, the win at 20 s.
on all-completed
  timer mission-pass 10
  timer mission-won 20
on timer mission-pass
  say operations "Mission complete."
on timer mission-won
  win

# Not expressible yet:
# - Each building's first-completion line is left to the generic rule.

# --- J1 as R ----------------------------------------
# J1 "Into the Arena", played as the Romulans (slot 2).
# Rewritten from the game's own mission graph (scripts_out/J1.json).
mission J1 R
site romulan-colony 130 397
site taubat-colony 298 205
site temple 394 309
objective 21
objective 23
objective 25 hidden

# --- The other sides' attacks on the colony -------------------------------
# From 300 a random Federation, Klingon and Taubat assault every 180 s
# each; Federation assaults at 1200 s and 1260 s; and once we field four
# armed APCs the Klingons send two assaults.
on start
  timer fed-assault-romulans-1 1200
  timer fed-assault-romulans-2 1260
  timer fed-raids-on-rom 300
  timer kli-raids-on-rom 300
  timer tau-raids-on-rom 300
on timer fed-assault-romulans-1
  ai federation fed-assault-romulans-1
on timer fed-assault-romulans-2
  ai federation fed-assault-romulans-2
every timer fed-raids-on-rom
  random fed-assault-rom-random1 fed-assault-rom-random2 fed-assault-rom-random3 fed-assault-rom-random4 fed-assault-rom-random5 fed-assault-rom-random6 fed-assault-rom-random7 fed-assault-rom-random8
  timer fed-raids-on-rom 180
every signal fed-assault-rom-random1
  ai federation fed-assault-rom-random1
every signal fed-assault-rom-random2
  ai federation fed-assault-rom-random2
every signal fed-assault-rom-random3
  ai federation fed-assault-rom-random3
every signal fed-assault-rom-random4
  ai federation fed-assault-rom-random4
every signal fed-assault-rom-random5
  ai federation fed-assault-rom-random5
every signal fed-assault-rom-random6
  ai federation fed-assault-rom-random6
every signal fed-assault-rom-random7
  ai federation fed-assault-rom-random7
every signal fed-assault-rom-random8
  ai federation fed-assault-rom-random8
every timer kli-raids-on-rom
  random kli-assault-rom-random1 kli-assault-rom-random2 kli-assault-rom-random3 kli-assault-rom-random4 kli-assault-rom-random5 kli-assault-rom-random6 kli-assault-rom-random7 kli-assault-rom-random8
  timer kli-raids-on-rom 180
every signal kli-assault-rom-random1
  ai klingon kli-assault-rom-random1
every signal kli-assault-rom-random2
  ai klingon kli-assault-rom-random2
every signal kli-assault-rom-random3
  ai klingon kli-assault-rom-random3
every signal kli-assault-rom-random4
  ai klingon kli-assault-rom-random4
every signal kli-assault-rom-random5
  ai klingon kli-assault-rom-random5
every signal kli-assault-rom-random6
  ai klingon kli-assault-rom-random6
every signal kli-assault-rom-random7
  ai klingon kli-assault-rom-random7
every signal kli-assault-rom-random8
  ai klingon kli-assault-rom-random8
every timer tau-raids-on-rom
  random taubat-assault-rom-random1 taubat-assault-rom-random2 taubat-assault-rom-random3 taubat-assault-rom-random4 taubat-assault-rom-random5 taubat-assault-rom-random6 taubat-assault-rom-random7 taubat-assault-rom-random8
  timer tau-raids-on-rom 180
every signal taubat-assault-rom-random1
  ai taubat taubat-assault-rom-random1
every signal taubat-assault-rom-random2
  ai taubat taubat-assault-rom-random2
every signal taubat-assault-rom-random3
  ai taubat taubat-assault-rom-random3
every signal taubat-assault-rom-random4
  ai taubat taubat-assault-rom-random4
every signal taubat-assault-rom-random5
  ai taubat taubat-assault-rom-random5
every signal taubat-assault-rom-random6
  ai taubat taubat-assault-rom-random6
every signal taubat-assault-rom-random7
  ai taubat taubat-assault-rom-random7
every signal taubat-assault-rom-random8
  ai taubat taubat-assault-rom-random8
on count armed-apc == 4
  ai klingon kli-assault-romulans-1
  ai klingon kli-assault-romulans-2

# --- What the rivals are up to --------------------------------------------
every colony-size federation 200   # their colony at twice its starting size
  say intelligence "The Federation is building up a large presence on this planet."
every colony-total taubat 50        # the settlement down to half
  say intelligence "The Taubat settlement has been laid waste."

# --- Objective 21: take 30 % of the Taubat settlement ----------------------
on captured taubat-colony 30
  say security "Sir, our strike on the Taubat settlement succeeded."
  complete 21

# --- Objective 23: find the temple with a science vehicle -----------------
on scanned science temple
  say intelligence "The field gradients point to this building as the radiation source. Its main containment field is misaligned; we must go in and fix it."
  complete 23
  reveal 25

# --- Objective 25: take the temple and hold it for a minute ---------------
# Our capture monitor reports each entry until the hold succeeds, then stops.
every entered temple as rom-temple-watch
  say engineering "Sir, the Taubat temple is ours."
every vacated temple
  say intelligence "Nobody holds the Taubat temple now."
on held temple 60
  say engineering "Sir, we control the alien power source."
  complete 25
  disarm rom-temple-watch
  disarm temple-unheld   # a held temple no longer fails us by being destroyed

# --- Rivals at the temple ---------------------------------------------------
# (Captures can't come from the computer's sides: they never capture.)
every captured-by federation temple
  say intelligence "The Federation has got into the Taubat temple."
every captured-by klingon temple
  say intelligence "The Klingons have got into the Taubat temple."
on destroyed temple
  say intelligence "The Taubat temple has been destroyed."

# --- Fails: the cause at once, a line at 10 s, the mission lost at 20 s ----
fail 22 count colony-hub == 0
  say intelligence "The Romulan colony hub has been destroyed."
  signal failing
# A rival's hold on the temple beats us (fail 24); only a human rival can capture.
on held-by federation temple 60
  fail 24 20
  say intelligence "The Federation now controls the Taubat power source."
  disarm temple-unheld
  signal failing
on held-by klingon temple 60
  fail 24 20
  say intelligence "The Klingons now control the Taubat power source."
  disarm temple-unheld
  signal failing
# The temple destroyed before anyone has held it (fail 26): any side's hold
# switches this off.
fail 26 destroyed temple as temple-unheld
  signal failing
on signal failing
  timer fail-notice 10
on timer fail-notice
  say intelligence "The Romulans are unable to finish their mission."

# --- Mission pass: all three objectives, a line at 10 s, the win at 20 s ----
on all-completed
  timer pass-notice 10
  timer pass 20
on timer pass-notice
  say intelligence "Mission accomplished for the Romulans."
on timer pass
  win

# Not expressible yet:
# - captured taubat-colony 30 counts the whole settlement (houses 1, hangars
#   3), but Reluva names the share by a held building's label: only one
#   hangar carries taubat-colony, so it fires only once that one is held.
# - captured-by / held-by <side> temple never fire: the computer's sides never
#   capture (kept for a human rival).
# - The temple scan is also wired to a stance switch (Tau_hostile_to_Romulan)
#   that the mission doesn't have, so nothing happens.
# - The rivals' own pass/fail lines and what the referee does with a rival's
#   pass; the generic side-failed rule covers only the fail.
# - The Romulans' own AI orders (build timetable, assaults on the Klingons at
#   900 s and on the Federation when its colony doubles, random raids) only run
#   when the side isn't played.

# --- J2 as R ----------------------------------------
# J2 "Absent Friends", played as the Romulans (slot 2).
)",
R"(# Rewritten from the game's own mission graph (scripts_out/J2.json).
mission J2 R
site romulan-colony 192 231
zone intrusion-zone 96 236 148 296
zone storm-field 244 284 252 292
site alien-structure 112 280   # the Hubrin fortress, as the 120 s message gives it
site taubat-hangar 120 295     # where the staff are held (group Taubat_Hangar)
objective 7
objective 8 hidden

# --- Timed messages, and the Hubrin turning on everyone at 120 s ------------
# (A magnetic storm flashes over a field south-east of the colony all along.)
on start
  storm storm-field
  timer storm-warning 60
  timer kidnap 120
  timer defence-advice 180
  timer weapons-advice 300
  timer mining-advice 360
  timer fed-assault-romulans 80
  timer fed-raids-on-rom 480
  timer kli-raids-on-rom 480
  timer hubrin-raids-on-rom 420
on timer storm-warning
  say intelligence "Our colony shield buffer is fluctuating. I can't be sure, but the magnetic storm may be the cause."
on timer kidnap
  hostile hubrin
  say intelligence "Two of our staff have gone missing! There's a transporter trace near an alien structure at {site:alien-structure}. Start a rescue right away!"
  complete 7
  reveal 8
on timer defence-advice
  say security "We must defend the colony and build our best technology against beings this strong. No wonder the Taubat left this world!"
on timer weapons-advice
  say security "Reports say photon tanks and turrets on the mountain tops may be the answer to the Hubrin's greater range and firepower."
on timer mining-advice
  say intelligence "To develop the colony fully we should set up remote mining sites, guarded by shield generators and turrets."

# --- The other sides' attacks on the colony -------------------------------
# Two Federation assaults at 80 s; from 480 s a random Federation and
# Klingon assault every 180 s; from 420 s a Hubrin raider every 120 s until
# the Hubrin fortress (or our hub) falls (each order beams its raider in
# near 134:305 itself); and the Klingons turn on us once
# the Federation colony has been wiped out.
on timer fed-assault-romulans
  ai federation fed-romulan-assault-one
  ai federation fed-romulan-assault-two
every timer fed-raids-on-rom
  random fed-assault-rom-random1 fed-assault-rom-random2 fed-assault-rom-random3 fed-assault-rom-random4 fed-assault-rom-random5 fed-assault-rom-random6 fed-assault-rom-random7 fed-assault-rom-random8
  timer fed-raids-on-rom 180
every signal fed-assault-rom-random1
  ai federation fed-assault-rom-random1
every signal fed-assault-rom-random2
  ai federation fed-assault-rom-random2
every signal fed-assault-rom-random3
  ai federation fed-assault-rom-random3
every signal fed-assault-rom-random4
  ai federation fed-assault-rom-random4
every signal fed-assault-rom-random5
  ai federation fed-assault-rom-random5
every signal fed-assault-rom-random6
  ai federation fed-assault-rom-random6
every signal fed-assault-rom-random7
  ai federation fed-assault-rom-random7
every signal fed-assault-rom-random8
  ai federation fed-assault-rom-random8
every timer kli-raids-on-rom
  random kli-assault-rom-random1 kli-assault-rom-random2 kli-assault-rom-random3 kli-assault-rom-random4 kli-assault-rom-random5 kli-assault-rom-random6 kli-assault-rom-random7 kli-assault-rom-random8
  timer kli-raids-on-rom 180
every signal kli-assault-rom-random1
  ai klingon kli-assault-rom-random1
every signal kli-assault-rom-random2
  ai klingon kli-assault-rom-random2
every signal kli-assault-rom-random3
  ai klingon kli-assault-rom-random3
every signal kli-assault-rom-random4
  ai klingon kli-assault-rom-random4
every signal kli-assault-rom-random5
  ai klingon kli-assault-rom-random5
every signal kli-assault-rom-random6
  ai klingon kli-assault-rom-random6
every signal kli-assault-rom-random7
  ai klingon kli-assault-rom-random7
every signal kli-assault-rom-random8
  ai klingon kli-assault-rom-random8
every timer hubrin-raids-on-rom
  random hubrin-assault-rom-random1 hubrin-assault-rom-random2 hubrin-assault-rom-random3 hubrin-assault-rom-random4 hubrin-assault-rom-random5 hubrin-assault-rom-random6 hubrin-assault-rom-random7 hubrin-assault-rom-random8
  timer hubrin-raids-on-rom 120
every signal hubrin-assault-rom-random1
  ai hubrin hubrin-assault-rom-random1
every signal hubrin-assault-rom-random2
  ai hubrin hubrin-assault-rom-random2
every signal hubrin-assault-rom-random3
  ai hubrin hubrin-assault-rom-random3
every signal hubrin-assault-rom-random4
  ai hubrin hubrin-assault-rom-random4
every signal hubrin-assault-rom-random5
  ai hubrin hubrin-assault-rom-random5
every signal hubrin-assault-rom-random6
  ai hubrin hubrin-assault-rom-random6
every signal hubrin-assault-rom-random7
  ai hubrin hubrin-assault-rom-random7
every signal hubrin-assault-rom-random8
  ai hubrin hubrin-assault-rom-random8
every destroyed hubrin-fortress
  stop hubrin-raids-on-rom
# (Four "none left" watches: three on the Federation's small buildings, one on
# their large ones; all four must report.)
on count federation-small-buildings <= 0
  signal fed-colony-down
on count federation-large-buildings <= 0
  signal fed-colony-down
on signal fed-colony-down*2
  ai klingon kli-romulan-assault-one
  ai klingon kli-romulan-assault-two

# --- Our bird-of-prey ------------------------------------------------------
# The first of our units inside the rectangle north-east of the colony sends
# the bird-of-prey along its route, where it lands at the last point.
on inside ours intrusion-zone
  move rom-bop 145 246 100 299 172 299 50 222

# --- Objective 8: rescue the staff from the Taubat hangar ----------------
# The raid: hold the hangar 10 s to start the search, 60 s more to finish it.
every entered taubat-hangar
  say intelligence "Searching the building for our staff."
every capture-lost taubat-hangar
  say intelligence "Something's wrong: the recovery team has stopped answering."
on captured taubat-hangar as rom-raid
  say intelligence "Our people aren't here. Faint bio-traces show they were taken elsewhere a while back."
  complete 8
  timer new-orders 7
  timer pass 10
on timer new-orders
  say communications "New orders from command: we're being moved to a new outpost, and another team will look into the loss of our staff."
on timer pass
  win
# A rival finishing its raid while ours is still running beats us (fail 10);
# once a rival has finished, our own success is futile (nothing happens).
# (A rival's raid here is a capture, which only a human rival can make.)
on beaten taubat-hangar
  fail 10 10
  say intelligence "A garbled message from our staff: it seems the Klingons or the Federation have raided the building."
every captured-by federation taubat-hangar
  timer fed-took-staff 7
every captured-by klingon taubat-hangar
  timer kli-took-staff 7
on timer fed-took-staff
  say intelligence "The Federation has taken the building and our staff with it."
on timer kli-took-staff
  say intelligence "The Klingons have taken the building and our staff with it."

# --- Fails: the cause at once, the mission lost 10 s later ----------------
on count colony-hub == 0
  fail 9 10
  say operations "The Romulan colony hub has been destroyed."
  stop hubrin-raids-on-rom
on destroyed taubat-hangar
  fail 10 10
  say operations "The Taubat hangar holding our staff has been destroyed. The mission has failed."

# Not expressible yet:
# - captured-by <side> taubat-hangar and beaten never fire against the
#   computer's sides: they never capture.
# - The objective sent at 120 s is shown only as the objective change; the
#   game's "new primary objective" mail goes to a distributor, not the panel.
# - The Romulans' own AI orders (colony guard at 60 s, assault on the
#   Klingons at 65 s, random raids, an assault on the Federation when the
#   Klingon colony is gone) only run when the side isn't played.

# --- J3 as R ----------------------------------------
# J3 "The Mask Slips", played as the Romulans (slot 2).
# Rewritten from the game's own mission graph (scripts_out/J3.json).
mission J3 R
site romulan-colony 144 295
site workshops 168 239          # the eight Taubat workshops (Raid_Building_One..Eight), 152-184:223-255
site metar-scan-building 176 239   # the harmonic charge's focal point: the Metar colony core
objective 11
objective 12 hidden

# --- Greeting ---------------------------------------------------------------
on start
  timer greeting 10
  timer fed-assault-romulans 80
  timer fed-raids-on-rom 420
  timer fed-heavy-raids-on-rom 600
  timer kli-raids-on-rom 420
  timer kli-heavy-raids-on-rom 600
  timer fed-scouts 60
  timer kli-scouts 60
  timer tau-raids-on-rom 180
on timer greeting
  say operations "Sir, the Taubat are getting busier: several units are heading in from the north."

# --- The other sides' attacks on the colony -------------------------------
# Two Federation assaults at 80 s; Federation and Klingon raids every 180 s
# from 420 s, heavier ones every 180 s from 600 s; a scout every 60 s from
# each; Taubat raids every 180 s from the start; and the Klingons turn on us
# once the Federation colony is wiped out.
on timer fed-assault-romulans
  ai federation fed-romulan-assault-one
  ai federation fed-romulan-assault-two
every timer fed-raids-on-rom
  random fed-assault-rom-random4 fed-assault-rom-random5 fed-assault-rom-random6 fed-assault-rom-random4 fed-assault-rom-random5 fed-assault-rom-random6 fed-assault-rom-random5 fed-assault-rom-random6
  timer fed-raids-on-rom 180
every signal fed-assault-rom-random4
  ai federation fed-assault-rom-random4
every signal fed-assault-rom-random5
  ai federation fed-assault-rom-random5
every signal fed-assault-rom-random6
  ai federation fed-assault-rom-random6
every timer fed-heavy-raids-on-rom
  random fed-assault-rom-random5a fed-assault-rom-random6a fed-assault-rom-random7a fed-assault-rom-random8a fed-assault-rom-random5a fed-assault-rom-random6a fed-assault-rom-random7a fed-assault-rom-random8a
  timer fed-heavy-raids-on-rom 180
every signal fed-assault-rom-random5a
  ai federation fed-assault-rom-random5a
every signal fed-assault-rom-random6a
  ai federation fed-assault-rom-random6a
every signal fed-assault-rom-random7a
  ai federation fed-assault-rom-random7a
every signal fed-assault-rom-random8a
  ai federation fed-assault-rom-random8a
every timer kli-raids-on-rom
  random kli-assault-rom-random4 kli-assault-rom-random5 kli-assault-rom-random6 kli-assault-rom-random4 kli-assault-rom-random5 kli-assault-rom-random6 kli-assault-rom-random5 kli-assault-rom-random6
  timer kli-raids-on-rom 180
every signal kli-assault-rom-random4
  ai klingon kli-assault-rom-random4
every signal kli-assault-rom-random5
  ai klingon kli-assault-rom-random5
every signal kli-assault-rom-random6
  ai klingon kli-assault-rom-random6
every timer kli-heavy-raids-on-rom
  random kli-assault-rom-random5a kli-assault-rom-random6a kli-assault-rom-random7a kli-assault-rom-random8a kli-assault-rom-random5a kli-assault-rom-random6a kli-assault-rom-random7a kli-assault-rom-random8a
  timer kli-heavy-raids-on-rom 180
every signal kli-assault-rom-random5a
  ai klingon kli-assault-rom-random5a
every signal kli-assault-rom-random6a
  ai klingon kli-assault-rom-random6a
every signal kli-assault-rom-random7a
  ai klingon kli-assault-rom-random7a
every signal kli-assault-rom-random8a
  ai klingon kli-assault-rom-random8a
every timer fed-scouts
  random fed-scout-kli-random1 fed-scout-kli-random2 fed-scout-kli-random3 fed-scout-kli-random4 fed-scout-rom-random5 fed-scout-rom-random6 fed-scout-rom-random7 fed-scout-rom-random8
  timer fed-scouts 60
every signal fed-scout-kli-random1
  ai federation fed-scout-kli-random1
every signal fed-scout-kli-random2
  ai federation fed-scout-kli-random2
every signal fed-scout-kli-random3
  ai federation fed-scout-kli-random3
every signal fed-scout-kli-random4
  ai federation fed-scout-kli-random4
every signal fed-scout-rom-random5
  ai federation fed-scout-rom-random5
every signal fed-scout-rom-random6
  ai federation fed-scout-rom-random6
every signal fed-scout-rom-random7
  ai federation fed-scout-rom-random7
every signal fed-scout-rom-random8
  ai federation fed-scout-rom-random8
every timer kli-scouts
  random kli-scout-fed-random1 kli-scout-fed-random2 kli-scout-fed-random3 kli-scout-fed-random4 kli-scout-rom-random5 kli-scout-rom-random6 kli-scout-rom-random7 kli-scout-rom-random8
  timer kli-scouts 60
every signal kli-scout-fed-random1
  ai klingon kli-scout-fed-random1
every signal kli-scout-fed-random2
  ai klingon kli-scout-fed-random2
every signal kli-scout-fed-random3
  ai klingon kli-scout-fed-random3
every signal kli-scout-fed-random4
  ai klingon kli-scout-fed-random4
every signal kli-scout-rom-random5
  ai klingon kli-scout-rom-random5
every signal kli-scout-rom-random6
  ai klingon kli-scout-rom-random6
every signal kli-scout-rom-random7
  ai klingon kli-scout-rom-random7
every signal kli-scout-rom-random8
  ai klingon kli-scout-rom-random8
every timer tau-raids-on-rom
  random rom-assualt-random-one rom-assualt-random-two romulan-assualt-one romulan-assualt-two rom-assualt-random-one rom-assualt-random-two romulan-assualt-one romulan-assualt-two
  timer tau-raids-on-rom 180
every signal rom-assualt-random-one
  ai taubat rom-assualt-random-one
every signal rom-assualt-random-two
  ai taubat rom-assualt-random-two
every signal romulan-assualt-one
  ai taubat romulan-assualt-one
every signal romulan-assualt-two
  ai taubat romulan-assualt-two
# (Four "none left" watches: three on the Federation's small buildings, one on
# their large ones; all four must report.)
on count federation-small-buildings <= 0
  signal fed-colony-down
on count federation-large-buildings <= 0
  signal fed-colony-down
on signal fed-colony-down*2
  ai klingon kli-romulan-assault-one
  ai klingon kli-romulan-assault-two

# --- Objective 11: capture three of the Taubat workshops ------------------
# Hold one 5 s to start the infiltration, 30 s more to take it. Each of the
# eight workshops (raid-building-one .. -eight) has its own watch.
every entered raid-building-one
  signal workshop-entered
every entered raid-building-two
  signal workshop-entered
every entered raid-building-three
  signal workshop-entered
every entered raid-building-four
  signal workshop-entered
every entered raid-building-five
  signal workshop-entered
every entered raid-building-six
  signal workshop-entered
every entered raid-building-seven
  signal workshop-entered
every entered raid-building-eight
  signal workshop-entered
every capture-lost raid-building-one
  signal workshop-capture-lost
every capture-lost raid-building-two
  signal workshop-capture-lost
every capture-lost raid-building-three
  signal workshop-capture-lost
every capture-lost raid-building-four
  signal workshop-capture-lost
every capture-lost raid-building-five
  signal workshop-capture-lost
every capture-lost raid-building-six
  signal workshop-capture-lost
every capture-lost raid-building-seven
  signal workshop-capture-lost
)",
R"(every capture-lost raid-building-eight
  signal workshop-capture-lost
every captured raid-building-one
  signal workshop-captured
every captured raid-building-two
  signal workshop-captured
every captured raid-building-three
  signal workshop-captured
every captured raid-building-four
  signal workshop-captured
every captured raid-building-five
  signal workshop-captured
every captured raid-building-six
  signal workshop-captured
every captured raid-building-seven
  signal workshop-captured
every captured raid-building-eight
  signal workshop-captured
every signal workshop-entered
  say operations "Infiltration teams are going in. Stand by..."
every signal workshop-capture-lost
  say engineering "We lost the capture. Try again."
every signal workshop-captured
  say operations "Sir, our troops now hold a Taubat workshop."
on captured raid-building-one     # any three of the eight
on captured raid-building-two
on captured raid-building-three
on signal workshop-captured*3
  say engineering "Sir, the harmonic generators are cut off, but a cascade discharge has begun and nothing can halt it now."
  complete 11
  reveal 12
  signal survive-step
  signal pass-step
# A workshop a rival got first is lost to us: beaten (a rival finished during
# our capture) or futile (ours finished after a rival's); six is fail 15.
# The Klingons' destroy watch succeeds whenever a workshop falls, whoever
# destroys it; the Federation's scan watch only for a human player.
every beaten raid-building-one
  signal workshop-lost
every beaten raid-building-two
  signal workshop-lost
every beaten raid-building-three
  signal workshop-lost
every beaten raid-building-four
  signal workshop-lost
every beaten raid-building-five
  signal workshop-lost
every beaten raid-building-six
  signal workshop-lost
every beaten raid-building-seven
  signal workshop-lost
every beaten raid-building-eight
  signal workshop-lost
every futile raid-building-one
  signal workshop-lost
every futile raid-building-two
  signal workshop-lost
every futile raid-building-three
  signal workshop-lost
every futile raid-building-four
  signal workshop-lost
every futile raid-building-five
  signal workshop-lost
every futile raid-building-six
  signal workshop-lost
every futile raid-building-seven
  signal workshop-lost
every futile raid-building-eight
  signal workshop-lost
on signal workshop-lost*6
  fail 15 10
  say operations "Too few hangars were taken in time. We can't complete the mission."

# --- Objective 12: scan the focal point of the harmonic charge ------------
# Held 10 s to start, 60 s more to finish. It wakes the Metar.
every scan-started science metar-scan-building
  say science "Scanners running now, sir."
every scan-lost science metar-scan-building
  say science "The scan has dropped out, sir. It needs another attempt."
on scanned science metar-scan-building as rom-core-scan
  say science "Confirmed: this is where the harmonic field is focused... Wait! The energy is off the scale, and harmonic fissures are opening everywhere!"
  reveal 12
  complete 12
  signal pass-step
  signal metar-wake
  disarm core-watch   # once scanned, its destruction no longer fails us

# --- The Metar wake: our scan, a rival's scan, or a portal destroyed --------
# (A rival's core scan, which makes ours futile, needs a human rival: the
# computer's sides never scan.)
every scanned-by federation metar-scan-building
  signal metar-wake
every scanned-by klingon metar-scan-building
  signal metar-wake
every count metar-portal == 2   # one of the three portals destroyed
  timer portal-surge 5
  timer portal-wake 10
on timer portal-surge
  say operations "An alien structure is down, and surges of power are running through the planet's core."
on timer portal-wake
  signal metar-wake
on signal metar-wake
  timer metar-stirs 10
  timer metar-lifeforms 25
  timer metar-outposts 30
  timer metar-raids-on-rom 60
  signal survive-step
on timer metar-stirs
  say "Incoming message" "See: the Metar sleep no more. Their eyes will turn on you!"
on timer metar-lifeforms
  say operations "Sir, many unknown life forms detected, with energy readings off the scale!"
on timer metar-outposts
  say operations "Our other outposts report the aliens coming up all over the planet and doing heavy damage."
every timer metar-raids-on-rom
  random metar-dea-protect metar-bur-rom-one metar-bur-rom-two metar-nak-rom-one metar-nak-rom-two metar-dea-rom-one metar-dea-rom-two metar-nak-rom-one
  timer metar-raids-on-rom 60
every signal metar-dea-protect
  ai metar metar-dea-protect
every signal metar-bur-rom-one
  ai metar metar-bur-rom-one
every signal metar-bur-rom-two
  ai metar metar-bur-rom-two
every signal metar-nak-rom-one
  ai metar metar-nak-rom-one
every signal metar-nak-rom-two
  ai metar metar-nak-rom-two
every signal metar-dea-rom-one
  ai metar metar-dea-rom-one
every signal metar-dea-rom-two
  ai metar metar-dea-rom-two

# --- Hold out for the Senate's ship: after 11 and the Metar's waking --------
on signal survive-step*2
  timer senate-orders 15
  timer rescue-craft 500
  timer data-request 1000
on timer senate-orders
  say "Message from the Senate" "Retreat to the colony and defend it. A starship is on the way for the records on the alien creatures... and for you too, naturally."
on timer rescue-craft
  say communications "Sir, a signal from the Senate's rescue ship: planetary interference stops them beaming us up for now. Stand by."
on timer data-request
  say "Incoming message" "Colony AP173.B: send us every record on the alien life forms at once. We will lift you and your crew out when it is in."
  signal pass-step

# --- Mission pass: objectives 11 and 12 and the pick-up, then 10 s ----------
on signal pass-step*3
  timer pass 10
on timer pass
  win

# --- Fails: the cause at once, the mission lost 10 s later ----------------
on count colony-hub == 0
  fail 14 10
  say operations "Our colony hub is gone. The mission has failed."
# The focal point destroyed before we scanned it (fail 16); our scan switches this
# off.
on destroyed metar-scan-building as core-watch
  fail 16 10
  say operations "The Taubat weapon's focal point has been destroyed. We can't complete the mission."

# Not expressible yet:
# - scanned-by <side> metar-scan-building never fires: the computer's sides
#   never scan (and the Federation never scans a workshop).
# - The jnt03rom table's string 13 (scan the artifacts near the workshops) is
#   not an objective in the game's graph, so it's left out.
# - A resource top-up object for the colony is active but has an empty
#   bundle (nothing to top up, so left out).
# - The Romulans' own AI orders (colony guard, assault on the Klingons at 80 s,
#   raids and scouts, an assault on the Federation when the Klingon colony is
#   gone) only run when the side isn't played.

# --- J4 as R ----------------------------------------
# J4, played as the Romulans (slot 2).
# Rewritten from the game's own mission graph (scripts_out/J4.json).
mission J4 R
site romulan-colony 298 333
zone old-klingon-colony 234 186 262 206
site klingon-arsenal 234 205      # group ~Kli Sec Cen
objective 12

# --- The other sides' attacks on the colony -------------------------------
# Three Klingon assaults at 360 s; Federation, Taubat and Metar raids every
# 120 s from 360 s; and a Taubat strike the first time our units go into
# the old Klingon colony.
on start
  timer kli-assault-romulans 360
  timer fed-raids-on-rom 360
  timer tau-raids-on-rom 360
  timer metar-raids-on-rom 360
on timer kli-assault-romulans
  ai klingon kli-assault-romulans-1
  ai klingon kli-assault-romulans-2
  ai klingon kli-assault-romulans-3
every timer fed-raids-on-rom
  random fed-assault-rom-random1 fed-assault-rom-random2 fed-assault-rom-random3 fed-assault-rom-random4 fed-assault-rom-random5 fed-assault-rom-random6 fed-assault-rom-random7 fed-assault-rom-random8
  timer fed-raids-on-rom 120
every signal fed-assault-rom-random1
  ai federation fed-assault-rom-random1
every signal fed-assault-rom-random2
  ai federation fed-assault-rom-random2
every signal fed-assault-rom-random3
  ai federation fed-assault-rom-random3
every signal fed-assault-rom-random4
  ai federation fed-assault-rom-random4
every signal fed-assault-rom-random5
  ai federation fed-assault-rom-random5
every signal fed-assault-rom-random6
  ai federation fed-assault-rom-random6
every signal fed-assault-rom-random7
  ai federation fed-assault-rom-random7
every signal fed-assault-rom-random8
  ai federation fed-assault-rom-random8
every timer tau-raids-on-rom
  random taubat-assault-rom-random1 taubat-assault-rom-random2 taubat-assault-rom-random3 taubat-assault-rom-random4 taubat-assault-rom-random5 taubat-assault-rom-random6 taubat-assault-rom-random7 taubat-assault-rom-random8
  timer tau-raids-on-rom 120
every signal taubat-assault-rom-random1
  ai taubat taubat-assault-rom-random1
every signal taubat-assault-rom-random2
  ai taubat taubat-assault-rom-random2
every signal taubat-assault-rom-random3
  ai taubat taubat-assault-rom-random3
every signal taubat-assault-rom-random4
  ai taubat taubat-assault-rom-random4
every signal taubat-assault-rom-random5
  ai taubat taubat-assault-rom-random5
every signal taubat-assault-rom-random6
  ai taubat taubat-assault-rom-random6
every signal taubat-assault-rom-random7
  ai taubat taubat-assault-rom-random7
# (taubat-assault-rom-random8: the game names this order but has none by that name, so this pick does nothing)
every timer metar-raids-on-rom
  random metar-assault-rom-random1 metar-assault-rom-random2 metar-assault-rom-random3 metar-assault-rom-random4 metar-assault-rom-random5 metar-assault-rom-random6 metar-assault-rom-random7 metar-assault-rom-random8
  timer metar-raids-on-rom 120
every signal metar-assault-rom-random1
  ai metar metar-assault-rom-random1
every signal metar-assault-rom-random2
  ai metar metar-assault-rom-random2
every signal metar-assault-rom-random3
  ai metar metar-assault-rom-random3
every signal metar-assault-rom-random4
  ai metar metar-assault-rom-random4
every signal metar-assault-rom-random5
  ai metar metar-assault-rom-random5
every signal metar-assault-rom-random6
  ai metar metar-assault-rom-random6
every signal metar-assault-rom-random7
  ai metar metar-assault-rom-random7
every signal metar-assault-rom-random8
  ai metar metar-assault-rom-random8
on inside ours old-klingon-colony
  ai taubat taubat-sat-rom-colony-assault

# --- Objective 12: take the Klingon arsenal and arm it ---------------------
# Held 1 s to start, 10 s more to take it; the anti-matter fields then take
# 60 s to set up.
on entered klingon-arsenal
  say intelligence "We're inside, sir. Searching the building."
on capture-lost klingon-arsenal
  say intelligence "The building has been abandoned before the search was finished."
on captured klingon-arsenal
  say intelligence "The building is ours, and we're starting to tune the anti-matter fields."
  timer anti-matter 60
on timer anti-matter
  complete 12
# (Fail 13: a rival taking the arsenal during our raid; nothing in the
# mission ever sends that, so it can't happen.)
fail 13 beaten klingon-arsenal
  say intelligence "The arsenal has been destroyed. The mission has failed."
  signal failing

# --- Mission pass: a line at 10 s, the win at 20 s --------------------------
on all-completed
  timer pass-notice 10
  timer pass 20
on timer pass-notice
  say intelligence "Mission accomplished for the Romulans."
on timer pass
  win

# --- Fails: the cause at once, a line at 10 s, the mission lost at 20 s ----
fail 14 count colony-hub == 0
  say intelligence "The Romulan colony hub has been destroyed."
  signal failing
on signal failing
  timer fail-notice 10
on timer fail-notice
  say intelligence "The Romulans are unable to finish their mission."

# Not expressible yet:
# - The raid's "beaten" outcome carries fail 13, but no object sends it (the
#   J4 raids don't hand off); written on the building's name, not the raid
#   label (kli-sec-cen), so Reluva's hand-off can't fire it either.
# - The 60 s anti-matter timer also carries a line ("the building is armed,
#   evacuate") that the game sends only to the pass counter, never to the
#   panel, so it isn't said here.
# - The Romulans' own AI orders (colony guard, a refugee sentry, random raids
#   on the Federation from 300 s) only run when the side isn't played.

# --- R8 as R ----------------------------------------
# R8 as the Romulans, written from the game's own mission graph (STNW.exe);
# there is no recording of this mission.
mission R8 R
site colony 276 272
site abandoned-base 404 412
site metar-colony 368 288
site metar-colony-core 370 285
zone colony-arrival 263 263 288 280
zone metar-approach 344 260 391 315
zone abandoned-base 384 399 424 435
zone tank-park 401 429 403 431
zone artillery-park 416 403 420 406
zone apc-park 418 416 421 419
objective 12
objective 14

# Opening: the vehicles at the abandoned base, then the meteor storm over it
# (warning at 20 s; from 80 s to 260 s a meteor a second over the base and
# one every 3 s on each vehicle park; "over" said at 240 s, "subsiding" when
# it stops).
on start
  timer briefing 5
  timer storm-warning 20
  timer storm-start 80
  timer storm-over 240
  timer storm-stop 260
on timer briefing
  say operations "You now command whatever remains at the deserted base north-east of here. The surviving APCs carry the essential supplies."
on timer storm-warning
  say science "A meteor storm is about to hit the abandoned base. Get the supply APCs out and bring them safely to our base. The Metar will try to stop us."
on timer storm-start
  say science "Meteorites coming down on the old main colony! Losing those APCs would be a disaster."
  meteors abandoned-base 1
  meteors tank-park 3
  meteors artillery-park 3
  meteors apc-park 3
on timer storm-over
  say science "The meteor storm is over. The old base's self-destruct has started; what's left will blow when the supplies arrive."
on timer storm-stop
  meteors abandoned-base off
  meteors tank-park off
  meteors artillery-park off
  meteors apc-park off
  say science "The storm is dying down. The old base has been put out of action for good; no Romulan technology must reach the enemy."

# Any of our units inside the rectangle round the Metar colony stirs its
# defences.
on inside ours metar-approach
  say intelligence "Careful: the Metar defences are responding to us."
  signal metar-guard
on signal metar-guard
  ai metar met-colony-protection-1
  ai metar met-colony-protection-2
  ai metar met-colony-protection-3
  ai metar met-colony-protection-4
  ai metar met-colony-protection-5

# Objective 12: the first supply vehicle (the whole group) inside the
# rectangle at the colony meets it; that
# also calls off the Metar sentinels in the corridor and on the plateau,
# sets the Metar guarding their colony and starts their raids (in 5 min).
on inside supply-vehicle-1 colony-arrival
)",
R"(  say engineering "Supply vehicle 1 is here. Getting ready to beam its load into colony storage."
  timer beam-1 10
  timer sentinel-alert 10
  signal vehicle-arrived
on inside supply-vehicle-2 colony-arrival
  say engineering "Supply vehicle 2 is here. Getting ready to beam its load into colony storage."
  timer beam-2 10
  timer sentinel-alert 10
  signal vehicle-arrived
on inside supply-vehicle-3 colony-arrival
  say engineering "Supply vehicle 3 is here. Getting ready to beam its load into colony storage."
  timer beam-3 10
  timer sentinel-alert 10
  signal vehicle-arrived
on signal vehicle-arrived
  complete 12
  signal metar-guard
  timer metar-raids 300
  ai metar abort met-corridor-sentinel-0
  ai metar abort met-corridor-sentinel-1
  ai metar abort met-corridor-sentinel-2
  ai metar abort met-corridor-sentinel-3
  ai metar abort met-corridor-sentinel-4
  ai metar abort met-corridor-sentinel-5
  ai metar abort met-corridor-sentinel-6
  ai metar abort met-corridor-sentinel-7
  ai metar abort met-plateau-sentinel-0
  ai metar abort met-plateau-sentinel-1
  ai metar abort met-plateau-sentinel-2
  ai metar abort met-plateau-sentinel-3
  ai metar abort met-plateau-sentinel-4
  ai metar abort met-plateau-sentinel-5
  ai metar abort met-plateau-sentinel-6
  ai metar abort met-plateau-sentinel-7
  ai metar abort met-plateau-sentinel-8
every timer sentinel-alert
  say intelligence "Metar combat units are reported on the move everywhere."

# Ten seconds after each arrival the vehicle is beamed up; once it's gone
# its load is in (165 T dilithium, 165 T transaluminium, 1350 KG nitrium,
# 1650 KG duranium). The first beam-up order ends the supply-vehicle watch;
# the first vehicle gone blows up what is left of the abandoned base.
on timer beam-1
  disarm supply-fail
  beam-up supply-vehicle-1
on beamed-up supply-vehicle-1
  say engineering "Supply vehicle 1 has reached colony storage. Next, seize the RAS commander of the Metar inside the core of their colony."
  signal base-self-destruct
  give dilithium 165
  give transaluminium 165
  give nitrium 1350
  give duranium 1650
on timer beam-2
  disarm supply-fail
  beam-up supply-vehicle-2
on beamed-up supply-vehicle-2
  say engineering "Supply vehicle 2 is in colony storage. The Metar RAS is waiting for us."
  signal base-self-destruct
  give dilithium 165
  give transaluminium 165
  give nitrium 1350
  give duranium 1650
on timer beam-3
  disarm supply-fail
  beam-up supply-vehicle-3
on beamed-up supply-vehicle-3
  say engineering "Supply vehicle 3 is in colony storage. Take the Metar colony core, search it and bring the RAS commander back."
  signal base-self-destruct
  give dilithium 165
  give transaluminium 165
  give nitrium 1350
  give duranium 1650

on signal base-self-destruct
  damage primary-colony-hub all
  damage primary-colony-building all

# Objective 14: hold the Metar colony core (2 s to get in, 20 s to search;
# leaving before then is reported once).
on entered metar-colony-core
  say intelligence "We're inside, sir. Searching the building."
on capture-lost metar-colony-core
  say intelligence "We've left the building before the search was done."
on captured metar-colony-core
  say intelligence "We have the RAS commander, sir. Transporting him now."
  complete 14

# The Metar raids: 5 min after the first arrival, then every 2 min, one of
# eight at random.
on timer metar-raids
  timer metar-raid 120
every timer metar-raid
  random metar-raid-1 metar-raid-2 metar-raid-3 metar-raid-4 metar-raid-5 metar-raid-6 metar-raid-7 metar-raid-8
  timer metar-raid 120
every signal metar-raid-1
  ai metar metar-assault-rom-random1
every signal metar-raid-2
  ai metar metar-assault-rom-random2
every signal metar-raid-3
  ai metar metar-assault-rom-random3
every signal metar-raid-4
  ai metar metar-assault-rom-random4
every signal metar-raid-5
  ai metar metar-assault-rom-random5
every signal metar-raid-6
  ai metar metar-assault-rom-random6
every signal metar-raid-7
  ai metar metar-assault-rom-random7
every signal metar-raid-8
  ai metar metar-assault-rom-random8

# Fails: the cause and "the Romulans cannot complete their mission" at once,
# the end 10 s later. The supply-vehicle check (no armed APC left: the
# game counts that type, which leaves out vehicle 3, a different type)
# stops with the first beam-up.
on count armed-apc <= 0 as supply-fail
  fail 13 10
  say operations "Every supply vehicle is lost. The mission has failed."
  say intelligence "The Romulans can't complete their mission."
# The game wires this fail to the core search's "beaten to it" output, which
# nothing sends: it never fires (the core's destruction isn't watched).
on beaten metar-colony-core
  fail 15 10
  say intelligence "The colony core is destroyed and the RAS commander is dead. The mission has failed."
  say intelligence "The Romulans can't complete their mission."
on destroyed colony-hub
  fail 16 10
  say operations "The colony hub is destroyed. The mission has failed."
  say intelligence "The Romulans can't complete their mission."

# The pass: both objectives, the message at once, the win 10 s later.
on all-completed
  say intelligence "The Romulans' mission is accomplished."
  timer mission-won 10
on timer mission-won
  win

# Not expressible yet:
# - At the start the vehicles are moved to their posts by the side's own
#   orders (scout-prepositioner, supply-vehicle-prepositioner,
#   tank-prepositioner, artillery-prepositioner).
# - Fail 15 is wired to the core search's "beaten to it" output, which
#   nothing sends; written as the core destroyed (inferred).

# --- R9 as R ----------------------------------------
# R9 as the Romulans, rewritten from the game's own mission graph (STNW.exe).
mission R9 R
zone federation-ambush-south 396 320 415 336
zone federation-ambush-north 379 328 391 347
zone klingon-ambush 207 384 232 409
zone klingon-colony 224 367 236 383
objective 12
objective 14
objective 16

# Opening: the Federation, the Klingons and the Metar each start random
# raids on us after 3 minutes, then one every 2 minutes.
on start
  timer federation-raids 180
  timer klingon-raids 180
  timer metar-raids 180

# Ambushes: any of our units inside these rectangles springs the waiting
# groups.
on inside ours federation-ambush-south
  ai federation trigger fed-pouncer-0
on inside ours federation-ambush-north
  ai federation trigger fed-pouncer-1
on inside ours klingon-ambush
  ai klingon trigger kli-pouncer-0
  ai klingon trigger kli-pouncer-1

# Objective 12: hold the Federation science block (2 s to get in, 20 s to
# search; leaving before then is reported once); the device data opens its
# research topic.
on entered fed-science-block
  say intelligence "We're inside, sir. Searching the building."
on capture-lost fed-science-block
  say intelligence "We've left the building before the search was done."
on captured fed-science-block
  say intelligence "We have the device data, sir. Sending it now."
  complete 12
  research bgd

# Objective 14: the study done wakes three watches: a vehicle yard (7 s
# after one stands, the device is sent on its way to the Klingon colony),
# the device's arrival there, and the device being made.
on researched bgd
  say engineering "The device study is finished. We've asked for one to be built."
  arm yard-check
  arm device-route
  arm device-made
on built vehicle-yard as yard-check dormant
  timer device-launch 7
on timer device-launch
  ai romulan rom-bgd-controller
# once the device exists, losing it fails the mission
on built bad-guy-device as device-made dormant
  complete 14
  arm device-fail

# Objective 16: the device inside the rectangle at the Klingon colony and,
# 7 s on, every Klingon building and their hub destroyed outright; the
# colony down to half its size meets the objective.
on inside bad-guy-device klingon-colony as device-route dormant
  say security "The device has been placed in the Klingon colony. Prepare for activation."
  timer device-blast 7
on timer device-blast
  damage kln-building all
  damage kln-colony-hub all
on colony-total klingon 50
  complete 16

# The others' hubs: losing one calls off that side's remaining build orders.
on hub-destroyed federation
  ai federation abort fed-npc-build-landing-pad
  ai federation abort fed-npc-build-sickbay
  ai federation abort fed-npc-build-farm
on hub-destroyed klingon
  ai klingon abort kli-npc-build-sickbay
  ai klingon abort kli-npc-build-res-proc
  ai klingon abort kli-npc-build-farm

# The raids: one of eight at random from each side, every 2 minutes.
on timer federation-raids
  timer federation-raid 120
every timer federation-raid
  random federation-raid-1 federation-raid-2 federation-raid-3 federation-raid-4 federation-raid-5 federation-raid-6 federation-raid-7 federation-raid-8
  timer federation-raid 120
every signal federation-raid-1
  ai federation fed-assault-rom-random1
every signal federation-raid-2
  ai federation fed-assault-rom-random2
every signal federation-raid-3
  ai federation fed-assault-rom-random3
every signal federation-raid-4
  ai federation fed-assault-rom-random4
every signal federation-raid-5
  ai federation fed-assault-rom-random5
every signal federation-raid-6
  ai federation fed-assault-rom-random6
every signal federation-raid-7
  ai federation fed-assault-rom-random7
every signal federation-raid-8
  ai federation fed-assault-rom-random8
on timer klingon-raids
  timer klingon-raid 120
every timer klingon-raid
  random klingon-raid-1 klingon-raid-2 klingon-raid-3 klingon-raid-4 klingon-raid-5 klingon-raid-6 klingon-raid-7 klingon-raid-8
  timer klingon-raid 120
every signal klingon-raid-1
  ai klingon kli-assault-rom-random1
every signal klingon-raid-2
  ai klingon kli-assault-rom-random2
every signal klingon-raid-3
  ai klingon kli-assault-rom-random3
every signal klingon-raid-4
  ai klingon kli-assault-rom-random4
every signal klingon-raid-5
  ai klingon kli-assault-rom-random5
every signal klingon-raid-6
  ai klingon kli-assault-rom-random6
every signal klingon-raid-7
  ai klingon kli-assault-rom-random7
every signal klingon-raid-8
  ai klingon kli-assault-rom-random8
on timer metar-raids
  timer metar-raid 120
every timer metar-raid
  random metar-raid-1 metar-raid-2 metar-raid-3 metar-raid-4 metar-raid-5 metar-raid-6 metar-raid-7 metar-raid-8
  timer metar-raid 120
every signal metar-raid-1
  ai metar metar-assault-rom-random1
every signal metar-raid-2
  ai metar metar-assault-rom-random2
every signal metar-raid-3
  ai metar metar-assault-rom-random3
every signal metar-raid-4
  ai metar metar-assault-rom-random4
every signal metar-raid-5
  ai metar metar-assault-rom-random5
every signal metar-raid-6
  ai metar metar-assault-rom-random6
every signal metar-raid-7
  ai metar metar-assault-rom-random7
every signal metar-raid-8
  ai metar metar-assault-rom-random8

# Fails: the cause at once, "the Romulans cannot complete their mission" at
# 5 s, the end at 10 s.
# The game wires this fail to the search's "beaten to it" output, which
# nothing sends: it never fires (its line says the block was destroyed, but
# the block's destruction isn't watched).
on beaten fed-science-block
  fail 13 10
  say intelligence "The science block is destroyed. The mission has failed."
  signal failing
on destroyed bad-guy-device as device-fail dormant
  fail 15 10
  say operations "The device has been destroyed. The mission has failed."
  signal failing
on destroyed colony-hub
  fail 17 10
  say operations "The colony hub is destroyed. The mission has failed."
  signal failing
on signal failing
  timer fail-message 5
on timer fail-message
  say intelligence "The Romulans can't complete their mission."

# The pass: all three objectives, the message 5 s later, the win at 10 s.
on all-completed
  timer mission-pass 5
  timer mission-won 10
on timer mission-pass
  say intelligence "The Romulans' mission is accomplished."
on timer mission-won
  win

# Not expressible yet:
# - Fail 13 is wired to the search's "beaten to it" output, which nothing
#   sends; written as the block destroyed (inferred from its line).

# --- J6 as R ----------------------------------------
# J6 as the Romulans (slot 2), from the game's own mission graph.
# Test the alien device on the Klingons (scan their hub for a target, then
# bring down their shields and beam it in), and scan the Metar core.
mission J6 R
site klingon-colony 234 301
site metar-colony 386 317
objective 13
objective 15
objective 17

# Fail criteria, the game's way: the fail string at once, word to every
# side at 10 s that the Romulans can't complete their mission, the end at
# 20 s. Our hub (19); the Klingon hub lost before our strong scan of it
# is done (14: the scan disarms this watch, see objective 13).
on destroyed colony-hub
  fail 19 20
  say intelligence "Our colony hub has been destroyed."
  signal failing
on hub-destroyed klingon as klingon-hub-watch
  fail 14 20
  signal failing
on signal failing
  timer fail-news 10
on timer fail-news
  say intelligence "The Romulans' mission can no longer succeed."

# Start: the other sides' attack clocks
on start
  timer fed-raid-kli 180
  timer fed-raid-rom 180
  timer kli-raid-fed 240
  timer kli-raid-rom 180
  timer met-raid-fed 480
  timer met-raid-kli 480
  timer met-raid-rom 480
  timer met-assault-klingons-1 1
  timer met-assault-romulans-1 1
  timer met-assault-romulans-2 900
  timer met-assault-federation-1 1
  timer met-assault-federation-2 1200

# Arrivals: the Klingons warn us off their colony; advice near the Metar
on seen klingon-colony
  say "Incoming message" "Away with you, Romulan! The treaty gives you no right to creep about on Klingon soil."
on seen metar-colony
  say intelligence "Leave the aliens be if you can - stirring them up may cost you."

# Objective 13: a strong engineering scan of the Klingon hub, held 10 s;
# done, it stops the fail-14 watch on their hub
every scan-started strong-engineering klingon-hub
  say engineering "Starting the survey. Looking for a likely target."
every scan-lost strong-engineering klingon-hub
  say engineering "The scan broke off. Allow me proper time, please."
on scanned strong-engineering klingon-hub
  say engineering "Their council chamber should suit the test nicely. Waiting for their shields to drop."
  complete 13
  disarm klingon-hub-watch
# An ordinary engineering scan of the hub only finds that Grinke is needed
every scan-started engineering klingon-hub
  say engineering "Scan under way, Sir."
every scan-lost engineering klingon-hub
  say engineering "Lost the scan. Starting again."
on scanned engineering klingon-hub
  say engineering "Survey done, but their screens are scrambling our sensors. Could Grinke lend a hand?"

# Objective 15: every Klingon shield generator destroyed; the device is
# beamed in and the objective met 15 s later, when the device also takes
# out the Klingon hub
on count klingon-shield-generator == 0
  timer kli-shields-down 5
  timer device-placed 15
on timer kli-shields-down
  say engineering "Klingon shields gone. Beaming the device across."
on timer device-placed
)",
R"(  say engineering "The device is in place."
  complete 15
  damage colony-hub all klingon

# Objective 17: a strong science scan of the Metar core (1 s to start, 10 s
# held); it also opens the biodata research
every scan-started strong-science metar-core
  say science "Running a thorough scan."
every scan-lost strong-science metar-core
  say science "Scan lost. You did not allow me enough time."
on scanned strong-science metar-core
  say science "It worked. The creatures are reshaping their own make-up - remarkable. Medical should study the data."
  complete 17
  research metar-biodata
every scan-started science metar-core
  say science "Scan started, Sir."
every scan-lost science metar-core
  say science "The scan dropped, Sir. Trying again."
on scanned science metar-core
  say science "Scan finished, though Kensal ought to check my results."
# The game also offers the research when a sickbay ward (type 314) is built
on built sickbay-ward
  research metar-biodata
on researched metar-biodata
  say medical "The biodata analysis is done. My findings are intriguing."

# Mission passed (all three objectives): the news at 10 s, the win at 20 s
on all-completed
  timer pass-news 10
  timer pass 20
on timer pass-news
  say intelligence "The Romulans' mission is accomplished."
on timer pass
  win

# The other sides' AI: attacks on a clock (the Romulans' own attack orders
# are left out: the player gives those)
# Federation raids on the Klingons: one of eight attacks every 60 s, the first at 180 s
every timer fed-raid-kli
  timer fed-raid-kli 60
  random fed-raid-kli-1 fed-raid-kli-2 fed-raid-kli-3 fed-raid-kli-4 fed-raid-kli-5 fed-raid-kli-6 fed-raid-kli-7 fed-raid-kli-8
every signal fed-raid-kli-1
  ai federation fed-assault-kli-random1
every signal fed-raid-kli-2
  ai federation fed-assault-kli-random2
every signal fed-raid-kli-3
  ai federation fed-assault-kli-random3
every signal fed-raid-kli-4
  ai federation fed-assault-kli-random4
every signal fed-raid-kli-5
  ai federation fed-assault-kli-random5
every signal fed-raid-kli-6
  ai federation fed-assault-kli-random6
every signal fed-raid-kli-7
  ai federation fed-assault-kli-random7
every signal fed-raid-kli-8
  ai federation fed-assault-kli-random8

# Federation raids on the Romulans: one of eight attacks every 60 s, the first at 180 s
every timer fed-raid-rom
  timer fed-raid-rom 60
  random fed-raid-rom-1 fed-raid-rom-2 fed-raid-rom-3 fed-raid-rom-4 fed-raid-rom-5 fed-raid-rom-6 fed-raid-rom-7 fed-raid-rom-8
every signal fed-raid-rom-1
  ai federation fed-assault-rom-random1
every signal fed-raid-rom-2
  ai federation fed-assault-rom-random2
every signal fed-raid-rom-3
  ai federation fed-assault-rom-random3
every signal fed-raid-rom-4
  ai federation fed-assault-rom-random4
every signal fed-raid-rom-5
  ai federation fed-assault-rom-random5
every signal fed-raid-rom-6
  ai federation fed-assault-rom-random6
every signal fed-raid-rom-7
  ai federation fed-assault-rom-random7
every signal fed-raid-rom-8
  ai federation fed-assault-rom-random8

# Klingon raids on the Federation: one of eight attacks every 60 s, the first at 240 s
every timer kli-raid-fed
  timer kli-raid-fed 60
  random kli-raid-fed-1 kli-raid-fed-2 kli-raid-fed-3 kli-raid-fed-4 kli-raid-fed-5 kli-raid-fed-6 kli-raid-fed-7 kli-raid-fed-8
every signal kli-raid-fed-1
  ai klingon kli-assault-fed-random1
every signal kli-raid-fed-2
  ai klingon kli-assault-fed-random2
every signal kli-raid-fed-3
  ai klingon kli-assault-fed-random3
every signal kli-raid-fed-4
  ai klingon kli-assault-fed-random4
every signal kli-raid-fed-5
  ai klingon kli-assault-fed-random5
every signal kli-raid-fed-6
  ai klingon kli-assault-fed-random6
every signal kli-raid-fed-7
  ai klingon kli-assault-fed-random7
every signal kli-raid-fed-8
  ai klingon kli-assault-fed-random8

# Klingon raids on the Romulans: one of eight attacks every 60 s, the first at 180 s
every timer kli-raid-rom
  timer kli-raid-rom 60
  random kli-raid-rom-1 kli-raid-rom-2 kli-raid-rom-3 kli-raid-rom-4 kli-raid-rom-5 kli-raid-rom-6 kli-raid-rom-7 kli-raid-rom-8
every signal kli-raid-rom-1
  ai klingon kli-assault-rom-random1
every signal kli-raid-rom-2
  ai klingon kli-assault-rom-random2
every signal kli-raid-rom-3
  ai klingon kli-assault-rom-random3
every signal kli-raid-rom-4
  ai klingon kli-assault-rom-random4
every signal kli-raid-rom-5
  ai klingon kli-assault-rom-random5
every signal kli-raid-rom-6
  ai klingon kli-assault-rom-random6
every signal kli-raid-rom-7
  ai klingon kli-assault-rom-random7
every signal kli-raid-rom-8
  ai klingon kli-assault-rom-random8

# Metar raids on the Federation: one of eight attacks every 120 s, the first at 480 s
every timer met-raid-fed
  timer met-raid-fed 120
  random met-raid-fed-1 met-raid-fed-2 met-raid-fed-3 met-raid-fed-4 met-raid-fed-5 met-raid-fed-6 met-raid-fed-7 met-raid-fed-8
every signal met-raid-fed-1
  ai metar metar-assault-fed-random1
every signal met-raid-fed-2
  ai metar metar-assault-fed-random2
every signal met-raid-fed-3
  ai metar metar-assault-fed-random3
every signal met-raid-fed-4
  ai metar metar-assault-fed-random4
every signal met-raid-fed-5
  ai metar metar-assault-fed-random5
every signal met-raid-fed-6
  ai metar metar-assault-fed-random6
every signal met-raid-fed-7
  ai metar metar-assault-fed-random7
every signal met-raid-fed-8
  ai metar metar-assault-fed-random8

# Metar raids on the Klingons: one of eight attacks every 120 s, the first at 480 s
every timer met-raid-kli
  timer met-raid-kli 120
  random met-raid-kli-1 met-raid-kli-2 met-raid-kli-3 met-raid-kli-4 met-raid-kli-5 met-raid-kli-6 met-raid-kli-7 met-raid-kli-8
every signal met-raid-kli-1
  ai metar metar-assault-kli-random1
every signal met-raid-kli-2
  ai metar metar-assault-kli-random2
every signal met-raid-kli-3
  ai metar metar-assault-kli-random3
every signal met-raid-kli-4
  ai metar metar-assault-kli-random4
every signal met-raid-kli-5
  ai metar metar-assault-kli-random5
every signal met-raid-kli-6
  ai metar metar-assault-kli-random6
every signal met-raid-kli-7
  ai metar metar-assault-kli-random7
every signal met-raid-kli-8
  ai metar metar-assault-kli-random8

# Metar raids on the Romulans: one of eight attacks every 120 s, the first at 480 s
every timer met-raid-rom
  timer met-raid-rom 120
  random met-raid-rom-1 met-raid-rom-2 met-raid-rom-3 met-raid-rom-4 met-raid-rom-5 met-raid-rom-6 met-raid-rom-7 met-raid-rom-8
every signal met-raid-rom-1
  ai metar metar-assault-rom-random1
every signal met-raid-rom-2
  ai metar metar-assault-rom-random2
every signal met-raid-rom-3
  ai metar metar-assault-rom-random3
every signal met-raid-rom-4
  ai metar metar-assault-rom-random4
every signal met-raid-rom-5
  ai metar metar-assault-rom-random5
every signal met-raid-rom-6
  ai metar metar-assault-rom-random6
every signal met-raid-rom-7
  ai metar metar-assault-rom-random7
every signal met-raid-rom-8
  ai metar metar-assault-rom-random8

# Metar orders on a clock
on timer met-assault-klingons-1
  ai metar met-assault-klingons-1
on timer met-assault-romulans-1
  ai metar met-assault-romulans-1
on timer met-assault-romulans-2
  ai metar met-assault-romulans-2
on timer met-assault-federation-1
  ai metar met-assault-federation-1
on timer met-assault-federation-2
  ai metar met-assault-federation-2

# Not expressible yet:
# - When the Federation destroys our shields the game means to tell us they
#   took the device, but the line goes to a misspelt target and never shows.
# - The scan "futile" lines are unreachable in J6 (nothing sends Go_Futile).
# - J6 gives the Romulans no "scan found nothing" lines (the other two sides
#   have them).
# - Messages caused by the other sides: their mission passed/failed
#   announcements; the Klingons' "an alien device in the main hall" goes to
#   them, not us. There's no event for a rival's pass.
# - Whether the biodata research needs both the scan and the ward, or either:
#   the game sends the topic from both; here either opens it.
# - The AI sides stop their build orders when their hub falls, and build on
#   their own timetable (npc build orders).

# --- J7 as R ----------------------------------------
# J7 as the Romulans (slot 2), from the game's own mission graph.
# Stock up dilithium and transal for the Hubrin (the game takes a share of
# each), and keep our Hubrin guide alive; both objectives are met together.
mission J7 R
objective 13
objective 15

# Fail criteria, the game's way: the cause at once, word to every side at
# 5 s that the Romulans can't complete their mission, the end at 10 s.
# Our Hubrin guide killed (16). Our hub's loss is only announced: the game
# never fails 14 for the Romulans (see the end).
on destroyed rom-hubrin
  fail 16 10
  say intelligence "Our Hubrin guide is dead."
  signal failing
on destroyed colony-hub
  say intelligence "Our colony hub has been destroyed."
on signal failing
  timer fail-news 5
on timer fail-news
  say intelligence "The Romulans' mission can no longer succeed."

# Start: the other sides' attack clocks
on start
  timer fed-raid-kli 180
  timer fed-raid-rom 180
  timer kli-raid-fed 180
  timer kli-raid-rom 180
  timer tau-raid-fed 180
  timer tau-raid-kli 180
  timer tau-raid-rom 180
  timer met-raid-fed 300
  timer met-raid-kli 300
  timer met-raid-rom 300
  timer met-assault-federation 1800
  timer met-assault-klingon 2100
  timer met-assault-romulan 2400

# Objectives 13 and 15: 250 of dilithium in stock (100 is taken), 250 of
# transal in stock (50 is taken); with both delivered the guide's job is done
on stock dilithium 250
  take dilithium 100
  say engineering "The dilithium quota is met."
  signal quota
on stock transalumin 250
  take transalumin 50
  say engineering "Sir, the transal quota is met."
  signal quota
on signal quota*2
  complete 13
  complete 15

# Mission passed: the news at once, the win 10 s later
on all-completed
  say intelligence "The Romulans' mission is accomplished."
  timer pass 10
on timer pass
  win

# A colony grown to four times its starting size draws two Metar attacks
# on it: ours, and the other sides' too
on colony-size 400
  ai metar met-assault-leading-player-rom-a
  ai metar met-assault-leading-player-rom-b
on colony-size federation 400
  ai metar met-assault-leading-player-fed-a
  ai metar met-assault-leading-player-fed-b
on colony-size klingon 400
  ai metar met-assault-leading-player-kli-a
  ai metar met-assault-leading-player-kli-b

# The other sides' AI: attacks on a clock (the Romulans' own attack orders
# are left out: the player gives those)
# Federation raids on the Klingons: one of eight attacks every 60 s, the first at 180 s
every timer fed-raid-kli
  timer fed-raid-kli 60
  random fed-raid-kli-1 fed-raid-kli-2 fed-raid-kli-3 fed-raid-kli-4 fed-raid-kli-5 fed-raid-kli-6 fed-raid-kli-7 fed-raid-kli-8
every signal fed-raid-kli-1
  ai federation fed-assault-kli-random1
every signal fed-raid-kli-2
  ai federation fed-assault-kli-random2
every signal fed-raid-kli-3
  ai federation fed-assault-kli-random3
every signal fed-raid-kli-4
  ai federation fed-assault-kli-random4
every signal fed-raid-kli-5
  ai federation fed-assault-kli-random5
every signal fed-raid-kli-6
  ai federation fed-assault-kli-random6
every signal fed-raid-kli-7
  ai federation fed-assault-kli-random7
every signal fed-raid-kli-8
  ai federation fed-assault-kli-random8

# Federation raids on the Romulans: one of eight attacks every 60 s, the first at 180 s
every timer fed-raid-rom
  timer fed-raid-rom 60
  random fed-raid-rom-1 fed-raid-rom-2 fed-raid-rom-3 fed-raid-rom-4 fed-raid-rom-5 fed-raid-rom-6 fed-raid-rom-7 fed-raid-rom-8
every signal fed-raid-rom-1
  ai federation fed-assault-rom-random1
every signal fed-raid-rom-2
  ai federation fed-assault-rom-random2
every signal fed-raid-rom-3
  ai federation fed-assault-rom-random3
every signal fed-raid-rom-4
  ai federation fed-assault-rom-random4
every signal fed-raid-rom-5
  ai federation fed-assault-rom-random5
every signal fed-raid-rom-6
  ai federation fed-assault-rom-random6
every signal fed-raid-rom-7
  ai federation fed-assault-rom-random7
every signal fed-raid-rom-8
  ai federation fed-assault-rom-random8

# Klingon raids on the Federation: one of eight attacks every 60 s, the first at 180 s
every timer kli-raid-fed
  timer kli-raid-fed 60
  random kli-raid-fed-1 kli-raid-fed-2 kli-raid-fed-3 kli-raid-fed-4 kli-raid-fed-5 kli-raid-fed-6 kli-raid-fed-7 kli-raid-fed-8
every signal kli-raid-fed-1
  ai klingon kli-assault-fed-random1
every signal kli-raid-fed-2
  ai klingon kli-assault-fed-random2
every signal kli-raid-fed-3
  ai klingon kli-assault-fed-random3
every signal kli-raid-fed-4
  ai klingon kli-assault-fed-random4
every signal kli-raid-fed-5
  ai klingon kli-assault-fed-random5
every signal kli-raid-fed-6
  ai klingon kli-assault-fed-random6
every signal kli-raid-fed-7
  ai klingon kli-assault-fed-random7
every signal kli-raid-fed-8
  ai klingon kli-assault-fed-random8

# Klingon raids on the Romulans: one of eight attacks every 60 s, the first at 180 s
every timer kli-raid-rom
  timer kli-raid-rom 60
  random kli-raid-rom-1 kli-raid-rom-2 kli-raid-rom-3 kli-raid-rom-4 kli-raid-rom-5 kli-raid-rom-6 kli-raid-rom-7 kli-raid-rom-8
every signal kli-raid-rom-1
  ai klingon kli-assault-rom-random1
every signal kli-raid-rom-2
  ai klingon kli-assault-rom-random2
every signal kli-raid-rom-3
  ai klingon kli-assault-rom-random3
every signal kli-raid-rom-4
  ai klingon kli-assault-rom-random4
every signal kli-raid-rom-5
  ai klingon kli-assault-rom-random5
every signal kli-raid-rom-6
  ai klingon kli-assault-rom-random6
every signal kli-raid-rom-7
  ai klingon kli-assault-rom-random7
every signal kli-raid-rom-8
  ai klingon kli-assault-rom-random8

# Taubat raids on the Federation: one of eight attacks every 60 s, the first at 180 s
every timer tau-raid-fed
  timer tau-raid-fed 60
  random tau-raid-fed-1 tau-raid-fed-2 tau-raid-fed-3 tau-raid-fed-4 tau-raid-fed-5 tau-raid-fed-6 tau-raid-fed-7 tau-raid-fed-8
every signal tau-raid-fed-1
  ai taubat taubat-assault-fed-random1
every signal tau-raid-fed-2
  ai taubat taubat-assault-fed-random2
every signal tau-raid-fed-3
  ai taubat taubat-assault-fed-random3
every signal tau-raid-fed-4
  ai taubat taubat-assault-fed-random4
every signal tau-raid-fed-5
  ai taubat taubat-assault-fed-random5
every signal tau-raid-fed-6
  ai taubat taubat-assault-fed-random6
every signal tau-raid-fed-7
  ai taubat taubat-assault-fed-random7
every signal tau-raid-fed-8
  ai taubat taubat-assault-fed-random8

# Taubat raids on the Klingons: one of eight attacks every 60 s, the first at 180 s
every timer tau-raid-kli
  timer tau-raid-kli 60
  random tau-raid-kli-1 tau-raid-kli-2 tau-raid-kli-3 tau-raid-kli-4 tau-raid-kli-5 tau-raid-kli-6 tau-raid-kli-7 tau-raid-kli-8
every signal tau-raid-kli-1
  ai taubat taubat-assault-kli-random1
every signal tau-raid-kli-2
  ai taubat taubat-assault-kli-random2
)",
R"(every signal tau-raid-kli-3
  ai taubat taubat-assault-kli-random3
every signal tau-raid-kli-4
  ai taubat taubat-assault-kli-random4
every signal tau-raid-kli-5
  ai taubat taubat-assault-kli-random5
every signal tau-raid-kli-6
  ai taubat taubat-assault-kli-random6
every signal tau-raid-kli-7
  ai taubat taubat-assault-kli-random7
every signal tau-raid-kli-8
  ai taubat taubat-assault-kli-random8

# Taubat raids on the Romulans: one of eight attacks every 60 s, the first at 180 s
every timer tau-raid-rom
  timer tau-raid-rom 60
  random tau-raid-rom-1 tau-raid-rom-2 tau-raid-rom-3 tau-raid-rom-4 tau-raid-rom-5 tau-raid-rom-6 tau-raid-rom-7 tau-raid-rom-8
every signal tau-raid-rom-1
  ai taubat taubat-assault-rom-random1
every signal tau-raid-rom-2
  ai taubat taubat-assault-rom-random2
every signal tau-raid-rom-3
  ai taubat taubat-assault-rom-random3
every signal tau-raid-rom-4
  ai taubat taubat-assault-rom-random4
every signal tau-raid-rom-5
  ai taubat taubat-assault-rom-random5
every signal tau-raid-rom-6
  ai taubat taubat-assault-rom-random6
every signal tau-raid-rom-7
  ai taubat taubat-assault-rom-random7
every signal tau-raid-rom-8
  ai taubat taubat-assault-rom-random8

# Metar raids on the Federation: one of eight attacks every 60 s, the first at 300 s
every timer met-raid-fed
  timer met-raid-fed 60
  random met-raid-fed-1 met-raid-fed-2 met-raid-fed-3 met-raid-fed-4 met-raid-fed-5 met-raid-fed-6 met-raid-fed-7 met-raid-fed-8
every signal met-raid-fed-1
  ai metar metar-assault-fed-random1
every signal met-raid-fed-2
  ai metar metar-assault-fed-random2
every signal met-raid-fed-3
  ai metar metar-assault-fed-random3
every signal met-raid-fed-4
  ai metar metar-assault-fed-random4
every signal met-raid-fed-5
  ai metar metar-assault-fed-random5
every signal met-raid-fed-6
  ai metar metar-assault-fed-random6
every signal met-raid-fed-7
  ai metar metar-assault-fed-random7
every signal met-raid-fed-8
  ai metar metar-assault-fed-random8

# Metar raids on the Klingons: one of eight attacks every 60 s, the first at 300 s
every timer met-raid-kli
  timer met-raid-kli 60
  random met-raid-kli-1 met-raid-kli-2 met-raid-kli-3 met-raid-kli-4 met-raid-kli-5 met-raid-kli-6 met-raid-kli-7 met-raid-kli-8
every signal met-raid-kli-1
  ai metar metar-assault-kli-random1
every signal met-raid-kli-2
  ai metar metar-assault-kli-random2
every signal met-raid-kli-3
  ai metar metar-assault-kli-random3
every signal met-raid-kli-4
  ai metar metar-assault-kli-random4
every signal met-raid-kli-5
  ai metar metar-assault-kli-random5
every signal met-raid-kli-6
  ai metar metar-assault-kli-random6
every signal met-raid-kli-7
  ai metar metar-assault-kli-random7
every signal met-raid-kli-8
  ai metar metar-assault-kli-random8

# Metar raids on the Romulans: one of eight attacks every 60 s, the first at 300 s
every timer met-raid-rom
  timer met-raid-rom 60
  random met-raid-rom-1 met-raid-rom-2 met-raid-rom-3 met-raid-rom-4 met-raid-rom-5 met-raid-rom-6 met-raid-rom-7 met-raid-rom-8
every signal met-raid-rom-1
  ai metar metar-assault-rom-random1
every signal met-raid-rom-2
  ai metar metar-assault-rom-random2
every signal met-raid-rom-3
  ai metar metar-assault-rom-random3
every signal met-raid-rom-4
  ai metar metar-assault-rom-random4
every signal met-raid-rom-5
  ai metar metar-assault-rom-random5
every signal met-raid-rom-6
  ai metar metar-assault-rom-random6
every signal met-raid-rom-7
  ai metar metar-assault-rom-random7
every signal met-raid-rom-8
  ai metar metar-assault-rom-random8

# Metar orders on a clock
on timer met-assault-federation
  ai metar met-assault-federation
on timer met-assault-klingon
  ai metar met-assault-klingon
on timer met-assault-romulan
  ai metar met-assault-romulan

# Not expressible yet:
# - Fail 14 (our hub destroyed) is never given in the game: the Romulan hub
#   monitor's target list runs two names together, so its fail relay (and the
#   first build-abort relay) never get the message. Left out here to match;
#   only the announcement to all sides happens.
# - The Hubrin guides walk to each colony on their own orders (unit orders of
#   the Hubrin side, not started by the script), and the AI sides build on
#   their own timetable and stop when their hub falls.
# - Messages caused by the other sides: their mission passed/failed
#   announcements (sent to every side). There's no event for a rival's pass.

# --- R12 as R ----------------------------------------
# R12 as the Romulans, rewritten from the game's own mission graph (STNW.exe).
mission R12 R
site device-site 184 359
site field-fault 186 357
zone storm-field 188 268 196 276
objective 12
objective 14

# Opening: the Taubat and the Metar each raid us at random every minute
# from the first; three set Metar assaults at 1, 2 and 3 minutes. A storm
# flashes over a field to the west all mission long.
on start
  storm storm-field
  timer taubat-raids 60
  timer metar-raids 60
  timer metar-assault-1 60
  timer metar-assault-3 120
  timer metar-assault-2 180
on timer metar-assault-1
  ai metar met-assault-romulan-1
on timer metar-assault-3
  ai metar met-assault-romulan-3
on timer metar-assault-2
  ai metar met-assault-romulan-2
on destroyed hubrin-observer
  say intelligence "The Hubrin observer is dead."

# Objective 12: the colony three times its starting size, plus four vehicle
# yards and five power generators (or three advanced ones); then the
# containment device's last research opens.
on built-all vehicle-yard*4 power-generator*5|advanced-generator*3
  signal infrastructure
on colony-size 300
  signal infrastructure
on signal infrastructure*2
  say engineering "Sir, the colony can now support building the containment device. One last round of research diagnostics is needed."
  complete 12
  research cfd

# The device: made by the script 10 s after the research and beamed in at
# the site the Hubrin marked; once it stands, the Metar go for it and a
# Hubrin saboteur is beamed in beside it.
on researched cfd
  timer device-made 10
on timer device-made
  spawn us containment-device 1 184 359 containment-device
  say engineering "It's built. We're taking the device to the site the Hubrin marked, to the south."
on standing containment-device 1
  say engineering "Sir, transport is complete: we have the device for the field's containment in position."
  ai metar metar-attack-on-cfd
  spawn hubrin hubrin 1 194 353 hubrin-saboteur
  timer saboteur-1 10
on timer saboteur-1
  say intelligence "Sir, another Hubrin has shown up by the device. It says a construction error nobody foresaw has unbalanced the field."
  timer saboteur-2 10
on timer saboteur-2
  say engineering "Our remote scan agrees. A science vehicle has to get close to isolate the imbalance."
  timer fault-scan-ready 10
on timer fault-scan-ready
  arm fault-scan

# Objective 14: the fault scanned (a watch asleep until 10 s after the line
# above) and fixed 20 s later, and the device standing for 10 minutes.
on scanned science field-fault as fault-scan dormant
  say science "Trying to isolate the imbalance. Stand by..."
  timer recalibrate 20
on timer recalibrate
  say engineering "Error located. Now recalibrating the device for the containment field..."
  signal device-online
on standing containment-device 600
  signal device-online
on signal device-online*2
  say engineering "A close call, commander. As first set up, it would soon have overloaded the whole system."
  complete 14

# Fails: the cause at once, "the Romulans cannot complete their mission" at
# 10 s, the end at 20 s.
on destroyed colony-hub
  fail 13 20
  say intelligence "The Romulan hub has been destroyed."
  signal failing
on destroyed containment-device
  fail 15 20
  say security "Metar forces have wrecked the field containment device!"
  signal failing
on signal failing
  timer fail-message 10
on timer fail-message
  say intelligence "The Romulans can't complete their mission."

# The pass: both objectives, the message at once (to every side), the win
# 10 s later.
on all-completed
  say intelligence "The Romulans' mission is accomplished."
  timer mission-won 10
on timer mission-won
  win

# The raids: every minute, one of eight at random, from each side.
on timer taubat-raids
  timer taubat-raid 60
every timer taubat-raid
  random taubat-raid-1 taubat-raid-2 taubat-raid-3 taubat-raid-4 taubat-raid-5 taubat-raid-6 taubat-raid-7 taubat-raid-8
  timer taubat-raid 60
every signal taubat-raid-1
  ai taubat taubat-assault-rom-random1
every signal taubat-raid-2
  ai taubat taubat-assault-rom-random2
every signal taubat-raid-3
  ai taubat taubat-assault-rom-random3
every signal taubat-raid-4
  ai taubat taubat-assault-rom-random4
every signal taubat-raid-5
  ai taubat taubat-assault-rom-random5
every signal taubat-raid-6
  ai taubat taubat-assault-rom-random6
every signal taubat-raid-7
  ai taubat taubat-assault-rom-random7
every signal taubat-raid-8
  ai taubat taubat-assault-rom-random8
on timer metar-raids
  timer metar-raid 60
every timer metar-raid
  random metar-raid-1 metar-raid-2 metar-raid-3 metar-raid-4 metar-raid-5 metar-raid-6 metar-raid-7 metar-raid-8
  timer metar-raid 60
every signal metar-raid-1
  ai metar metar-assault-rom-random1
every signal metar-raid-2
  ai metar metar-assault-rom-random2
every signal metar-raid-3
  ai metar metar-assault-rom-random3
every signal metar-raid-4
  ai metar metar-assault-rom-random4
every signal metar-raid-5
  ai metar metar-assault-rom-random5
every signal metar-raid-6
  ai metar metar-assault-rom-random6
every signal metar-raid-7
  ai metar metar-assault-rom-random7
every signal metar-raid-8
  ai metar metar-assault-rom-random8

# Not expressible yet:
# - The device spawner's own line ("construction complete") is sent when its
#   group is gone; the recording heard it at once, as written here.
# - The fault scan is of the cell 186:357, of the kind the game's first scan
#   bit records (taken to be science).
# - The game's hub fail sends "Fail Crieria 1", misspelt, so fail string 13
#   is never ticked (the mission still fails); written as fail 13.
# - Messages sent to every side's mail (the pass, the fail, the observer).

# --- J8 as R ----------------------------------------
# J8 as the Romulans (slot 2), from the game's own mission graph.
# Research and build the anomaly device, then get it to the detonation site
# while keeping the vehicle yard and the device itself alive.
mission J8 R
zone detonation-site 252 307 260 315
objective 13
objective 15
objective 17 hidden

# Fail criteria, the game's way: the cause at once, the end 10 s later (no
# word to the other sides in J8). Our hub (14), our last vehicle yard (16),
# and the device (18), watched only once it's built (see objective 13).
on destroyed colony-hub
  fail 14 10
  say command "Colony hub lost. The mission has failed."
on count vehicle-yard == 0
  fail 16 10
  say command "Vehicle yard lost. The mission has failed."
on destroyed anomaly-device as device-watch dormant
  fail 18 10
  say command "The anomaly device has been destroyed. The mission has failed."

# Start: the device research is open from the start (nothing in the graph
# unlocks it); the Metar starship flies its patrol loop; the other sides'
# clocks
on start
  research anomaly-device
  move met-starship 202 237 276 410 178 359 202 237 loop
  timer fed-raid-kli 90
  timer fed-raid-rom 90
  timer federation-colony-protection-relay 16
  timer kli-raid-fed 90
  timer kli-raid-rom 90
  timer met-raid-fed 120
  timer met-raid-kli 120
  timer met-raid-rom 120
  timer metar-colony-protection-relay-one 16
  timer metar-colony-protection-relay-two 16
  timer metar-colony-protection-relay-three 16
  timer metar-colony-protection-relay-four 16
  timer metar-federation-attack 960
  timer metar-klingon-attack 960
  timer metar-romulan-attack 960

# Objective 13: the research done, the device built; a minute after the
# research our device is given its route to the site
on researched anomaly-device
  say science "Our researchers have worked out how to build the device."
  timer device-route 60
on timer device-route
  ai romulan rom-anomaly-device-controller
# (Objective 17 is hidden until the device is built.)
on built anomaly-device
  say engineering "Our anomaly device is built and setting off. Guard it with everything we have."
  arm device-watch
  complete 13
  reveal 17

# Objectives 15 and 17: the device inside the rectangle at the site
on inside anomaly-device detonation-site
  say engineering "The device is at its target and starting up. We've done it."
  complete 15
  complete 17

# Mission passed: the win 10 s later (no announcement in J8)
on all-completed
  timer pass 10
on timer pass
  win

# The other sides' AI: orders on a clock (the Romulans' own attack orders
# are left out: the player gives those)
# Federation raids on the Klingons: one of eight attacks every 30 s, the first at 90 s
every timer fed-raid-kli
  timer fed-raid-kli 30
  random fed-raid-kli-1 fed-raid-kli-2 fed-raid-kli-3 fed-raid-kli-4 fed-raid-kli-5 fed-raid-kli-6 fed-raid-kli-7 fed-raid-kli-8
every signal fed-raid-kli-1
  ai federation fed-assault-kli-random1
every signal fed-raid-kli-2
  ai federation fed-assault-kli-random2
every signal fed-raid-kli-3
  ai federation fed-assault-kli-random3
every signal fed-raid-kli-4
  ai federation fed-assault-kli-random4
every signal fed-raid-kli-5
  ai federation fed-assault-kli-random5
every signal fed-raid-kli-6
  ai federation fed-assault-kli-random6
every signal fed-raid-kli-7
  ai federation fed-assault-kli-random7
every signal fed-raid-kli-8
  ai federation fed-assault-kli-random8

# Federation raids on the Romulans: one of eight attacks every 30 s, the first at 90 s
every timer fed-raid-rom
  timer fed-raid-rom 30
  random fed-raid-rom-1 fed-raid-rom-2 fed-raid-rom-3 fed-raid-rom-4 fed-raid-rom-5 fed-raid-rom-6 fed-raid-rom-7 fed-raid-rom-8
every signal fed-raid-rom-1
  ai federation fed-assault-rom-random1
every signal fed-raid-rom-2
  ai federation fed-assault-rom-random2
every signal fed-raid-rom-3
  ai federation fed-assault-rom-random3
every signal fed-raid-rom-4
  ai federation fed-assault-rom-random4
every signal fed-raid-rom-5
  ai federation fed-assault-rom-random5
every signal fed-raid-rom-6
  ai federation fed-assault-rom-random6
every signal fed-raid-rom-7
  ai federation fed-assault-rom-random7
every signal fed-raid-rom-8
  ai federation fed-assault-rom-random8

# Federation orders on a clock
on timer federation-colony-protection-relay
  ai federation federation-colony-protection-one
  ai federation federation-colony-protection-two
  ai federation federation-colony-protection-three
)",
R"(  ai federation federation-colony-protection-four

# Klingon raids on the Federation: one of eight attacks every 30 s, the first at 90 s
every timer kli-raid-fed
  timer kli-raid-fed 30
  random kli-raid-fed-1 kli-raid-fed-2 kli-raid-fed-3 kli-raid-fed-4 kli-raid-fed-5 kli-raid-fed-6 kli-raid-fed-7 kli-raid-fed-8
every signal kli-raid-fed-1
  ai klingon kli-assault-fed-random1
every signal kli-raid-fed-2
  ai klingon kli-assault-fed-random2
every signal kli-raid-fed-3
  ai klingon kli-assault-fed-random3
every signal kli-raid-fed-4
  ai klingon kli-assault-fed-random4
every signal kli-raid-fed-5
  ai klingon kli-assault-fed-random5
every signal kli-raid-fed-6
  ai klingon kli-assault-fed-random6
every signal kli-raid-fed-7
  ai klingon kli-assault-fed-random7
every signal kli-raid-fed-8
  ai klingon kli-assault-fed-random8

# Klingon raids on the Romulans: one of eight attacks every 30 s, the first at 90 s
every timer kli-raid-rom
  timer kli-raid-rom 30
  random kli-raid-rom-1 kli-raid-rom-2 kli-raid-rom-3 kli-raid-rom-4 kli-raid-rom-5 kli-raid-rom-6 kli-raid-rom-7 kli-raid-rom-8
every signal kli-raid-rom-1
  ai klingon kli-assault-rom-random1
every signal kli-raid-rom-2
  ai klingon kli-assault-rom-random2
every signal kli-raid-rom-3
  ai klingon kli-assault-rom-random3
every signal kli-raid-rom-4
  ai klingon kli-assault-rom-random4
every signal kli-raid-rom-5
  ai klingon kli-assault-rom-random5
every signal kli-raid-rom-6
  ai klingon kli-assault-rom-random6
every signal kli-raid-rom-7
  ai klingon kli-assault-rom-random7
every signal kli-raid-rom-8
  ai klingon kli-assault-rom-random8

# Metar raids on the Federation: one of eight attacks every 60 s, the first at 120 s
every timer met-raid-fed
  timer met-raid-fed 60
  random met-raid-fed-1 met-raid-fed-2 met-raid-fed-3 met-raid-fed-4 met-raid-fed-5 met-raid-fed-6 met-raid-fed-7 met-raid-fed-8
every signal met-raid-fed-1
  ai metar metar-assault-fed-random1
every signal met-raid-fed-2
  ai metar metar-assault-fed-random2
every signal met-raid-fed-3
  ai metar metar-assault-fed-random3
every signal met-raid-fed-4
  ai metar metar-assault-fed-random4
every signal met-raid-fed-5
  ai metar metar-assault-fed-random5
every signal met-raid-fed-6
  ai metar metar-assault-fed-random6
every signal met-raid-fed-7
  ai metar metar-assault-fed-random7
every signal met-raid-fed-8
  ai metar metar-assault-fed-random8

# Metar raids on the Klingons: one of eight attacks every 60 s, the first at 120 s
every timer met-raid-kli
  timer met-raid-kli 60
  random met-raid-kli-1 met-raid-kli-2 met-raid-kli-3 met-raid-kli-4 met-raid-kli-5 met-raid-kli-6 met-raid-kli-7 met-raid-kli-8
every signal met-raid-kli-1
  ai metar metar-assault-kli-random1
every signal met-raid-kli-2
  ai metar metar-assault-kli-random2
every signal met-raid-kli-3
  ai metar metar-assault-kli-random3
every signal met-raid-kli-4
  ai metar metar-assault-kli-random4
every signal met-raid-kli-5
  ai metar metar-assault-kli-random5
every signal met-raid-kli-6
  ai metar metar-assault-kli-random6
every signal met-raid-kli-7
  ai metar metar-assault-kli-random7
every signal met-raid-kli-8
  ai metar metar-assault-kli-random8

# Metar raids on the Romulans: one of eight attacks every 60 s, the first at 120 s
every timer met-raid-rom
  timer met-raid-rom 60
  random met-raid-rom-1 met-raid-rom-2 met-raid-rom-3 met-raid-rom-4 met-raid-rom-5 met-raid-rom-6 met-raid-rom-7 met-raid-rom-8
every signal met-raid-rom-1
  ai metar metar-assault-rom-random1
every signal met-raid-rom-2
  ai metar metar-assault-rom-random2
every signal met-raid-rom-3
  ai metar metar-assault-rom-random3
every signal met-raid-rom-4
  ai metar metar-assault-rom-random4
every signal met-raid-rom-5
  ai metar metar-assault-rom-random5
every signal met-raid-rom-6
  ai metar metar-assault-rom-random6
every signal met-raid-rom-7
  ai metar metar-assault-rom-random7
every signal met-raid-rom-8
  ai metar metar-assault-rom-random8

# Metar orders on a clock
on timer metar-colony-protection-relay-one
  ai metar metar-colony-protection-one
  ai metar metar-colony-protection-two
  ai metar metar-colony-protection-three
  ai metar metar-colony-protection-four
on timer metar-colony-protection-relay-two
  ai metar metar-colony-protection-five
  ai metar metar-colony-protection-six
  ai metar metar-colony-protection-seven
  ai metar metar-colony-protection-eight
on timer metar-colony-protection-relay-three
  ai metar metar-colony-protection-nine
  ai metar metar-colony-protection-ten
  ai metar metar-colony-protection-eleven
  ai metar metar-colony-protection-twelve
on timer metar-colony-protection-relay-four
  ai metar metar-colony-protection-thirteen
on timer metar-federation-attack
  ai metar metar-federation-assault-one
  ai metar metar-federation-assault-two
on timer metar-klingon-attack
  ai metar metar-klingon-assault-one
  ai metar metar-klingon-assault-two
on timer metar-romulan-attack
  ai metar metar-romulan-assault-one
  ai metar metar-romulan-assault-two

# Not expressible yet:
# - The rivals' devices: their research, building and route orders (60 s
#   after their research) happen on their side; there's no event for a
#   rival's research.
# - Objective 15 (protect the vehicle yard) is met only when the device
#   arrives, together with 17; the game words it as its own objective but
#   checks nothing else.

# --- R14 as R ----------------------------------------
# R14 as the Romulans, rewritten from the game's own mission graph (STNW.exe).
mission R14 R
site metar-core 82 269
objective 12
objective 14

# Opening: Taubat raids every 30 s from the first 30 s; Metar raids every
# 100 s after 2 minutes; set Metar assaults at 2, 5 and 9 minutes, and an
# attack on the containment device at 11 minutes.
on start
  timer taubat-raids 30
  timer metar-raids 120
  timer metar-assault-1 120
  timer metar-assault-2 300
  timer metar-assault-3 540
  timer metar-device-attack 660
on timer metar-assault-1
  ai metar met-assault-romulan-1
on timer metar-assault-2
  ai metar met-assault-romulan-2
on timer metar-assault-3
  ai metar met-assault-romulan-3
on timer metar-device-attack
  ai metar metar-attack-on-cfd

# Objectives 12 and 14: the scan of the Metar core meets both.
on scanned science metar-core
  say science "Sir, these readings suggest the Metar are making an anomaly generator."
  complete 12
  complete 14

# Fails: the cause at once, "you have failed" at 10 s, the end at 20 s.
on destroyed colony-hub
  fail 13 20
  say operations "The colony hub is destroyed."
  signal failing
on destroyed containment-device
  fail 15 20
  say security "The Metar have wrecked the containment field device."
  signal failing
on signal failing
  timer fail-message 10
on timer fail-message
  say intelligence "You have not completed your mission."

# The pass: the message 10 s later (to every side), the win at 20 s.
on all-completed
  timer mission-pass 10
  timer mission-won 20
on timer mission-pass
  say intelligence "Your mission is complete."
on timer mission-won
  win

# The raids: one of eight at random each time.
on timer taubat-raids
  timer taubat-raid 30
every timer taubat-raid
  random taubat-raid-1 taubat-raid-2 taubat-raid-3 taubat-raid-4 taubat-raid-5 taubat-raid-6 taubat-raid-7 taubat-raid-8
  timer taubat-raid 30
every signal taubat-raid-1
  ai taubat taubat-assault-rom-random1
every signal taubat-raid-2
  ai taubat taubat-assault-rom-random2
every signal taubat-raid-3
  ai taubat taubat-assault-rom-random3
every signal taubat-raid-4
  ai taubat taubat-assault-rom-random4
every signal taubat-raid-5
  ai taubat taubat-assault-rom-random5
every signal taubat-raid-6
  ai taubat taubat-assault-rom-random6
every signal taubat-raid-7
  ai taubat taubat-assault-rom-random7
every signal taubat-raid-8
  ai taubat taubat-assault-rom-random8
on timer metar-raids
  timer metar-raid 100
every timer metar-raid
  random metar-raid-1 metar-raid-2 metar-raid-3 metar-raid-4 metar-raid-5 metar-raid-6 metar-raid-7 metar-raid-8
  timer metar-raid 100
every signal metar-raid-1
  ai metar metar-assault-rom-random1
every signal metar-raid-2
  ai metar metar-assault-rom-random2
every signal metar-raid-3
  ai metar metar-assault-rom-random3
every signal metar-raid-4
  ai metar metar-assault-rom-random4
every signal metar-raid-5
  ai metar metar-assault-rom-random5
every signal metar-raid-6
  ai metar metar-assault-rom-random6
every signal metar-raid-7
  ai metar metar-assault-rom-random7
every signal metar-raid-8
  ai metar metar-assault-rom-random8

# Not expressible yet:
# - The core scan is a cell-scan monitor on 82:269 (the game's first scan
#   bit, taken to be science; weak or strong not told apart).
# - Messages sent to every side's mail (the pass and the fails).

# --- J9 as R ----------------------------------------
# J9 as the Romulans (slot 2), from the game's own mission graph.
# Scan the Metar temple for the sentient anomaly device, research it, and
# capture one of the Metar starships.
mission J9 R
site metar-temple 264 303
objective 13
objective 15

# Fail criteria, the game's way: the cause at once, the end 10 s later (no
# word to the other sides in J9). Our hub (14); all three Metar starships
# destroyed (16; the game's message calls them colony cores). Fail 17 (a
# rival got the technology while we were scanning) is with the scan below.
on destroyed colony-hub
  fail 14 10
  say command "Colony hub lost. The mission has failed."
on count metar-starship == 0
  fail 16 10
  say command "The Metar colony cores are all gone. The mission has failed."

# Start: the other sides' attack clocks
on start
  timer fed-raid-kli 60
  timer fed-raid-rom 60
  timer kli-raid-fed 60
  timer met-raid-fed 220
  timer met-raid-kli 220
  timer met-raid-rom 220

# Objective 13: scan the temple (10 s before the officer reports, then held
# a minute); the technology is beamed home and its research opens; the
# research done meets the objective. The game's watch is "the device's block
# scanned by us" (any of our scanning units nearby), on the group
# Anomaly_Device: a cell scan.
every scan-started cell anomaly-device
  say engineering "Searching the temple for the device's technology."
every scan-lost cell anomaly-device
  say engineering "We lost the scan. Have another try."
on scanned cell anomaly-device
  say engineering "The technology is beamed home to our hub for research."
  research anomaly-device
# The race for the device: the first side to finish takes the technology. A
# rival finishing while ours runs beats us (fail 17); once a rival has
# finished, our later scan is futile (no technology, no research). The
# computer's sides only win it if their own units happen to scan the block.
on beaten anomaly-device
  fail 17 10
  say engineering "The technology is gone from here; the Federation or the Klingons must have taken it."
every futile anomaly-device
  say engineering "That scan was useless."
on researched anomaly-device
  say science "Our researchers have worked out how to use the device."
  complete 13
  arm starship-watch

# Objective 15: holding a Metar starship; the game starts watching for it
# only once the research is done
on captured metar-starship as starship-watch dormant
  say engineering "The starship is ours."
  complete 15

# Mission passed: the win 10 s later (no announcement in J9)
on all-completed
  timer pass 10
on timer pass
  win

# The other sides' AI: attacks on a clock (the Romulans' own attack orders
# are left out: the player gives those)
# Federation raids on the Klingons: one of eight attacks every 30 s, the first at 60 s
every timer fed-raid-kli
  timer fed-raid-kli 30
  random fed-raid-kli-1 fed-raid-kli-2 fed-raid-kli-3 fed-raid-kli-4 fed-raid-kli-5 fed-raid-kli-6 fed-raid-kli-7 fed-raid-kli-8
every signal fed-raid-kli-1
  ai federation fed-assault-kli-random1
every signal fed-raid-kli-2
  ai federation fed-assault-kli-random2
every signal fed-raid-kli-3
  ai federation fed-assault-kli-random3
every signal fed-raid-kli-4
  ai federation fed-assault-kli-random4
every signal fed-raid-kli-5
  ai federation fed-assault-kli-random5
every signal fed-raid-kli-6
  ai federation fed-assault-kli-random6
every signal fed-raid-kli-7
  ai federation fed-assault-kli-random7
every signal fed-raid-kli-8
  ai federation fed-assault-kli-random8

# Federation raids on the Romulans: one of eight attacks every 30 s, the first at 60 s
every timer fed-raid-rom
  timer fed-raid-rom 30
  random fed-raid-rom-1 fed-raid-rom-2 fed-raid-rom-3 fed-raid-rom-4 fed-raid-rom-5 fed-raid-rom-6 fed-raid-rom-7 fed-raid-rom-8
every signal fed-raid-rom-1
  ai federation fed-assault-rom-random1
every signal fed-raid-rom-2
  ai federation fed-assault-rom-random2
every signal fed-raid-rom-3
  ai federation fed-assault-rom-random3
every signal fed-raid-rom-4
  ai federation fed-assault-rom-random4
every signal fed-raid-rom-5
  ai federation fed-assault-rom-random5
every signal fed-raid-rom-6
  ai federation fed-assault-rom-random6
every signal fed-raid-rom-7
  ai federation fed-assault-rom-random7
every signal fed-raid-rom-8
  ai federation fed-assault-rom-random8

# Klingon raids on the Federation: one of eight attacks every 30 s, the first at 60 s
every timer kli-raid-fed
  timer kli-raid-fed 30
  random kli-raid-fed-1 kli-raid-fed-2 kli-raid-fed-3 kli-raid-fed-4 kli-raid-fed-5 kli-raid-fed-6 kli-raid-fed-7 kli-raid-fed-8
every signal kli-raid-fed-1
  ai klingon kli-assault-fed-random1
every signal kli-raid-fed-2
  ai klingon kli-assault-fed-random2
every signal kli-raid-fed-3
  ai klingon kli-assault-fed-random3
every signal kli-raid-fed-4
  ai klingon kli-assault-fed-random4
every signal kli-raid-fed-5
  ai klingon kli-assault-fed-random5
every signal kli-raid-fed-6
  ai klingon kli-assault-fed-random6
every signal kli-raid-fed-7
  ai klingon kli-assault-fed-random7
every signal kli-raid-fed-8
  ai klingon kli-assault-fed-random8

# Metar raids on the Federation: one of eight attacks every 100 s, the first at 220 s
every timer met-raid-fed
  timer met-raid-fed 100
  random met-raid-fed-1 met-raid-fed-2 met-raid-fed-3 met-raid-fed-4 met-raid-fed-5 met-raid-fed-6 met-raid-fed-7 met-raid-fed-8
every signal met-raid-fed-1
  ai metar metar-assault-fed-random1
every signal met-raid-fed-2
  ai metar metar-assault-fed-random2
every signal met-raid-fed-3
  ai metar metar-assault-fed-random3
every signal met-raid-fed-4
  ai metar metar-assault-fed-random4
every signal met-raid-fed-5
  ai metar metar-assault-fed-random5
every signal met-raid-fed-6
  ai metar metar-assault-fed-random6
every signal met-raid-fed-7
  ai metar metar-assault-fed-random7
every signal met-raid-fed-8
  ai metar metar-assault-fed-random8

# Metar raids on the Klingons: one of eight attacks every 100 s, the first at 220 s
every timer met-raid-kli
  timer met-raid-kli 100
  random met-raid-kli-1 met-raid-kli-2 met-raid-kli-3 met-raid-kli-4 met-raid-kli-5 met-raid-kli-6 met-raid-kli-7 met-raid-kli-8
)",
R"(every signal met-raid-kli-1
  ai metar metar-assault-kli-random1
every signal met-raid-kli-2
  ai metar metar-assault-kli-random2
every signal met-raid-kli-3
  ai metar metar-assault-kli-random3
every signal met-raid-kli-4
  ai metar metar-assault-kli-random4
every signal met-raid-kli-5
  ai metar metar-assault-kli-random5
every signal met-raid-kli-6
  ai metar metar-assault-kli-random6
every signal met-raid-kli-7
  ai metar metar-assault-kli-random7
every signal met-raid-kli-8
  ai metar metar-assault-kli-random8

# Metar raids on the Romulans: one of eight attacks every 100 s, the first at 220 s
every timer met-raid-rom
  timer met-raid-rom 100
  random met-raid-rom-1 met-raid-rom-2 met-raid-rom-3 met-raid-rom-4 met-raid-rom-5 met-raid-rom-6 met-raid-rom-7 met-raid-rom-8
every signal met-raid-rom-1
  ai metar metar-assault-rom-random1
every signal met-raid-rom-2
  ai metar metar-assault-rom-random2
every signal met-raid-rom-3
  ai metar metar-assault-rom-random3
every signal met-raid-rom-4
  ai metar metar-assault-rom-random4
every signal met-raid-rom-5
  ai metar metar-assault-rom-random5
every signal met-raid-rom-6
  ai metar metar-assault-rom-random6
every signal met-raid-rom-7
  ai metar metar-assault-rom-random7
every signal met-raid-rom-8
  ai metar metar-assault-rom-random8

# Not expressible yet:
# - The game's capture watch counts starships held, so one captured before
#   the research and still held when the watch starts counts at once; here
#   only a capture made after the research does (the event is the capture).
# - The Klingons' random raids on the Romulans never happen in the game: their
#   timer names orders (Kli_Assault_Kli_Random1-8) that don't exist. Left out.
# - The AI sides' own research and captures (no events for rivals).

# --- C11 as R ----------------------------------------
# C11 "Bonus Romulan Mission" (Romulan, slot 2; map f1, colony at 184:183). Rewritten
# from the game's own graph (S/ghidra/scripts_out/C11.json). Two escape pods of the
# Babarakka lie on the map (Klingon slot 1): the Federation ambassador's (124:291), taken
# by the Hubrin (fortress 292:419, slot 4), and the Klingon ambassador's (320:47), held at
# the Taubat temple (440:231, slot 3, with Orion help). Narok's deadline is an hour.
# Handlers are in the order the recording met them (play-c11.md).
mission C11 R
site colony 184 183
site fed-pod 124 291
site kli-pod 320 47
site hubrin-fortress 292 419
site temple 440 231
zone taubat-trail 272 155 304 215
objective 81
objective 82
objective 83 hidden
objective 84 hidden
objective 85 hidden

# --- Narok's orders and the deadline ---
on start
  timer intro 2
  timer deadline 3600
on timer intro
  say "Incoming message from Narok" "Move fast, Commander: Federation and Klingon ships will soon arrive to look for their missing men."
# Out of time: Narok's verdict at once, the failure 10 s later, over at 20 s.
on timer deadline
  fail 87 20
  say "Incoming transmission from Narok" "Commander, you have not succeeded. Ships of the Federation and the Klingons now orbit Junil IV, and the Senate disowns everything done there: officially, neither you nor your crew exists."
  timer fail-message 10

# --- Any of our units inside the rectangle south of the colony sets off Orion-Taubat
# raids, every 60 s ---
on inside ours taubat-trail
  timer orion-raid 60
every timer orion-raid
  random ori-raid-1 ori-raid-2 ori-raid-3 ori-raid-4
  timer orion-raid 60
every signal ori-raid-1
  ai taubat ori-assault-rom-random1
every signal ori-raid-2
  ai taubat ori-assault-rom-random2
every signal ori-raid-3
  ai taubat ori-assault-rom-random3
every signal ori-raid-4
  ai taubat ori-assault-rom-random4

# --- The Klingon pod: a science scan (2 s to start, held 20 s) ---
on seen kli-pod
  say operations "We've found a pod, Commander. I suggest a science scan."
every scan-started science kli-pod
  say science "The Klingon ambassador isn't here, Commander, but the area is full of energy traces matching Orion and Taubat units."
every scan-lost science kli-pod
  say science "We couldn't finish the scan; we'll have to try again."
on scanned science kli-pod
  say "Incoming transmission from Narok" "Locate him, Commander, or Junil IV becomes your permanent home."
  complete 82
  reveal 84
  signal objective-met

# --- The Federation pod: a medical scan (2 s to start, held 20 s) ---
on seen fed-pod
  say operations "An escape pod from the Babarakka has turned up. Scanning it medically should reveal its occupant."
every scan-started medical fed-pod
  say medical "The pod is empty, but cell traces show a human was aboard. There are strong bi-phasic traces too, with the Hubrin's signature."
every scan-lost medical fed-pod
  say medical "We couldn't finish the scan; we'll have to try again."
on scanned medical fed-pod
  say "Incoming transmission from Narok" "So the Hubrin got there first; no matter. Find their fortress and bargain for the ambassador's release. Be quick, Commander: time is running out."
  complete 81
  reveal 83
  signal objective-met

# --- The temple: a medical scan locks on to the Klingon ambassador ---
on seen temple
  say operations "Sir, medically scanning the temple should let our transporter lock onto the ambassador."
every scan-started medical temple
  say operations "Locking on to the ambassador. Stand by..."
every scan-lost medical temple
  say operations "The ambassador has slipped out of our transporter lock. Another scan is required."
on scanned medical temple
  timer ambassador-aboard 1
  timer narok-crush 5
on timer ambassador-aboard
  say operations "The ambassador is in our hands!"
  complete 84
  signal objective-met
on timer narok-crush
  say "Incoming transmission from Narok" "Let these pitiful curs learn what the Star Empire's power truly means. Leave nothing of them standing!"
  reveal 85

# --- The Orion-Taubat colony razed: temple, houses, hangars and guns all gone ---
on count taubat-temple == 0
  signal taubat-kind-gone
on count taubat-house == 0
  signal taubat-kind-gone
on count taubat-hangar == 0
  signal taubat-kind-gone
on count taubat-gun == 0
  signal taubat-kind-gone
on signal taubat-kind-gone*4
  say "Incoming transmission from Narok" "Well done, Commander. Have the ambassador ready to be collected: my ship reaches orbit in a few hours, and your new posting will follow."
  complete 85
  signal objective-met

# --- The Hubrin refuse; Narok wants them and the Federation ambassador dead ---
on seen hubrin-fortress
  timer hubrin-1 1
  timer hubrin-2 5
  timer hubrin-3 10
  timer hubrin-4 25
  timer hubrin-5 40
  timer hubrin-6 45
on timer hubrin-1
  say communications "We've found the Hubrin. Their fortress stands on what is left of a Metar hive. They're hailing us..."
on timer hubrin-2
  say "Transmission from Hubrin" "Stranger, why do you come near?"
on timer hubrin-3
  say "Incoming transmission from Narok" "Surrender the Federation fugitive to us. That deceiver will pay the Empire of Romulus for what he has done."
on timer hubrin-4
  say "Transmission from Hubrin" "We think this is untrue. The human will go back to his own people, under Hubrin protection."
on timer hubrin-5
  say communications "Commander, the Hubrin have stopped answering."
on timer hubrin-6
  say "Incoming transmission from Narok" "Insolent wretches! Commander, see that the ambassador dies, that the Hubrin are exterminated and that the fortress they cherish is reduced to rubble. Fear of us is a lesson they will learn!"
  hostile hubrin
  hostile metar
  timer hubrin-raid 60
# (each raid order beams its own 3 or 2 Hubrin in near the fortress, at 322:449 or
# 330:437)
every timer hubrin-raid
  random hbr-raid-1 hbr-raid-2 hbr-raid-3 hbr-raid-4
  timer hubrin-raid 60
every signal hbr-raid-1
  ai hubrin hbr-assault-rom-random1
every signal hbr-raid-2
  ai hubrin hbr-assault-rom-random2
every signal hbr-raid-3
  ai hubrin hbr-assault-rom-random3
every signal hbr-raid-4
  ai hubrin hbr-assault-rom-random4
on count hubrin-fortress == 0
  say "Incoming transmission from Narok" "Excellent. The Federation's lackey burns on a pyre he built himself."
  complete 83
  signal objective-met

# --- All five objectives: the pass ---
on signal objective-met*5
  timer pass-message 10
  timer pass 20
on timer pass-message
  say command "All objectives complete. Well done."
on timer pass
  win

# --- The fail: the colony hub destroyed (said at once, the failure 10 s later, over at 20 s;
# the deadline uses the same closing line) ---
on destroyed colony-hub
  fail 86 20
  say command "We have lost the colony hub."
  timer fail-message 10
on timer fail-message
  say command "You have not achieved your objectives."

# Not expressible yet:
# - The postures: Narok's last line also turns the Hubrin (slot 4) and the Metar guns
#   (slot 5) hostile to the Romulans; `hostile <side>` only changes the Romulans' own
#   stance.
# - The Hubrin raiders' spawners make them slot 5's units (the Metar guns' slot), not
#   the Hubrin's; the orders here beam them in for the Hubrin.
# - Scans: the game's conditions are the weak medical / weak science scan flags (4, 5),
#   2 s to start and 20 s to hold; each start or loss re-sends its line (the 1 s timers
#   in front of the lines are folded in). The "start" lines already give the findings.
# - The deadline timer keeps running after the pass (the pass ends the mission first).
# - Both fails share one closing relay: only the first runs the 10 s line and the end
#   (Reluva's `fail` ignores the second too, but its `timer fail-message` restarts the
#   10 s line's timer if the second cause comes within those 10 s).
# - The Taubat and the Hubrin also run patrol orders the game starts by itself.

# --- C1 as F ----------------------------------------
# C1 "Tutorial" (Federation, slot 0; map coverd3). Rewritten from the game's own graph
# (S/ghidra/scripts_out/C1.json). Every line comes from Operations (the training officer)
# with its voice clip, one after another on timers; some steps wait for the player to
# build something or press a key (key codes are the game's, Windows virtual keys:
# 9 Tab, 17 Ctrl, 38 Up arrow, 49 "1", 100/103/104/105 numpad 4/7/8/9, 112 F1, 113 F2).
mission C1 F
site colony 426 286
zone colony-gate 408 280 424 312    # at the colony's south side
zone attack-front 389 259 397 276   # where the Romulan attack comes through
objective 16

# --- Selecting buildings and the first build order (timed from the start) ---
on start
  timer select-1 1
on timer select-1
  say operations "Left-click the buildings one by one: the New Worlds window at the top right shows what each one is and its details." Tutor08
  timer select-2 10
on timer select-2
  say operations "The buttons along the bottom of that window depend on the building. Keep clicking until you have the colony hub."
  timer select-3 20
on timer select-3
  say operations "A right-click drops the selection; pick the hub again. More buildings and vehicles open up as the colony grows." Tutor09
  timer build-1 17
on timer build-1
  say operations "Press Build to see what can be built. Pick the construction yard and move the pointer up the screen, north of the hub." Tutor10
  timer build-2 12
on timer build-2
  say operations "Move it over free concrete until the hologram turns green, then left-click: building starts straight away."

# --- The vehicle yard and a scout (once the construction yard stands) ---
on built construction-yard
  timer vehicles-1 3
on timer vehicles-1
  say operations "Move the pointer down to the lower edge of the screen so the view scrolls past the hub, then select the vehicle yard with a left-click." Tutor11
  timer vehicles-2 13
on timer vehicles-2
  say operations "Now press Build at the foot of the New Worlds panel and page through the vehicles; only the ones you can make are shown." Tutor12
  timer vehicles-3 21
on timer vehicles-3
  say operations "Left-click the scout: it is made at once and drives off to its rally point by itself." Tutor13
  timer vehicles-4 10
on timer vehicles-4
  say operations "Leave Engineering with the X button at the panel's bottom right."
  timer scout-watch 10
# The scout step goes on once a scout exists and the watch has started (both, any order).
on built ground-scout
  signal scout-step
on timer scout-watch
  signal scout-step
on signal scout-step*2
  say operations "Scroll back to the colony hub, or press Home to jump there." Tutor14
  timer upgrade-1 17
on timer upgrade-1
  say operations "Left-click the hub. The lit right arrow at the panel's bottom left means this building can be upgraded." Tutor15
  timer upgrade-2 15
on timer upgrade-2
  say operations "Click the lit arrow to page on, then left-click the improved hub icon: the upgrade starts at once." Tutor16

# --- The Tricorder (once the hub is upgraded); from here on each key detector is armed by
# its prompt and stays deaf until then ---
on built colony-hub-upgrade
  timer tricorder-1 3
on timer tricorder-1
  say operations "Tab switches between the mini-map and the full-screen Tricorder, which takes over the main view from its corner." Tutor17
  arm tab-1
  timer tricorder-2 12
on timer tricorder-2
  say operations "It shows the world you are on as a relief map."
  signal tricorder-step
# The next line needs both: Tab pressed, and the line above shown (the game's two-count relay).
on key 9 as tab-1 dormant
  signal tricorder-step
on signal tricorder-step*2
  timer tricorder-3 12
on timer tricorder-3
  say operations "The Tricorder also colours minerals, terrain and every race's vehicles and buildings. You can play the whole game from it if you like." Tutor18
  timer tricorder-4 19
on timer tricorder-4
  say operations "Press Tab again to go back to the normal camera." Tutor19
  arm tab-2

# --- The camera: each step waits for its key ---
on key 9 as tab-2 dormant
  timer camera-1 3
on timer camera-1
  say operations "This standard view is the bird's eye view. With NumLock on, numpad 4 and 6 turn the camera." Tutor20
  timer camera-2 10
on timer camera-2
  say operations "Numpad 8 and 2 zoom in and out towards wherever the pointer is: targeted zoom."
  arm numpad-8
on key 104 as numpad-8 dormant
  timer camera-3 25
on timer camera-3
  say operations "The four arrow keys move the camera forward, back, left and right." Tutor21
  arm arrow-up
on key 38 as arrow-up dormant
  timer camera-4 15
on timer camera-4
  say operations "F2 lifts the camera much higher, to the sky view; the other controls stay the same." Tutor22
  arm f2-sky
on key 113 as f2-sky dormant
  timer camera-5 26
on timer camera-5
  say operations "There is also control zoom: hold Ctrl and left-click to zoom in on what the pointer is over." Tutor23
  timer camera-6 10
on timer camera-6
  say operations "Ctrl with a right-click zooms back out."
  arm ctrl-zoom
on key 17 as ctrl-zoom dormant
  timer camera-7 22
on timer camera-7
  say operations "Now press F1 for the first-person view." Tutor24
  arm f1-view
)",
R"(on key 112 as f1-view dormant
  timer camera-8 5
on timer camera-8
  say operations "Here the keys work a little differently: with NumLock on, 8 and 2 do targeted zoom, the arrows are unchanged and 4 and 6 turn the view." Tutor25
  timer camera-9 13
on timer camera-9
  say operations "On the numpad, 7 and 1 raise and lower the camera."
  arm numpad-7
on key 103 as numpad-7 dormant
  timer camera-10 23
on timer camera-10
  say operations "And 9 and 3 tip the camera forward and back." Tutor26
  arm numpad-9
on key 105 as numpad-9 dormant
  timer camera-11 23
on timer camera-11
  say operations "In this view, double-clicking a building moves the camera in close and keeps it fixed on that building." Tutor27
  timer camera-12 7
on timer camera-12
  say operations "In the locked view numpad 4 and 6 circle the building; a right-click unlocks it."
  arm numpad-4
on key 100 as numpad-4 dormant
  timer camera-13 26
on timer camera-13
  say operations "Press F2 to get back to the sky view, then Shift+V to go straight to your vehicles." Tutor28
  arm f2-back

# --- Vehicles and battle groups ---
on key 113 as f2-back dormant
  timer group-1 10
on timer group-1
  say operations "Drag a box over the vehicles with the left button and let go: the ones picked show in the details panel." Tutor29
  timer group-2 17
on timer group-2
  say operations "Ctrl+1 makes them battle group 1 (the manual explains groups). Then press Home to go back to the hub." Tutor30
  arm ctrl-group
on key 17 as ctrl-group dormant
  timer group-3 20
on timer group-3
  say operations "Right-click to drop the vehicles and left-click the hub: the details panel now describes the colony." Tutor34
  timer group-4 19
on timer group-4
  say operations "Press 1 to pick the battle group again; Shift+1 takes the camera to it." Tutor35
  arm key-1
on key 49 as key-1 dormant
  timer orders-1 19

# --- Orders and the playbook; the Romulans start moving ---
on timer orders-1
  say operations "With the group selected, left-click somewhere on top of the mountain: the vehicles drive there and defend themselves on the way." Tutor36
  ai romulan rom-scout-fed-colony
  move rom-artillery 370 270
  move rom-artillery-b 370 266
  timer orders-2 18
on timer orders-2
  say operations "Double-click elsewhere on the mountain top to open the playbook; rest the pointer on an icon for a moment to see what it does." Tutor37
  timer orders-3 17
on timer orders-3
  say operations "Scouts and science vehicles have playbooks of their own, but only when they aren't grouped with other kinds of vehicle." Tutor38
  timer orders-4 9
on timer orders-4
  say operations "They can run engineering, science and medical scans, and the science vehicle can take officers along for better scans."
  timer orders-5 15
on timer orders-5
  say operations "Officers can also be put in buildings that use their speciality, to make those buildings work better; the manual tells you how." Tutor39
  timer orders-6 11
on timer orders-6
  say operations "Press Home to return to the hub and 1 to make sure the battle group is selected."
  timer orders-7 16
on timer orders-7
  say operations "Move the pointer round the hub until it becomes a green arrow and click, then wait for the vehicles to gather there. An enemy scout is close to the colony." Tutor31
  timer orders-8 13
on timer orders-8
  say operations "Attack the scout with one of the playbook orders. Never let a scout scan your colony."

# --- The Romulan attack: launched once one of our armed APCs and the Romulan scout have
# reached the colony's south edge ---
on inside armed-apc colony-gate
  signal assault-trigger
on inside romulan colony-gate
  signal assault-trigger
on signal assault-trigger*2
  ai romulan rom-assault-fed-colony
on count romulan-ground-scout == 0
  timer attack-1 1
on timer attack-1
  say operations "Now that the scout is gone, enemies are attacking from the south. Move the pointer close to the incoming vehicles south of your hub, then press the left mouse button." Tutor32
  timer attack-2 10
on timer attack-2
  say operations "Your vehicles close in on the enemy and fire on it once it is in range, by themselves."
  timer attack-3 12
# The last hint needs both: the line above, and Romulans at the attack front.
on timer attack-3
  signal attack-hint
on inside romulan attack-front
  signal attack-hint
on signal attack-hint*2
  say operations "Destroy the attacking units and the tutorial is over. Good luck." Tutor33

# --- The end: both Romulan photon artillery units destroyed ---
on count romulan-photon-artillery == 0
  complete 16
on all-completed
  timer pass-message 5
  timer pass 15
on timer pass-message
  say operations "The Romulan attackers are destroyed and your training is complete. Well done." Tutor40
on timer pass
  win

# --- The fail: the colony hub destroyed (marked at once, told 5 s later, over at 15 s) ---
on destroyed colony-hub
  fail 17 15
  timer fail-message 5
on timer fail-message
  say intelligence "Your colony hub is gone, and without it your training can't be finished." Tutor41

# Not expressible yet:
# - An armed monitor tests the colony as it is, at once: the scout monitor (armed 10 s after
#   the "leave Engineering" line) fires straight away if the scout was built earlier. Reluva's
#   events come only when a count changes, so the step stays an AND of "scout built" and
#   "watch started" (two signals), not a dormant handler.
# - The Romulan area detectors filter by class: at the colony gate only a ground scout
#   (0x4000000), at the attack front only armed vehicles (0x100); "inside romulan" takes any
#   Romulan unit (no "romulan-<kind>" who).
# - The Klingon and Taubat slots only hold dummy objects (they keep those sides alive).

# --- C4 as F ----------------------------------------
# C4 "Building" (Federation, slot 0; map coverd3, colony at 88:415). Rewritten from the
# game's own graph (S/ghidra/scripts_out/C4.json). All lines come from Command, without
# voice clips. Upgrades by Reluva's names: construction-yard-upgrade / -advanced are the
# improved and advanced yard (types 161, 162); processor-plant-a and processor-plant-b are
# the resource processor's two kinds of plant (types 166, 167), built three of each.
mission C4 F
site colony 88 415
objective 25

# --- Opening instructions ---
on start
  timer intro-1 2
  timer intro-2 15
on timer intro-1
  say command "To check your objectives, press Mission on this window to open Command."
on timer intro-2
  say command "Choose the construction yard in the Build menu on the right, or press B. Put the hologram down beside the hub and click when it goes green."

# --- Construction yard, then the resource processor ---
on built construction-yard
  say command "With the construction yard working, the Build menu offers more buildings. Put a resource processor next to the storage facility."
on built resource-processor
  say command "Select the resource processor, go to the next page of the menu with the arrow, and order every upgrade it offers."

# --- The first two plants, then hub and yard upgrades open the rest ---
on built-all processor-plant-a processor-plant-b
  say command "Upgrade the colony hub the same way, and when that's done, the construction yard too."
on built construction-yard-upgrade
  say command "The resource processor has two new upgrades. The last two need a second upgrade of both the hub and the yard."
on built construction-yard-advanced
  say command "The resource processor's last two upgrades can now be built. Build them both to finish this tutorial."

# --- All six plants: the objective ---
on built-all processor-plant-a*3 processor-plant-b*3
  say command "Your resource processor has every upgrade now. Keep in mind that new buildings and upgrades appear only as you improve your hub and your construction yard."
  complete 25
on all-completed
  timer pass-message 10
  timer pass 20
on timer pass-message
  say command "Tutorial 1 is complete. Stand by while tutorial 2 loads."
on timer pass
  win

# --- The fail: the colony hub destroyed (said at once, the failure 10 s later, over at 20 s) ---
on destroyed colony-hub
  fail 26 20
  say command "We have lost the colony hub."
  timer fail-message 10
on timer fail-message
  say command "You have not achieved your objectives."

# Not expressible yet:
# - The plant counts are exact (the game fires when exactly 1, then exactly 3, of each
#   plant type stand); built-all means "at least".

# --- C5 as F ----------------------------------------
# C5 "Mining Resources" (Federation, slot 0; map coverd3, colony at 88:415). Rewritten
# from the game's own graph (S/ghidra/scripts_out/C5.json). All lines come from Command,
# without voice clips. The colony starts short: 100 dilithium, 900 nitrium.
mission C5 F
site colony 88 415
objective 34
objective 35

# --- Opening instructions ---
on start
  timer intro-1 2
  timer intro-2 15
  timer intro-3 25
on timer intro-1
  say command "To check your objectives, press Mission on this window to open Command."
on timer intro-2
  say command "A mining station needs flat ground. Pick it from the Build menu, move the hologram about until it turns green and click to give the order."
on timer intro-3
  say command "On the Tricorder, mineral deposits show up as coloured patches..."

# --- Two mining stations: the first objective ---
on built mining-station
  say command "The first mine is working. Its ore is taken to the resource processor and refined there into materials the colony can use."
on built-all mining-station*2
  say command "Both mining stations are running. Upgrading the resource processor will make the refining go faster."
  complete 34

# --- 200 dilithium and 1500 nitrium in store: the second objective, 10 s after both ---
on stock nitrium 1500
  say command "The nitrium we need has been refined and is in the colony's storage facility."
  signal supplies
on stock dilithium 200
  say command "The dilithium we need has been refined and is in the colony's storage facility."
  signal supplies
on signal supplies*2
  timer supplies-done 10
on timer supplies-done
  say command "Everything we needed has been refined and stored."
  complete 35

# --- Pass ---
on all-completed
  timer pass-message 10
  timer pass 20
on timer pass-message
  say command "Tutorial 2 is complete. Stand by while tutorial 3 loads."
on timer pass
  win

# --- The fail: the colony hub destroyed (said at once, the failure 10 s later, over at 20 s) ---
on destroyed colony-hub
  fail 36 20
  say command "We have lost the colony hub."
  timer fail-message 10
on timer fail-message
  say command "You have not achieved your objectives."

# Not expressible yet:
# - The mine counts are exact (exactly one, then exactly two stations standing).

# --- C6 as F ----------------------------------------
# C6 "Power" (Federation, slot 0; map coverd3, colony at 88:415). Rewritten from the
# game's own graph (S/ghidra/scripts_out/C6.json). All lines come from Command, without
# voice clips. The colony starts with 300 dilithium and 3950 nitrium: enough for four
# generators (800 nitrium each), not five.
mission C6 F
site colony 88 415
objective 44

# --- Opening instruction ---
on start
  timer intro-1 2
on timer intro-1
  say command "To check your objectives, press Mission on this window to open Command."

# --- Four generators use up the nitrium; mining must make up the fifth (the nitrium watch
# only starts then: before, the stock is above 800 from the start) ---
on built-all power-generator*4
  say command "There isn't enough nitrium left for the last generator. Set up mining to refill the colony's stores."
  arm nitrium-up
on stock nitrium 800 as nitrium-up dormant
  say command "There is now enough nitrium for a fifth generator. Build it to finish this part of the tutorial."

# --- Five generators: the objective ---
on built-all power-generator*5
  say command "Every generator we need is up and running."
  complete 44
on all-completed
  timer pass-message 10
  timer pass 20
on timer pass-message
  say command "Tutorial 3 is complete. Stand by while tutorial 4 loads."
on timer pass
  win

# --- The fail: the colony hub destroyed (said at once, the failure 10 s later, over at 20 s) ---
on destroyed colony-hub
  fail 45 20
  say command "We have lost the colony hub."
  timer fail-message 10
on timer fail-message
  say command "You have not achieved your objectives."

# Not expressible yet:
# - The generator counts are exact (exactly four, exactly five standing).

# --- C7 as F ----------------------------------------
# C7 "Defense" (Federation, slot 0; map coverd3, colony at 88:415). Rewritten from the
# game's own graph (S/ghidra/scripts_out/C7.json). All lines come from Command, without
# voice clips. vehicle-yard-upgrade is the extended vehicle yard (type 146). Handlers are
# in the order the recording met them (play-vm-c7.md).
mission C7 F
site colony 88 415
objective 53
objective 54
objective 55

# --- Opening instructions ---
on start
  timer intro-1 2
  timer intro-2 10
on timer intro-1
  say command "To check your objectives, press Mission on this window to open Command."
on timer intro-2
  say command "Before you can build a security center, your colony hub must reach stage 2, and so must your construction yard."

# --- Vehicle yard and its upgrade: the way to photon artillery and phaser tanks ---
on built vehicle-yard
  say command "Photon artillery needs a stage 2 vehicle yard now, and that in turn needs the hub and construction yard at stage 2."
on built vehicle-yard-upgrade
  say command "Photon units can be built now. For phaser tanks you'll need a science block with a phaser coil lab added to it."

# --- The three objectives ---
on built security-center
  say command "The security center is working. A vehicle yard will give you mobile units."
  complete 53
on built-all photon-artillery*2
  say command "The photon artillery vehicles are ready."
  complete 55
  signal defence-force
on built-all phaser-tank*2
  say command "The phaser tanks are ready."
  complete 54
  signal defence-force
on signal defence-force*2
  timer defence-message 5
on timer defence-message
  say command "The colony now has the defence force it needs."

# --- Pass ---
on all-completed
  timer pass-message 10
  timer pass 20
on timer pass-message
  say command "Tutorial 4 is complete. Stand by while tutorial 5 loads."
on timer pass
  win

# --- The fail: the colony hub destroyed (said at once, the failure 10 s later, over at 20 s) ---
on destroyed colony-hub
  fail 56 20
  say command "We have lost the colony hub."
  timer fail-message 10
on timer fail-message
  say command "You have not achieved your objectives."

# Not expressible yet:
# - The counts are exact: the game fires when exactly one security center / vehicle yard
#   / yard upgrade, or exactly two tanks / artillery units, exist (a unit lost before the
#   second is built keeps the count from reaching 2 at that moment).

)",
R"(# --- C8 as F ----------------------------------------
# C8 "Combat" (Federation, slot 0; map coverd3, colony at 88:415; a small Klingon colony
# at 232:343). Rewritten from the game's own graph (S/ghidra/scripts_out/C8.json). All
# lines come from Command, without voice clips. Upgrades: colony-hub-advanced (type 160),
# construction-yard-advanced (162), vehicle-yard-upgrade (146).
mission C8 F
site colony 88 415
site klingon-colony 232 343
objective 69
objective 70

# --- Opening instructions ---
on start
  timer intro-1 2
  timer intro-2 10
  timer intro-3 20
on timer intro-1
  say command "To check your objectives, press Mission on this window to open Command."
on timer intro-2
  say command "Keep the colony's power up, and mine more materials when you run low."
on timer intro-3
  say command "Use your forces with thought: unarmed scouts find targets fast, tanks suit close fighting, and artillery is for long range only."

# --- A viable colony: advanced hub and yard, extended vehicle yard, security center ---
on built-all colony-hub-advanced construction-yard-advanced vehicle-yard-upgrade security-center
  say command "The colony is viable now. From here on its power, resources and defence are in your hands."
  complete 69

# --- The Klingon hub ---
on destroyed klingon-hub
  say command "The Klingon hub has been destroyed."
  complete 70

# --- Pass ---
on all-completed
  timer pass-message 10
  timer pass 20
on timer pass-message
  say command "Tutorial 5 is complete, and with it the colony infrastructure training. You are ready for your first training mission. Congratulations."
on timer pass
  win

# --- The fail: the colony hub destroyed (said at once, the failure 10 s later, over at 20 s) ---
on destroyed colony-hub
  fail 71 20
  say command "We have lost the colony hub."
  timer fail-message 10
on timer fail-message
  say command "You have not achieved your objectives."

# Not expressible yet:
# - The Klingons (slot 1) have three patrol orders round their colony (222:337, 222:345,
#   230:345), started by the game itself, not by a script.
# - The four buildings are counted as "exactly one standing" each, at the moment it's
#   finished.

)"
};

// The scripts, in pieces (compilers limit a string literal's length).
std::string joined() {
    std::string s;
    for (const char* piece : kScripts)
        s += piece;
    return s;
}


} // namespace

const std::vector<MissionScript>& missionScripts() {
    static const std::vector<MissionScript> scripts = [] {
        std::vector<MissionScript> s;
        if (Error err = parseMissionScripts(joined(), s))
            s.clear(); // the tests parse it, so this doesn't happen
        return s;
    }();
    return scripts;
}

} // namespace reluva::newworlds
