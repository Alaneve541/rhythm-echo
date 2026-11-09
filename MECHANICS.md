# Mechanics sheet — osu!mania 4K rebuild

The game was rebuilt against a reference osu! stable 1.3.3.8 installation whose profile is
configured for 4-key mania:

- `osu!.cfg`: `LastPlayMode = OsuMania`, `ManiaSpeed = 24`,
  `ManiaLayouts4K = A S OemSemicolon OemQuotes`, `ManiaLayoutSelected4K = 0`,
  skin `[ DJMAX RESPECT ]`.
- The reference library contains 1045 osu!mania 4K charts. Six of them were used for
  play-testing while building this demo; none of them are distributed with the repository.
- The original executable is a managed .NET program whose metadata is obfuscated
  (`#=q…` types, `#=z…` methods), so the gameplay constants below come from the public osu!
  rules (wiki / stable reference) and from the player's files, not from decompiled code.
  Values that were not readable are marked **estimated**.

## Judgement (osu!mania)

A note is judged by the time between the key press and the note's hit time. `OD` is the chart's
OverallDifficulty.

| Result | Score value | Accuracy weight | Max hit error (ms) |
|---|---:|---:|---|
| PERFECT (300g) | 320 | 100% | 16 (fixed) |
| GREAT (300) | 300 | 100% | `trunc(64 − 3 × OD)` |
| GOOD (200) | 200 | 66.67% | `trunc(97 − 3 × OD)` |
| OK (100) | 100 | 33.33% | `trunc(127 − 3 × OD)` |
| MEH (50) | 50 | 16.67% | `trunc(151 − 3 × OD)`, early hits only |
| MISS | 0 | 0% | after the OK window |

At OD 6.5 the windows are 16 / 44 / 77 / 107 / 131 ms; at OD 7.0 they are 16 / 43 / 76 / 106 / 130 ms.
A late MEH is impossible in osu!mania, so a note becomes a miss once the OK window has passed.
A press outside every window does nothing (no penalty), as in the original.

Sources: <https://osu.ppy.sh/wiki/en/Gameplay/Judgement/osu%21mania> (maximum hit error table and
the note that late MEH hits are impossible).

## Hold notes

A hold note receives one judgement at its tail. The head must be pressed, the key must stay down
through the tail, and the release error is combined with the head error:

| Result | Requirement |
|---|---|
| PERFECT | head ≤ 16 × 1.2 ms and head+tail ≤ 16 × 2.4 ms |
| GREAT | head ≤ GREAT × 1.1 and head+tail ≤ GREAT × 2.2 |
| GOOD | head ≤ GOOD and head+tail ≤ GOOD × 2 |
| OK | head ≤ OK and head+tail ≤ OK × 2 |
| MEH | anything else that is not a miss |
| MISS | the key is not down from the tail's early MEH window to the tail |

Releasing during the hold body caps the result at MEH; pressing again before the tail keeps the
cap. Source: the same osu! wiki page (hold notes section).

## Score (ScoreV1, osu!mania)

Every chart is worth a maximum of **1,000,000** points, half base score and half bonus score.
For `N` note heads (a hold counts once):

```
unit        = 1,000,000 × 0.5 / N
BaseScore   = unit × (HitValue / 320)
BonusScore  = unit × (HitBonusValue × sqrt(Bonus) / 320)
Bonus       = clamp(Bonus + HitBonus − HitPunishment, 0, 100), starts at 100
Score       = Σ (BaseScore + BonusScore), rounded to the nearest integer
```

| Result | HitValue | HitBonusValue | HitBonus | HitPunishment |
|---|---:|---:|---:|---:|
| PERFECT | 320 | 32 | 2 | — |
| GREAT | 300 | 32 | 1 | — |
| GOOD | 200 | 16 | — | 8 |
| OK | 100 | 8 | — | 24 |
| MEH | 50 | 4 | — | 44 |
| MISS | 0 | 0 | — | Bonus becomes 0 |

No mods are implemented, so the mod multiplier is 1. Source:
<https://osu.ppy.sh/wiki/en/Gameplay/Score/ScoreV1/osu%21mania>.

## Accuracy, combo, grade

- Accuracy `= 100 × (300 × (PERFECT + GREAT) + 200 × GOOD + 100 × OK + 50 × MEH) / (300 × judged)`.
  A PERFECT counts as a 300 for accuracy. Source: osu! wiki Accuracy.
- Combo increases on every non-miss judgement and resets on a miss. A hold increments combo once,
  at its tail.
- Grades: SS at 100%, S over 95%, A over 90%, B over 80%, C over 70%, D otherwise; F on fail.
  Source: <https://osu.ppy.sh/wiki/en/Gameplay/Grade>.

## Health and fail

- Starts at 100.
- PERFECT +1.2, GREAT +1.0, GOOD +0.4, OK +0.1, MEH −0.5, MISS −5.0.
- Passive drain `0.10 + 0.08 × HP` health per second (0.62/s at HP 6.5).
- Reaching 0 fails the chart and shows F on the results screen.
- **Estimated**: the original's health constants are inside the obfuscated binary. These were
  tuned so a clean run stays near full and a missed section drains visibly.

