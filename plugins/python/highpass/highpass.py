# SPDX-License-Identifier: CC0-1.0

try:
    from PyQt6.QtCore import Qt
    from PyQt6.QtWidgets import (
        QCheckBox,
        QComboBox,
        QDialog,
        QDialogButtonBox,
        QFormLayout,
        QMessageBox,
        QSpinBox,
        QVBoxLayout,
    )
except:
    from PyQt5.QtCore import Qt
    from PyQt5.QtWidgets import (
        QCheckBox,
        QComboBox,
        QDialog,
        QDialogButtonBox,
        QFormLayout,
        QMessageBox,
        QSpinBox,
        QVBoxLayout,
    )
from krita import Extension
from builtins import i18n, i18nc, Application


class HighpassExtension(Extension):

    def __init__(self, parent):
        super().__init__(parent)

    def setup(self):
        pass

    def createActions(self, window):
        action = window.createAction("high_pass_filter", i18n("High Pass"))
        action.triggered.connect(self.showDialog)

    def showDialog(self):
        doc = Application.activeDocument()
        if doc is None:
            QMessageBox.information(
                Application.activeWindow().qwindow(),
                i18n("High Pass Filter"),
                i18n("There is no active image."))
            return

        self.dialog = QDialog(Application.activeWindow().qwindow())

        self.intRadius = QSpinBox()
        self.intRadius.setValue(10)
        self.intRadius.setRange(2, 200)

        self.cmbMode = QComboBox()
        self.cmbMode.addItems(
            ["Color", "Preserve DC", "Greyscale",
             "Greyscale, Apply Chroma", "Redrobes"]
        )
        self.keepOriginal = QCheckBox(i18n("Keep original layer"))
        self.keepOriginal.setChecked(True)
        form = QFormLayout()
        form.addRow(i18nc("Filter radius in Highpass filter settings", "Filter radius:"), self.intRadius)
        form.addRow(i18n("Mode:"), self.cmbMode)
        form.addRow("", self.keepOriginal)

        self.buttonBox = QDialogButtonBox(self.dialog)
        self.buttonBox.setOrientation(Qt.Orientation.Horizontal)
        self.buttonBox.setStandardButtons(
            QDialogButtonBox.StandardButton.Ok | QDialogButtonBox.StandardButton.Cancel)
        self.buttonBox.accepted.connect(self.dialog.accept)
        self.buttonBox.accepted.connect(self.highpass)
        self.buttonBox.rejected.connect(self.dialog.reject)

        vbox = QVBoxLayout(self.dialog)
        vbox.addLayout(form)
        vbox.addWidget(self.buttonBox)

        self.dialog.show()
        self.dialog.activateWindow()
        self.dialog.exec()

    def highpass(self):
        # XXX: Start undo macro
        image = Application.activeDocument()
        original = image.activeNode()
        working_layer = original

        # We can only highpass on paint layers
        if self.keepOriginal.isChecked() or original.type() != "paintlayer":
            working_layer = image.createNode("working", "paintlayer")
            working_layer.setColorSpace(
                original.colorModel(),
                original.colorSpace(),
                original.profile())
            working_layer.writeBytes(
                original.readBytes(0, 0, image.width(), image.height()),
                0, 0, image.width(), image.height())

            # XXX: Unimplemented:
            original.parentNode().addChildNode(working_layer, original)

        image.setActiveNode(working_layer)
        colors_layer = None

        # if keeping colors
        if (self.cmbMode.currentIndex() == 1
                or self.cmbMode.currentIndex() == 3):
            # XXX: Unimplemented:
            colors_layer = working_layer.duplicate()
            colors_layer.setName("colors")
            # XXX: Unimplemented:
            original.parentNode().addChildNode(working_layer, colors_layer)

        # if greyscale, desature
        if (self.cmbMode.currentIndex() == 2
                or self.cmbMode.currentIndex() == 3):
            filter = Application.filter("desaturate")
            filter.apply(working_layer, 0, 0, image.width(), image.height())

        # Duplicate on top and blur
        blur_layer = working_layer.duplicate()
        blur_layer.setName("blur")
        # XXX: Unimplemented:
        original.parentNode().addChildNode(blur_layer, working_layer)

        # blur
        filter = Application.filter("gaussian blur")
        filter_configuration = filter.configuration()
        filter_configuration.setProperty("horizRadius", self.intRadius.value())
        filter_configuration.setProperty("vertRadius", self.intRadius.value())
        filter_configuration.setProperty("lockAspect", True)
        filter.setConfiguration(filter_configuration)
        filter.apply(blur_layer, 0, 0, image.width(), image.height())

        if self.cmbMode.currentIndex() <= 3:
            blur_layer.setBlendingMode("grain_extract")
            working_layer = image.mergeDown(blur_layer)

            # if preserve chroma, change set the mode to value and
            # merge down with the layer we kept earlier.
            if self.cmbMode.currentIndex() == 3:
                working_layer.setBlendingMode("value")
                working_layer = image.mergeDown(working_layer)

            # if preserve DC, change set the mode to overlay and merge
            # down with the average color of the layer we kept
            # earlier.
            if self.cmbMode.currentIndex() == 1:
                # get the average color of the entire image
                # clear the colors layer to the given color
                working_layer = image.mergeDown(working_layer)

        else:  # Mode == 4, RedRobes
            image.setActiveNode(blur_layer)
            # Get the average color of the input layer
            # copy the solid color layer
            # copy the blurred layer
        # XXX: End undo macro
