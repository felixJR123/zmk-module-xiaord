# Xiaord Background Images

The built-in backgrounds are compiled from this module. The custom background
slot, `CONFIG_XIAORD_BG_4`, is generated during the keyboard build from an image
stored in the keyboard config repo.

## Default Keyboard Repo Layout

For the default ZMK GitHub Actions workflow, place one JPG or PNG here:

```text
config/xiaord-bg/01-background.jpg
```

Then enable the custom slot in the keyboard `.conf`:

```conf
CONFIG_XIAORD_BG_1=n
CONFIG_XIAORD_BG_2=n
CONFIG_XIAORD_BG_3=n
CONFIG_XIAORD_BG_4=y
```

If the folder has more than one image, the first filename in sorted order is
used. Prefix the one you want with `01-`.

If no image is found or conversion fails, the firmware falls back to `BG_1`.

## Privacy

Pictures live in the keyboard config repo, not this public module. Keep the
keyboard config repo private if the image is personal or contains family photos.
If the image is meant to be public, the keyboard config repo can be public.

## Custom Folder

To use a different folder inside the keyboard config repo:

```conf
CONFIG_XIAORD_BG_4=y
CONFIG_XIAORD_BG_4_SOURCE_DIR="my-background-folder"
```

Relative paths are resolved from the keyboard config repo.