## Playfield and scroll speed

- 4 columns, fixed 4:3-style playfield height equal to the window height; osu!'s column width is
  64 osu! pixels at a 480 px playfield, i.e. `columnWidth = height × 64/480` (96 px at 720p,
  288 px at 4K).
- Judgement line at 84% of the window height (osu! stable's default hit position is 402/480 ≈
  0.8375 — **estimated** from the stable layout).
- `ManiaSpeed = 24` from the player's config. The original's exact speed-to-pixel mapping is not
  readable, so this build maps it to a **720 ms** travel for the full window height
  (`pixels/ms = height / 720`); the visible top-to-line travel is ≈ 605 ms. **Estimated.**
- Notes use the player's skin: `mania-note1` on lanes 1 and 4, `mania-note2` on lanes 2 and 3,
  with the `H`/`L`/`T` images for hold heads, bodies and tails. `@2x` variants are preferred so
  the field stays sharp at 4K.
- `DimLevel = 80` in the player's config: the background is shown at 20% brightness during play.

## Controls (from `osu!.cfg`)

| Action | Binding |
|---|---|
| Lane 1 / 2 / 3 / 4 | `A` `S` `;` `'` (as `ManiaLayouts4K`) |
| Same lanes (alternate) | `D` `F` `J` `K` (**estimated**, common 4K alternate) |
| Pause / resume | `Escape` |
| Menus | Up / Down, Enter / Space, mouse click |
| Back | `Escape` |

The original's `keyPause = Escape`, `keySkip = Space`.

## Screens and HUD

- Main menu → song select → play → results. Pause overlay with resume / retry / quit.
- Song select: title, artist, difficulty name, 4K, BPM, OD, HP, note count, hold count, length.
- In play: health bar top-left with title/artist, score top-right, accuracy under it, combo above
  the stage, hit-error meter at the top centre, skin judgement burst, key overlay with `A S ; '`
  (and `D F J K`), song progress and time at the bottom, `AUTO` tag when autoplay is running.
- Results: grade, score, accuracy, max combo, 300g / 300 / 200 / 100 / 50 / miss counts,
  retry / song select / menu.
- The original's exact HUD pixel placement is **estimated**; the skin elements and information
  shown are the same.

## Bundled charts (real numbers from the player's library)

| Song | Difficulty | OD | HP | BPM | Notes | Holds | Length |
|---|---|---:|---:|---:|---:|---:|---:|
| BlackY — azalea | BASIC | 6.5 | 6.5 | 198.0 | 413 | 173 | 2:04 |
| Camellia — PSYCHO+HEROES | BASIC | 7.0 | 7.0 | 210.5 | 497 | 178 | 1:56 |
| Yooh — snow storm -euphoria- | Sava's NOVICE | 6.5 | 6.5 | 180.0 | 402 | 124 | 1:56 |
| Camellia — Bangin' Burst | NOVICE | 6.5 | 6.5 | 234.0 | 554 | 23 | 1:57 |
| Kraken — Do it!! Do it!! | 2 | 6.5 | 6.5 | 300.0 | 289 | 46 | 1:49 |
| lapix — Inner Spirit | OWJI's Beginner | 6.5 | 6.5 | 146.0 | 287 | 65 | 2:00 |

Sources: the `.osu` files from the author's own osu! library (see `CREDITS.md`), imported
into `Content/Rhythm/Songs/*/chart.osu` with `Tools/import_charts.py`.

## Not implemented in this version

- osu!standard, osu!taiko and osu!catch modes; 5K/6K/7K/8K mania.
- Mods (DT/HT/HR/EZ/HD/FL…), ScoreV2, multiplayer, online ranking, replay recording.
- The beatmap editor, storyboards, video backgrounds, skin.ini per-column widths and the
  original's menus/audio.
- The original game's own menu skin and fonts; the gameplay skin elements are used.

## Crash fixes in this build

The editor crash the player reported (access violation at 0x60 in UnrealEd/LevelEditor/Slate during
editor shutdown after Play-In-Editor) is addressed by removing the runtime window-mode change that
`BeginPlay` used to perform (it resized the PIE window while Slate was running), by not rewriting
the editor's command line from the game module, and by stopping audio/clearing texture caches in
`EndPlay`. The Python and Paper2D plugins are no longer enabled. The game window size now comes
from config / `-ResX` / `-ResY`, never from gameplay code.

## Build / automation

- `--screenshot <file.png>`: starts the first chart in autoplay near its first notes, draws 120
  frames, saves the window as a PNG and exits.
- `--self-test <file.json>`: starts the first chart, drives the four lanes with scripted presses for
  a few seconds, measures note travel, hits and the drawn HUD, writes the JSON and exits.
- For 4K output run with `-ResX=3840 -ResY=2160` (or set the window size in the config); the HUD
  and playfield scale from the window height and the `@2x` skin assets are used automatically.
