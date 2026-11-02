# Upstream contributions

English | [Español](UPSTREAM_PRS.es.md)

Six separate **draft** pull requests are open against the official project. Each explains scope, validation,
remaining checks and an illustrative screenshot of the integrated fork. They preserve the official features
present at their pinned base, rather than offering the entire custom fork as one replacement.

| Proposal | Shared-library dependency |
|---|---|
| [RawInput Multi Keyboard](https://github.com/tmcleroy/cps2recomp/pull/224) | recomp-common #51 |
| [Neutralize Opposing Directions](https://github.com/tmcleroy/cps2recomp/pull/225) | Independent / Independiente |
| [Physics Assignment Joysticks](https://github.com/tmcleroy/cps2recomp/pull/226) | recomp-common #51 |
| [Sliders](https://github.com/tmcleroy/cps2recomp/pull/227) | recomp-common #51 |
| [New Cheats](https://github.com/tmcleroy/cps2recomp/pull/228) | Independent / Independiente |
| [Launcher Hardening](https://github.com/tmcleroy/cps2recomp/pull/229) | recomp-common #51 |

The prerequisite is [recomp-common #51](https://github.com/tmcleroy/recomp-common/pull/51): optional host
controller extensions, signed-axis capture, player-scoped binding conflicts, numeric menu controls and an
initial-window policy. The four dependent CPS2 proposals adopt the same canonical snapshot/manifest.
They should be repinned after upstream accepts the shared API, not merged with a private library pin.

Windows builds and focused fixtures passed; the PR descriptions distinguish original-routine tests,
short interpreter starts, prior integrated-fork play testing and still-pending full ladders/hash/rollback,
cross-platform CI and hardware acceptance sessions. Draft does not mean accepted or merged.

Global Bindable Hotkeys and Audio Equalizer remain in this fork and its [gallery](SCREENSHOTS.md), but
are not additional upstream PRs in this batch. MAME-derived EQ code was not included in these proposals.
Launcher Hardening is intentionally the remaining window/layout opt-in; it does not duplicate official #208's
ROM scan/packaging/path work or disable the official updater.

The user-tested 018 runtime and installed build are unchanged by publishing these branches and documents.
