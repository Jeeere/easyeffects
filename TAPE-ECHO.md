# Custom Tape Echo

This branch adds a Tape Echo effect backed by the Calf Vintage Delay LV2 plugin (`http://calf.sourceforge.net/plugins/VintageDelay`), not a new tape simulation. On Arch, the `calf` package supplies the plugin; it is optional for Easy Effects overall but required for this effect. Check availability with `lv2ls | grep -Fx 'http://calf.sourceforge.net/plugins/VintageDelay'` (provided by `lilv-tools`).

## Controls

The settings schema and UI specify these ranges and defaults:

| Control | Range | Default |
| --- | --- | --- |
| Delay Time | 10–2000 ms | 250 ms |
| Feedback | 0–95% | 35% |
| Medium | Plain, Tape, Old Tape | Tape |
| Dry | 0–100% | 100% |
| Wet | 0–100% | 35% |
| Input / output gain | −36 to +36 dB | 0 dB each |
| Bypass | off / on | off |

Dry and Wet are separate levels, not a single crossfade. The backend selects millisecond timing with equal left/right delay settings. Reset restores defaults. These are source-level descriptions, not a claim that a live audio session has been verified.

## Build and install on Arch

The local recipe builds a **committed Git revision**, not uncommitted edits. Install `base-devel` and the build, runtime and check dependencies listed in `packaging/PKGBUILD` through your usual authorized process first. Default package checks require `calf`, `lilv-tools` and `python`; `calf` must also remain installed to use Tape Echo. From this checkout, as an ordinary user:

```sh
cd /home/jere/Development/easyeffects-custom
commit=$(git rev-parse HEAD)
git show -s --format='%H %s' "$commit"  # record this hash
cd packaging
EASYEFFECTS_COMMIT="$commit" makepkg --cleanbuild
```

Build output is an `easyeffects-custom-*.pkg.tar.*` file; building does not install it. To install a **specific** resulting package after reviewing it, use `sudo pacman -U /full/path/to/easyeffects-custom-VERSION-ARCH.pkg.tar.zst` (substitute the actual filename and compression). It conflicts with official `easyeffects` and `easyeffects-git`, provides `easyeffects`, and shares application settings; it is not side-by-side. Review pacman prompts, including future update conflicts. See `packaging/README.md` for source pinning, trust and reproducibility limits.

## Update and export patches

Commit or otherwise save all work first; `git rebase` requires a clean working tree. Fetch and rebase the feature branch, resolve any conflicts and test before rebuilding the new committed HEAD with the commands above:

```sh
cd /home/jere/Development/easyeffects-custom
git switch feature/tape-echo
git fetch upstream
git rebase upstream/master
# If conflicts occur: resolve them, git add the resolved files, then git rebase --continue
# To abandon the rebase instead: git rebase --abort
```

The original upstream base was `3d1ce79f45d490491b5eae869cda9b6868d4d00d` (v8.3.3); `upstream` points to `https://github.com/wwmm/easyeffects.git`. Export committed feature commits **outside** the repository/build tree after fetching the intended upstream base:

```sh
mkdir -p /home/jere/backups/easyeffects/tape-echo-patches
git format-patch --output-directory /home/jere/backups/easyeffects/tape-echo-patches upstream/master..HEAD
```

Use a new or empty export directory to avoid confusing old patches with the current series. Patches do not include uncommitted changes.

## Rollback and backup

To replace the custom package with the official repository package, review the transaction and run `sudo pacman -S easyeffects`; pacman may prompt to remove the conflicting custom package. This changes the package, not necessarily user settings. A pre-change backup exists at `/home/jere/backups/easyeffects/20261122-023157-tX9u5C`, containing `.config/easyeffects` and `.local/share/easyeffects`. To restore those saved files, first quit Easy Effects and back up the **current** directories separately; then, after reviewing the backup, run:

```sh
backup=/home/jere/backups/easyeffects/20261122-023157-tX9u5C
mkdir -p "$HOME/.config" "$HOME/.local/share"
cp -a "$backup/.config/easyeffects" "$HOME/.config/"
cp -a "$backup/.local/share/easyeffects" "$HOME/.local/share/"
```

This overlays saved files and does not delete newer files; review extra custom settings/presets separately. Restoring overwrites files with matching names.

## Tests and limits

```sh
python -m unittest discover -s tests -p 'test_tape_echo_integration.py' -v
python -m unittest discover -s tests -p 'test_calf_vintage_delay.py' -v
```

The first checks settings bounds and source wiring; the second renders offline impulses through Calf using `lv2apply` and skips if the LV2 tools or plugin are unavailable. On this checkout, the two integration tests and five offline tests passed. Neither establishes that the packaged application builds, installs, displays correctly, routes PipeWire audio, or works interactively; those require separate verification. No installation is implied by these tests.
