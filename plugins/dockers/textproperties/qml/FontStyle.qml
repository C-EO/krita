/*
 *  SPDX-FileCopyrightText: 2024 Wolthera van Hövell tot Westerflier <griffinvalley@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */
import QtQuick 2.15
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.12
import org.krita.flake.text 1.0
import org.krita.components 1.0 as Kis

CollapsibleGroupProperty {
    propertyTitle: i18nc("@label", "Font Style");
    propertyName: "font-sub-style";
    propertyType: TextPropertyConfigModel.Character;
    visibilityState: TextPropertyConfigModel.AlwaysVisible;
    toolTip: i18nc("@info:tooltip",
                   "Font style allows setting the sub style of the given font family, such as italics and bold");
    searchTerms: i18nc("comma separated search terms for the fontsize property, matching is case-insensitive",
                       "weight, width, style, italics, oblique, font-style, font-stretch, font-weight, bold, optical-size, variation");

    property alias fontWeight: fontWeightSpn.value;
    property alias fontWidth: fontStretchSpn.value;
    property alias fontSlantSlope: fontSlantSpn.value;
    property int fontSlant: CssFontStyleModel.StyleNormal;

    onFontSlantChanged: {
        fontSlantCmb.currentIndex = fontSlantCmb.indexOfValue(fontSlant);
        if (!blockSignals) {
            properties.fontStyle.style = fontSlant;
        }
    }
    property alias fontOptical: opticalSizeCbx.checked;
    property alias fontSynthesizeWeight: synthesizeWeightCbx.checked;
    property alias fontSynthesizeSlant: synthesizeSlantCbx.checked;

    onPropertiesUpdated: {
        blockSignals = true;
        fontWeight = properties.fontWeight;
        fontWidth = properties.fontWidth;
        fontOptical = properties.fontOpticalSizeLink;
        fontSlant = properties.fontStyle.style;
        fontSlantSlope = properties.fontStyle.value;
        fontSynthesizeSlant = properties.fontSynthesisStyle;
        fontSynthesizeWeight = properties.fontSynthesisWeight;
        styleCmb.currentIndex = fontStylesModel.rowForStyle(properties.fontWeight, properties.fontWidth, properties.fontStyle.style, properties.fontStyle.value);

        propertyState = [
            properties.fontWeightState,
            properties.fontStyleState,
            properties.fontWidthState,
            properties.fontOpticalSizeLinkState,
            properties.axisValueState,
            properties.fontSynthesisStyleState,
            properties.fontSynthesisWeightState
        ]
        setVisibleFromProperty();
        blockSignals = false;
    }
    onFontWeightChanged: {
        if (!blockSignals) {
            properties.fontWeight = fontWeight;
        }
    }

    onFontWidthChanged: {
        if (!blockSignals) {
            properties.fontWidth = fontWidth;
        }
    }

    onFontSlantSlopeChanged: {
        if (!blockSignals) {
            properties.fontStyle.value = fontSlantSlope;
        }
    }

    onFontOpticalChanged: {
        if (!blockSignals) {
            properties.fontOpticalSizeLink = fontOptical;
        }
    }

    onFontSynthesizeSlantChanged:  {
        if (!blockSignals) {
            properties.fontSynthesisStyle = fontSynthesizeSlant;
        }
    }

    onFontSynthesizeWeightChanged:  {
        if (!blockSignals) {
            properties.fontSynthesisWeight = fontSynthesizeWeight;
        }
    }

    onEnableProperty: {
        properties.fontWeightState = KoSvgTextPropertiesModel.PropertySet;
    }

    Component.onCompleted: {
        mainWindow.connectAutoEnabler(fontSlantSpnArea);
    }

    titleItem: RowLayout{
        width: parent.width;
        height: implicitHeight;
        spacing: columnSpacing;
        Label {
            id: propertyTitleLabel;
            text: propertyTitle;
            verticalAlignment: Text.AlignVCenter
            color: sysPalette.text;
            elide: Text.ElideRight;
            Layout.maximumWidth: contentWidth;
            Layout.preferredHeight: implicitHeight;
        }

        SqueezedComboBox {
        id: styleCmb;
        model: fontStylesModel;
        textRole: "display";
        Layout.fillWidth: true;
        Layout.preferredHeight: implicitHeight;
        onActivated: {
            if (!blockSignals) {
                // Because each change to propertiesModel causes signals to fire,
                // we need to first store the vars and then apply them.
                var weight = fontStylesModel.weightValue(currentIndex);
                var width = fontStylesModel.widthValue(currentIndex);
                var styleMode = fontStylesModel.styleModeValue(currentIndex);
                var styleVal = fontStylesModel.slantValue(currentIndex);
                var axesValues = fontStylesModel.axesValues(currentIndex);
                properties.fontWeight = weight;
                properties.fontWidth = width;
                properties.fontStyle.style = styleMode;
                if (styleMode === CssFontStyleModel.StyleOblique) {
                    properties.fontStyle.value = styleVal;
                }
                properties.axisValues = axesValues;
            }
        }
    }}

    contentItem: GridLayout {
        id: mainLayout;
        columns: 3
        anchors.left: parent.left
        anchors.right: parent.right
        columnSpacing: 5;

        RevertPropertyButton {
            revertState: properties.fontWeightState;
            onClicked: properties.fontWeightState = KoSvgTextPropertiesModel.PropertyUnset;
        }

        Kis.IntSliderSpinBox {
            id: fontWeightSpn
            prefix: i18nc("@label:slider", "Weight: ");
            from: 0;
            to: 1000;
            blockUpdateSignalOnDrag: true;
            Layout.fillWidth: true;
            Layout.columnSpan: 2;

        }
        RevertPropertyButton {
            revertState: properties.fontSynthesisWeightState;
            onClicked: properties.fontSynthesisWeightState = KoSvgTextPropertiesModel.PropertyUnset;
        }
        Item {
        width: 1;
        height: 1;}

        CheckBox {
            id: synthesizeWeightCbx
            text: i18nc("@option:check", "Synthesize Bold")
            Layout.fillWidth: true;
            font.italic: properties.fontSynthesisWeightState === KoSvgTextPropertiesModel.PropertyTriState;
        }


        RevertPropertyButton {
            revertState: properties.fontWidthState;
            onClicked: properties.fontWidthState = KoSvgTextPropertiesModel.PropertyUnset;
        }

        Kis.IntSliderSpinBox {
            id: fontStretchSpn
            prefix: i18nc("@label:slider", "Width: ")
            from: 0;
            to: 200;
            blockUpdateSignalOnDrag: true;
            Layout.fillWidth: true;
            Layout.columnSpan: 2;
        }

        RevertPropertyButton {
            revertState: properties.fontStyleState;
            onClicked: properties.fontStyleState = KoSvgTextPropertiesModel.PropertyUnset;
        }
        Label {
            text: i18nc("@label:listbox", "Slant:")
            Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
            horizontalAlignment: Text.AlignRight;
            Layout.preferredWidth: implicitWidth;
            elide: Text.ElideRight;
            font.italic: properties.fontStyleState === KoSvgTextPropertiesModel.PropertyTriState;
        }

        ComboBox {
            id: fontSlantCmb
            model: [
                {text: i18nc("@label:inlistbox", "Normal"), value: CssFontStyleModel.StyleNormal},
                {text: i18nc("@label:inlistbox", "Italic"), value: CssFontStyleModel.StyleItalic},
                {text: i18nc("@label:inlistbox", "Oblique"), value: CssFontStyleModel.StyleOblique}
            ]
            Layout.fillWidth: true
            textRole: "text";
            valueRole: "value";
            onActivated: fontSlant = currentValue;
            wheelEnabled: true;
        }

        Item {
        width: 1;
        height: 1;
        Layout.columnSpan: 2;
        }
        MouseArea {
            id: fontSlantSpnArea;
            function autoEnable() {
                fontSlant = CssFontStyleModel.StyleOblique;
            }
            Layout.fillWidth: true;
            Layout.preferredHeight: fontSlantSpn.implicitHeight;
            Kis.IntSliderSpinBox {
                id: fontSlantSpn;
                suffix: i18nc("@item:valuesuffix", "%")
                from: -90;
                to: 90;
                enabled: fontSlant == CssFontStyleModel.StyleOblique;
                PaletteControl {
                    id: slantSpnPal;
                    colorGroup: parent.enabled? SystemPalette.Active: SystemPalette.Disabled;
                }
                blockUpdateSignalOnDrag: true;
                anchors.fill: parent;
                palette: slantSpnPal.palette;
            }
        }

        RevertPropertyButton {
            revertState: properties.fontSynthesisStyleState;
            onClicked: properties.fontSynthesisStyleState = KoSvgTextPropertiesModel.PropertyUnset;
        }
        Item {
        width: 1;
        height: 1;}

        CheckBox {
            id: synthesizeSlantCbx
            text: i18nc("@option:check", "Synthesize Slant")
            Layout.fillWidth: true;
            font.italic: properties.fontSynthesisStyleState === KoSvgTextPropertiesModel.PropertyTriState;
        }

        RevertPropertyButton {
            revertState: properties.fontOpticalSizeLinkState;
            onClicked: properties.fontOpticalSizeLinkState = KoSvgTextPropertiesModel.PropertyUnset;
        }
        Item {
        width: 1;
        height: 1;}

        CheckBox {
            id: opticalSizeCbx
            text: i18nc("@option:check", "Optical Size")
            Layout.fillWidth: true;
            font.italic: properties.fontOpticalSizeLinkState === KoSvgTextPropertiesModel.PropertyTriState;
        }

        ColumnLayout {
            RevertPropertyButton {
                revertState: properties.axisValueState;
                onClicked: properties.axisValueState = KoSvgTextPropertiesModel.PropertyUnset;
            }
            Item {
                Layout.fillHeight: true;
            }
        }
        ListView {
            id: axesView;
            model: fontAxesModel;
            Layout.columnSpan: 2;
            Layout.fillWidth: true;
            Layout.preferredHeight: contentHeight;
            spacing: parent.columnSpacing;
            reuseItems: true;

            Label {
                text: i18n("No extra variable axes in this font");
                wrapMode: Text.WordWrap;
                anchors.fill: parent;
                anchors.horizontalCenter: parent.horizontalCenter;
                visible: parent.count === 0;
            }

            delegate: Kis.DoubleSliderSpinBox {
                id: axisSpn;
                required property string display;
                required property double axismin;
                required property double axismax;
                required property bool axishidden;
                required property var model;

                prefix: display + ": ";
                dFrom: axismin;
                dTo: axismax;
                dValue: model.edit;
                blockUpdateSignalOnDrag: true;
                onDValueChanged: model.edit = dValue;

                width: ListView.view.width;
            }
        }

    }
}
