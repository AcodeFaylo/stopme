import QtQuick

// Design token object — instantiate as `PrefTokens { id: tok }` in each component.
// Qt::ColorScheme::Dark == 2; binding updates live when the user changes the setting.
//
// The palette is the stopme brand: a dark-first navy ground with a magenta
// primary and an electric-blue secondary. The six brand constants below are
// fixed; every semantic token is derived from them per colour scheme.
QtObject {
    readonly property bool isDark: Qt.styleHints.colorScheme === 2

    // ── Brand constants ───────────────────────────────────────────────────────
    // Fixed in both schemes — use these for logo lockups and the gradient,
    // and the semantic tokens below for anything that has to stay legible.
    readonly property color brandMagenta: "#FF3D8B"   // primary · accent / CTA
    readonly property color brandBlue:    "#5566FF"   // secondary
    readonly property color brandNavy:    "#1A204F"   // structure · outlines
    readonly property color brandInk:     "#0A0E1C"   // background
    readonly property color brandPanel:   "#131A33"   // panels · cards
    readonly property color brandMist:    "#EEF1F8"   // light surface

    // Signature gradient: 135°, brandMagenta → brandBlue.
    readonly property color gradientFrom: brandMagenta
    readonly property color gradientTo:   brandBlue

    // ── Surfaces ──────────────────────────────────────────────────────────────
    readonly property color bg:         isDark ? brandInk   : brandMist
    readonly property color panel:      isDark ? brandPanel : "#FFFFFF"
    readonly property color panel2:     isDark ? brandNavy  : "#F6F8FD"
    readonly property color edge:       isDark ? Qt.rgba(238/255, 241/255, 248/255, 0.14)
                                               : Qt.rgba( 26/255,  32/255,  79/255, 0.14)
    readonly property color edge2:      isDark ? Qt.rgba(238/255, 241/255, 248/255, 0.08)
                                               : Qt.rgba( 26/255,  32/255,  79/255, 0.08)

    // ── Text ──────────────────────────────────────────────────────────────────
    readonly property color ink:        isDark ? brandMist : "#10152E"
    readonly property color ink2:       isDark ? "#C3CAE0" : "#3A4266"
    readonly property color mute:       isDark ? "#808CAD" : "#6E7794"

    // ── Accents ───────────────────────────────────────────────────────────────
    // Magenta primary. Lifted on the dark ground and darkened on the light one
    // so it stays readable as text and as an icon tint, not just as a fill.
    readonly property color accent:      isDark ? "#FF5C9D" : "#E01F72"
    readonly property color accentStrong:isDark ? "#FF8CBB" : "#A8134F"
    readonly property color accentSoft:  isDark ? "#3D2043" : "#FFE1EE"

    // Electric-blue secondary.
    readonly property color accent2:     isDark ? "#7B88FF" : "#4050E8"
    readonly property color accent2Soft: isDark ? "#1F2858" : "#E2E6FF"

    // ── State ─────────────────────────────────────────────────────────────────
    readonly property color track:      isDark ? "#202844" : "#DDE3F1"
    readonly property color danger:     isDark ? "#FF5C6E" : "#CE1F3A"
    readonly property color dangerSoft: isDark ? "#3D1A24" : "#FFE3E7"
    readonly property color warn:       isDark ? "#FFB020" : "#A96A00"
    readonly property color rest:       isDark ? "#A855F7" : "#7C3AED"
    readonly property color actionBg:   isDark ? "#1B2340" : "#E9EDF8"
    readonly property color actionEdge: isDark ? Qt.rgba(238/255, 241/255, 248/255, 0.30)
                                               : Qt.rgba( 26/255,  32/255,  79/255, 0.26)

    // ── Typography ────────────────────────────────────────────────────────────
    readonly property int labelPx:    14    // primary row label
    readonly property int hintPx:     12    // secondary / hint text
    readonly property int bodyPx:     13    // body / nav items / lede
    readonly property int captionPx:  11    // section headers, small badges
    readonly property int stepperPx:  19    // large time value in stepper
    readonly property int btnPx:      16    // +/− button glyphs
    readonly property int tickPx:     10    // slider tick labels

    // Lists, not single names: Qt walks them until one resolves, so the brand
    // face is used where it is installed and a sane system sans elsewhere.
    readonly property var displayFamilies: ["Space Grotesk", "Inter", "Segoe UI Variable",
                                            "Avenir Next", "DejaVu Sans"]
    readonly property var monoFamilies:    ["JetBrains Mono", "SF Mono", "Menlo",
                                            "Consolas", "DejaVu Sans Mono"]

    // ── Layout ────────────────────────────────────────────────────────────────
    readonly property int  labelHintGap: 3     // spacing between label and hint
    readonly property real hintLineH:    1.45  // hint / body text line height
    readonly property int  rowPad:       28    // vertical padding in toggle/choice rows
    readonly property int  rowPadLg:     36    // vertical padding in stepper/time rows
    readonly property int  actionRadius: 8     // actions are visibly distinct from status pills
}
