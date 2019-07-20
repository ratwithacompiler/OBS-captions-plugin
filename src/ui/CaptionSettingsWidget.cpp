/******************************************************************************
Copyright (C) 2019 by <rat.with.a.compiler@gmail.com>

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/

#include <obs.hpp>
#include "CaptionSettingsWidget.h"
#include "../log.c"
#include <utils.h>
#include "../storage_utils.h"

static void update_combobox_with_current_sources(QComboBox &comboBox) {
    while (comboBox.count())
        comboBox.removeItem(0);

    comboBox.addItem("", "");

    auto cb = [](void *param, obs_source_t *source) {
        auto comboBox = reinterpret_cast<QComboBox *>(param);

//        const char *id = obs_source_get_id(source);
        const char *name = obs_source_get_name(source);

        if (!name)//|| !id
            return true;

        QString q_name = name;
//        QString q_id = id;

        uint32_t caps = obs_source_get_output_flags(source);
        if (caps & OBS_SOURCE_AUDIO)
            comboBox->addItem(name);

//        info_log("source: %s id: %s", name, id);
        return true;
    };
    obs_enum_sources(cb, &comboBox);
}

CaptionSettingsWidget::CaptionSettingsWidget(CaptionerSettings latest_settings)
        : QWidget(),
          Ui_CaptionSettingsWidget(),
          latest_settings(latest_settings) {
    setupUi(this);
    this->updateUi();

    captionWhenComboBox->addItem("When Caption Source is streamed", "own_source");
    captionWhenComboBox->addItem("When Other Source is streamed", "other_mute_source");

    QObject::connect(this->cancelPushButton, &QPushButton::clicked, this, &CaptionSettingsWidget::hide);
    QObject::connect(this->savePushButton, &QPushButton::clicked, this, &CaptionSettingsWidget::accept_current_settings);

    QObject::connect(captionWhenComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
                     this, &CaptionSettingsWidget::caption_when_index_change);
}

void CaptionSettingsWidget::caption_when_index_change(int new_index) {
    QVariant data = captionWhenComboBox->currentData();
    string val_str = data.toString().toStdString();
    info_log("current index changed: %d, %s", new_index, val_str.c_str());

    this->update_other_source_visibility(string_to_mute_setting(val_str, CAPTION_SOURCE_MUTE_TYPE_FROM_OWN_SOURCE));
}

void CaptionSettingsWidget::accept_current_settings() {
    CaptionerSettings new_settings = latest_settings;

    new_settings.caption_source_settings.caption_source_name = sourcesComboBox->currentText().toStdString();
    new_settings.caption_source_settings.mute_source_name = muteSourceComboBox->currentText().toStdString();
    string when_str = captionWhenComboBox->currentData().toString().toStdString();
    info_log("accepting when: %s", when_str.c_str());
    new_settings.caption_source_settings.mute_when = string_to_mute_setting(when_str, CAPTION_SOURCE_MUTE_TYPE_FROM_OWN_SOURCE);
    info_log("accepting when: %d", latest_settings.caption_source_settings.mute_when);

    new_settings.format_settings.caption_line_count = lineCountSpinBox->value();

    new_settings.format_settings.caption_insert_newlines = insertLinebreaksCheckBox->isChecked();
    new_settings.enabled = enabledCheckBox->isChecked();

    new_settings.format_settings.caption_timeout_enabled = this->captionTimeoutEnabledCheckBox->isChecked();
    new_settings.format_settings.caption_timeout_seconds = this->captionTimeoutDoubleSpinBox->value();

    string banned_words_line = this->bannedWordsPlainTextEdit->toPlainText().toStdString();
    new_settings.format_settings.manual_banned_words = string_to_banned_words(banned_words_line);
//    info_log("acceptt %s, words: %lu", banned_words_line.c_str(), banned_words.size());

    emit settings_accepted(new_settings);
}

void CaptionSettingsWidget::updateUi() {
    update_combobox_with_current_sources(*sourcesComboBox);
    update_combobox_with_current_sources(*muteSourceComboBox);
    sourcesComboBox->setCurrentText(QString(latest_settings.caption_source_settings.caption_source_name.c_str()));
    muteSourceComboBox->setCurrentText(QString(latest_settings.caption_source_settings.mute_source_name.c_str()));

    info_log("latest_settings.caption_source_settings.mute_when %d", latest_settings.caption_source_settings.mute_when);
    string mute_when_str = mute_setting_to_string(latest_settings.caption_source_settings.mute_when, "own_source");
    int when_index = captionWhenComboBox->findData(mute_when_str.c_str());
    info_log("setting mute_when_str '%s', index %d", mute_when_str.c_str(), when_index);
    if (when_index == -1)
        when_index = 0;
    captionWhenComboBox->setCurrentIndex(when_index);

    lineCountSpinBox->setValue(latest_settings.format_settings.caption_line_count);
    insertLinebreaksCheckBox->setChecked(latest_settings.format_settings.caption_insert_newlines);

    enabledCheckBox->setChecked(latest_settings.enabled);

    this->captionTimeoutEnabledCheckBox->setChecked(latest_settings.format_settings.caption_timeout_enabled);
    this->captionTimeoutDoubleSpinBox->setValue(latest_settings.format_settings.caption_timeout_seconds);

    string banned_words_line;
    words_to_string(latest_settings.format_settings.manual_banned_words, banned_words_line);
    this->bannedWordsPlainTextEdit->setPlainText(QString(banned_words_line.c_str()));

    this->update_other_source_visibility(latest_settings.caption_source_settings.mute_when);
}

void CaptionSettingsWidget::set_settings(CaptionerSettings new_settings) {
    latest_settings = new_settings;
    updateUi();
}

void CaptionSettingsWidget::update_other_source_visibility(CaptionSourceMuteType mute_state) {
    bool be_visible = mute_state == CAPTION_SOURCE_MUTE_TYPE_USE_OTHER_MUTE_SOURCE;
    this->muteSourceComboBox->setVisible(be_visible);
    this->muteSourceLabel->setVisible(be_visible);

}
