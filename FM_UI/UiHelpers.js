.pragma library

function mapValue(mapObject, key, fallbackValue) {
    if (!mapObject || mapObject[key] === undefined || mapObject[key] === null) {
        return fallbackValue
    }
    return mapObject[key]
}

function hasMapValue(mapObject, key) {
    return !!mapObject && mapObject[key] !== undefined && mapObject[key] !== null
}

function displayValue(mapObject, key) {
    return hasMapValue(mapObject, key) ? String(mapObject[key]) : "—"
}

function resultColor(resultLetter) {
    if (resultLetter === "W") {
        return "#067647"
    }
    if (resultLetter === "D") {
        return "#b54708"
    }
    if (resultLetter === "L") {
        return "#b42318"
    }
    return "#667085"
}

function resultBackground(resultLetter) {
    if (resultLetter === "W") {
        return "#ecfdf3"
    }
    if (resultLetter === "D") {
        return "#fffaeb"
    }
    if (resultLetter === "L") {
        return "#fef3f2"
    }
    return "#f2f4f7"
}
