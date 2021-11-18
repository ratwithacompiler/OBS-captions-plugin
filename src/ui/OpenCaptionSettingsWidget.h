//
// Created by Rat on 11/18/21.
//

#ifndef OBS_GOOGLE_CAPTION_PLUGIN_OPENCAPTIONSETTINGSWIDGET_H
#define OBS_GOOGLE_CAPTION_PLUGIN_OPENCAPTIONSETTINGSWIDGET_H

#include <QPushButton>
#include "ui_OpenCaptionSettingsWidget.h"
#include "uiutils.h"

static void clearLayout(QLayout *layout, int start) {
    QLayoutItem *item;
    while ((item = layout->takeAt(start))) {
        if (item->widget()) {
            item->widget()->setParent(nullptr);
            delete item->widget();
        }
        delete item;
    }
}

class OpenCaptionSettingsWidget : public QWidget, Ui_OpenCaptionSettingsWidget {
Q_OBJECT
private:
//    TextOutputSettings settings;
public:
    OpenCaptionSettingsWidget(QWidget *parent, TextOutputSettings settings)
            : QWidget(parent), Ui_OpenCaptionSettingsWidget() {
        setupUi(this);
        QObject::connect(this->enabledCheckBox, &QCheckBox::stateChanged,
                         this, &OpenCaptionSettingsWidget::on_enabledCheckBox_stateChanged);

        QObject::connect(this->deleteToolButton, &QCheckBox::clicked,
                         this, &OpenCaptionSettingsWidget::on_deleteToolButton_clicked);

        setup_combobox_capitalization(*capitalizationComboBox);

        this->enabledCheckBox->setChecked(settings.enabled);
        on_enabledCheckBox_stateChanged(settings.enabled);

        this->punctuationCheckBox->setChecked(settings.insert_punctuation);

        auto text_sources = get_text_sources();
        text_sources.insert(text_sources.begin(), "");
        setup_combobox_texts(*outputComboBox, text_sources);

        this->lineLengthSpinBox->setValue(settings.line_length);
        this->lineCountSpinBox->setValue(settings.line_count);
        combobox_set_data_int(*capitalizationComboBox, settings.capitalization, 0);

        this->outputComboBox->setCurrentText(QString::fromStdString(settings.text_source_name));
    }

    void on_enabledCheckBox_stateChanged(int new_state) {
        const bool disabled = !new_state;
        this->textSourceWidget->setDisabled(disabled);
    }

    void on_deleteToolButton_clicked() {
        emit deleteClicked(this);
    }

    TextOutputSettings getSettings() {
        TextOutputSettings settings;
        settings.enabled = this->enabledCheckBox->isChecked();
        settings.text_source_name = this->outputComboBox->currentText().toStdString();
        settings.line_count = this->lineCountSpinBox->value();
        settings.line_length = this->lineLengthSpinBox->value();
        settings.insert_punctuation = this->punctuationCheckBox->isChecked();
        settings.capitalization = (CapitalizationType) this->capitalizationComboBox->currentData().toInt();

        return settings;
    }

signals:

    void deleteClicked(OpenCaptionSettingsWidget *self);
};


class OpenCaptionSettingsList : public QWidget {
Q_OBJECT
private:
    std::vector<OpenCaptionSettingsWidget> widgets;
public:
    OpenCaptionSettingsList(QWidget *parent) : QWidget(parent) {
        auto layout = new QVBoxLayout(this);
        layout->setAlignment(Qt::AlignTop);

        auto button = new QPushButton("Add Text Output");
        button->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        QObject::connect(button, &QPushButton::clicked, this, &OpenCaptionSettingsList::addDefault);
        layout->addWidget(button);

        layout->setSpacing(0);
        this->setLayout(layout);
    }

    void addDefault() {
        addSettingsWidget(default_TextOutputSettings());
    }

    void addSettingsWidget(const TextOutputSettings &settings) {
        auto widget = new OpenCaptionSettingsWidget(this, settings);
        QObject::connect(widget, &OpenCaptionSettingsWidget::deleteClicked,
                         this, &OpenCaptionSettingsList::deleteSettingsWidget);
        this->layout()->addWidget(widget);
    }

public:
    void setSettings(const vector<TextOutputSettings> &settings) {
        clearLayout(this->layout(), 1);

        for (const auto &i: settings) {
            addSettingsWidget(i);
        }
    }

    vector<TextOutputSettings> getSettings() {
        vector<TextOutputSettings> settings;
        for (int i = 1; i < this->layout()->count(); i++) {
            auto item = this->layout()->itemAt(i);
            if (!item)
                continue;

            auto widget = item->widget();
            if (!widget)
                continue;

            auto *settingsWidget = dynamic_cast<OpenCaptionSettingsWidget *>(widget);
            if (!settingsWidget)
                continue;

            settings.push_back(settingsWidget->getSettings());
        }
        return settings;
    }

public slots:

    void deleteSettingsWidget(OpenCaptionSettingsWidget *target) {
        if (!target)
            return;
        for (int i = 1; i < this->layout()->count(); i++) {
            auto item = this->layout()->itemAt(i);
            if (!item)
                continue;

            auto widget = item->widget();
            if (!widget || widget != target)
                continue;

            this->layout()->removeItem(item);
            widget->hide();
            widget->setParent(nullptr);
            delete widget;
            return;
        }
    };
};


#endif //OBS_GOOGLE_CAPTION_PLUGIN_OPENCAPTIONSETTINGSWIDGET_H
