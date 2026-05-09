.pragma library

// TODO: Team primary/secondary colors and crest references are temporary UI-side mappings.
// Move them to the SQL/data layer later as Team fields:
// - primaryColor
// - secondaryColor
// - crestAssetKey / badgePath

function normalizedTeamName(value) {
    return String(value || "")
        .normalize("NFD")
        .replace(/[\u0300-\u036f]/g, "")
        .toLowerCase()
}

function palette(teamName) {
    const name = normalizedTeamName(teamName)
    if (name.indexOf("alanyaspor") >= 0) {
        return { primary: "#f97316", secondary: "#22c55e", text: "#071016" }
    }
    if (name.indexOf("basaksehir") >= 0) {
        return { primary: "#1d4ed8", secondary: "#f97316", text: "#f8fafc" }
    }
    if (name.indexOf("trabzonspor") >= 0) {
        return { primary: "#7f1d1d", secondary: "#2563eb", text: "#f8fafc" }
    }
    if (name.indexOf("rizespor") >= 0) {
        return { primary: "#16a34a", secondary: "#f8fafc", text: "#071016" }
    }
    if (name.indexOf("samsunspor") >= 0) {
        return { primary: "#dc2626", secondary: "#f8fafc", text: "#071016" }
    }
    if (name.indexOf("galatasaray") >= 0) {
        return { primary: "#b91c1c", secondary: "#facc15", text: "#f8fafc" }
    }
    if (name.indexOf("kayserispor") >= 0) {
        return { primary: "#facc15", secondary: "#dc2626", text: "#071016" }
    }
    if (name.indexOf("goztepe") >= 0) {
        return { primary: "#dc2626", secondary: "#facc15", text: "#f8fafc" }
    }
    if (name.indexOf("gaziantep") >= 0) {
        return { primary: "#dc2626", secondary: "#111827", text: "#f8fafc" }
    }
    if (name.indexOf("fenerbahce") >= 0) {
        return { primary: "#facc15", secondary: "#172554", text: "#071016" }
    }
    if (name.indexOf("besiktas") >= 0) {
        return { primary: "#111827", secondary: "#f8fafc", text: "#f8fafc" }
    }
    if (name.indexOf("konyaspor") >= 0) {
        return { primary: "#16a34a", secondary: "#f8fafc", text: "#071016" }
    }
    if (name.indexOf("antalyaspor") >= 0) {
        return { primary: "#dc2626", secondary: "#f8fafc", text: "#071016" }
    }
    if (name.indexOf("eyupspor") >= 0) {
        return { primary: "#7e22ce", secondary: "#facc15", text: "#f8fafc" }
    }
    return { primary: "#0f1a24", secondary: "#22c55e", text: "#f8fafc" }
}

function primaryColor(teamName) {
    return palette(teamName).primary
}

function secondaryColor(teamName) {
    return palette(teamName).secondary
}

function textColor(teamName) {
    return palette(teamName).text
}

function initial(teamName) {
    const text = String(teamName || "").trim()
    return text.length > 0 ? text.charAt(0).toUpperCase() : "-"
}
