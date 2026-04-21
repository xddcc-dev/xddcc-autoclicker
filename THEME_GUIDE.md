# XDDCC AutoClicker — Theme Guide

Themes are JSON files that control every visual aspect of the app. You can create, import, and export themes from the **Themes** tab inside the app.

---

## File format

Themes use `.xtheme` or `.json` extension. Example skeleton:

```json
{
    "name": "My Theme",
    "author": "your name",
    "description": "A short description",
    "fontFamily": "Segoe UI",
    "fontSize": 13,
    "borderRadius": 6,
    "colors": {
        "background": "#1a1a2e",
        "surface":    "#16213e",
        "border":     "#2d2d4e",
        "primary":    "#6c63ff",
        "accent":     "#f5a623",
        "text":       "#e0e0f0",
        "muted":      "#7a7a9a",
        "success":    "#10b981",
        "danger":     "#ef4444"
    },
    "gradients": {
        "header": ["#1a1a2e", "#16213e"],
        "startBtn": ["#6c63ff", "#9d97ff"],
        "startBtnActive": ["#ef4444", "#ff7777"]
    },
    "starburst": false,
    "particles": {
        "enabled": false
    }
}
```

---

## Color fields (`colors` object)

| Key | Used for |
|-----|----------|
| `background` | Main window background |
| `surface` | Cards, panels, inputs |
| `border` | Borders, dividers, spinbox buttons |
| `primary` | Tab indicator, buttons, active elements |
| `accent` | Secondary highlight color |
| `text` | Main text color |
| `muted` | Dimmed labels, status bar text |
| `success` | "RUNNING" indicator |
| `danger` | Close button hover, stop state |

All values are CSS hex colors (`#rrggbb` or `#aarrggbb`).

---

## Gradient fields (`gradients` object)

Each gradient is a two-element array `["#color1", "#color2"]` for a top-to-bottom linear gradient.

| Key | Used for |
|-----|----------|
| `header` | Title bar background |
| `startBtn` | START button in idle state |
| `startBtnActive` | START/STOP button while clicking is active |

---

## Typography

```json
"fontFamily": "Segoe UI",
"fontSize": 13,
"borderRadius": 6
```

- `fontFamily` — any font installed on the system.
- `fontSize` — base font size in px (labels and inputs scale with this).
- `borderRadius` — corner radius in px for cards, buttons, inputs.

---

## Starburst effect

```json
"starburst": true
```

When `true`, the background is replaced with a radial wedge pattern that fans out from the center, alternating between the `primary` and `accent` colors. This is the XDDCC theme look. The central widget renders on top of it with a transparent background.

---

## Particle system (`particles` object)

Particles are small floating dots that drift across the window — perfect for subtle ambience in dark themes.

```json
"particles": {
    "enabled":   true,
    "color":     "#ffffff",
    "count":     35,
    "speed":     0.45,
    "size":      2.2,
    "lifetime":  5.0,
    "spawnRate": 1
}
```

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `enabled` | bool | `false` | Turn particles on/off |
| `color` | string | `"#ffffff"` | Particle color (hex) |
| `count` | int | `35` | Max simultaneous particles on screen |
| `speed` | float | `0.45` | Movement speed (pixels per frame at 60 fps) |
| `size` | float | `2.2` | Particle radius in pixels |
| `lifetime` | float | `5.0` | Seconds each particle lives before fading out |
| `spawnRate` | int | `1` | New particles spawned per frame tick |

Particles fade in over the first 20% of their lifetime and fade out over the last 20%, so they always appear and disappear smoothly.

---

## Creating a theme in the app

1. Go to the **Themes** tab.
2. Click **Create Custom**.
3. Edit the JSON on the left — a live preview updates on the right as you type.
4. Click **Save Theme** when done. The theme is added to the grid and applied immediately.

---

## Importing and exporting

- **Import** — click "Import .xtheme" and select a `.xtheme` or `.json` file.
- **Export** — click "Export current" to save the active theme as a `.xtheme` file you can share.

---

## Tips

- Keep `surface` slightly lighter or darker than `background` so cards stand out.
- If `starburst` is `true`, the background color is hidden — but cards still use `surface`.
- Particles look best at low `speed` (0.3–0.6) with a `count` of 20–50.
- You can combine `starburst: true` **and** particles for a layered effect.
- Font families that work well on Windows: `Segoe UI`, `Inter`, `Consolas`, `JetBrains Mono`.











why did i waste so much time on this