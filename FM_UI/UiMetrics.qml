import QtQuick

QtObject {
    id: root

    property real viewportWidth: 1600
    property real viewportHeight: 900

    readonly property real baseWidth: 1600
    readonly property real baseHeight: 900
    readonly property real scale: clamp(Math.min(viewportWidth / baseWidth, viewportHeight / baseHeight), 0.78, 1.08)
    readonly property real fontScale: clamp(scale, 0.86, 1.08)
    readonly property bool compact: viewportWidth < 1400 || viewportHeight < 800
    readonly property bool narrow: viewportWidth < 1180
    readonly property bool wide: viewportWidth >= 1600
    readonly property bool dense: compact || viewportHeight < 820

    readonly property int spacingXs: px(4)
    readonly property int spacingSm: px(8)
    readonly property int spacingMd: px(dense ? 10 : 12)
    readonly property int spacingLg: px(dense ? 14 : 18)
    readonly property int spacingXl: px(dense ? 20 : 28)

    readonly property int radiusSm: px(6)
    readonly property int radiusMd: px(8)
    readonly property int radiusLg: px(12)
    readonly property int pageMargin: px(dense ? 18 : 28)
    readonly property int cardPadding: px(dense ? 14 : 20)
    readonly property int panelGap: px(dense ? 12 : 18)
    readonly property real compactTokenScale: clamp(scale, 0.78, 1.0)

    function clamp(value, minValue, maxValue) {
        if (maxValue < minValue)
            return minValue
        return Math.max(minValue, Math.min(maxValue, value))
    }

    function px(value) {
        return Math.round(value * scale)
    }

    function font(value) {
        return Math.round(value * fontScale)
    }
}
