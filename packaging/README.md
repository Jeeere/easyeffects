# Local Arch package

This recipe builds **committed** source from the enclosing Git checkout, defaulting to its current `HEAD`, not a remote upstream branch or the working tree. Commit the feature changes first; uncommitted changes, untracked files, personal backups and existing build directories are not included. A full clone with upstream version tags and history is required. `EASYEFFECTS_COMMIT` can name any commit present locally; record its full hash for a repeatable source selection. The Git source checksum is `SKIP`: the pinned commit identifies the tree but does not authenticate it. Trust and verify the checkout. This does not promise bit-identical binaries across toolchains, dependency versions or build environments.

Install the PKGBUILD's build, runtime and check dependencies and Arch `base-devel` using your normal administrator-approved process first. `calf` is optional for Easy Effects overall but required at runtime for Tape Echo; the default package check also requires `calf` and `lilv-tools`. Then as an ordinary user, from the checkout:

```sh
cd packaging
commit=$(git rev-parse HEAD)  # or select a specific committed feature revision
git show -s --format='%H %s' "$commit"
EASYEFFECTS_COMMIT="$commit" makepkg --cleanbuild
```

To rebuild that source later, use the recorded full commit instead of `HEAD`. Keep the relevant tags unchanged and use the same recipe revision, compiler, dependencies and makepkg configuration for comparable builds. Build in a clean Arch environment for stronger reproducibility. Source caches are named by commit and the extracted Git HEAD is checked before compilation. Do not use `--noextract` or `--repackage` for a fresh build: these reuse existing work. The default `check()` verifies that Calf Vintage Delay is available; `--nocheck` skips that check and its check dependencies. The result is an `easyeffects-custom-*.pkg.tar.zst` package (or the compression format configured in makepkg). Building does not install it.

`easyeffects-custom` version-provides `easyeffects` and conflicts with the official `easyeffects` and `easyeffects-git` packages. Installing it later with `pacman -U` will ask to remove the official package; do not use file overwrite flags. It is an alternative, not a side-by-side app, and shares the same application ID, settings and user data. Installing, reverting, and dependency changes require administrator authorization; they are not performed by this recipe. Reinstall the official `easyeffects` package to revert. A future system upgrade may offer the official package again or report a conflict, so review pacman prompts and update policy rather than assuming this custom build will stay installed.

The dependency baseline follows Arch's official 8.3.2-1 recipe, with `kcolorscheme` and `libmysofa` from this checkout's upstream recipe. Feature changes may introduce further dependencies. Upstream version tags and commit distance determine the package version; a rewritten history or changed tags can change ordering. Bump `pkgrel` when changing only this packaging recipe. The official install script only warns about migrations when upgrading from pre-8.0; a distinct package replacement runs a fresh install, so it is not included. Users migrating from versions older than 8.0 should consult the upstream 8.0 migration notes first.
