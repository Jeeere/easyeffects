pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import ee.ui
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.formcard as FormCard

Kirigami.ScrollablePage {
    id: page
    required property string name
    required property DbTapeEcho pluginDB
    required property EffectsBase pipelineInstance
    property BackendTapeEcho pluginBackend
    Component.onCompleted: pluginBackend = pipelineInstance.getPluginInstance(name)
    function updateMeters() {
        if (!pluginBackend) return;
        levels.setInputLevelLeft(pluginBackend.getInputLevelLeft());
        levels.setInputLevelRight(pluginBackend.getInputLevelRight());
        levels.setOutputLevelLeft(pluginBackend.getOutputLevelLeft());
        levels.setOutputLevelRight(pluginBackend.getOutputLevelRight());
    }
    header: EeInputOutputGain { id: levels; pluginDB: page.pluginDB }
    ColumnLayout {
        Kirigami.CardsLayout {
            Layout.fillWidth: true
            EeCard {
                title: i18n("Tape Echo")
                contentItem: ColumnLayout {
                    FormCard.FormComboBoxDelegate {
                        text: i18n("Medium")
                        displayMode: FormCard.FormComboBoxDelegate.ComboBox
                        currentIndex: page.pluginDB.medium
                        model: [i18n("Plain"), i18n("Tape"), i18n("Old Tape")]
                        onActivated: idx => { page.pluginDB.medium = idx; }
                    }
                    EeSpinBox {
                        label: i18n("Delay Time")
                        from: page.pluginDB.getMinValue("delayTime") * 1
                        to: page.pluginDB.getMaxValue("delayTime") * 1
                        value: page.pluginDB.delayTime * 1
                        decimals: 0
                        stepSize: 1
                        unit: Units.ms
                        onValueModified: v => { page.pluginDB.delayTime = v / 1; }
                    }
                    EeSpinBox {
                        label: i18n("Feedback")
                        from: page.pluginDB.getMinValue("feedback") * 100
                        to: page.pluginDB.getMaxValue("feedback") * 100
                        value: page.pluginDB.feedback * 100
                        decimals: 0
                        stepSize: 1
                        unit: Units.percent
                        onValueModified: v => { page.pluginDB.feedback = v / 100; }
                    }
                    EeSpinBox {
                        label: i18n("Dry")
                        from: page.pluginDB.getMinValue("dry") * 100
                        to: page.pluginDB.getMaxValue("dry") * 100
                        value: page.pluginDB.dry * 100
                        decimals: 0
                        stepSize: 1
                        unit: Units.percent
                        onValueModified: v => { page.pluginDB.dry = v / 100; }
                    }
                    EeSpinBox {
                        label: i18n("Wet")
                        from: page.pluginDB.getMinValue("wet") * 100
                        to: page.pluginDB.getMaxValue("wet") * 100
                        value: page.pluginDB.wet * 100
                        decimals: 0
                        stepSize: 1
                        unit: Units.percent
                        onValueModified: v => { page.pluginDB.wet = v / 100; }
                    }
                }
            }
        }
    }
    footer: Kirigami.ActionToolBar {
        actions: [Kirigami.Action {
            text: i18n("Reset")
            icon.name: "edit-reset-symbolic"
            onTriggered: page.pluginBackend.reset()
        }]
    }
}
