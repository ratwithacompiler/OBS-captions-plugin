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
#include "../caption_stream_helper.cpp"

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

CaptionSettingsWidget::CaptionSettingsWidget(const CaptionPluginSettings &latest_settings)
        : QWidget(),
          Ui_CaptionSettingsWidget(),
          latest_settings(latest_settings) {
    setupUi(this);
    this->updateUi();

    captionWhenComboBox->addItem("When Caption Source is streamed", "own_source");
    captionWhenComboBox->addItem("When Other Source is streamed", "other_mute_source");

    setup_combobox_languages(*languageComboBox);
    setup_combobox_profanity(*profanityFilterComboBox);
    setup_combobox_output_target(*outputTargetComboBox);

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
    CaptionPluginSettings new_settings = latest_settings;
    SourceCaptionerSettings &source_settings = new_settings.source_cap_settings;

    source_settings.caption_source_settings.caption_source_name = sourcesComboBox->currentText().toStdString();
    source_settings.caption_source_settings.mute_source_name = muteSourceComboBox->currentText().toStdString();

    string lang_str = languageComboBox->currentData().toString().toStdString();
    source_settings.stream_settings.stream_settings.language = lang_str;
    info_log("lang: %s", lang_str.c_str());

    const int profanity_filter = profanityFilterComboBox->currentData().toInt();
    source_settings.stream_settings.stream_settings.profanity_filter = profanity_filter;
    info_log("profanity_filter: %d", profanity_filter);

    string when_str = captionWhenComboBox->currentData().toString().toStdString();
//    info_log("accepting when: %s", when_str.c_str());
    source_settings.caption_source_settings.mute_when = string_to_mute_setting(when_str, CAPTION_SOURCE_MUTE_TYPE_FROM_OWN_SOURCE);
//    info_log("accepting when: %d", latest_settings.caption_source_settings.mute_when);

    source_settings.format_settings.caption_line_count = lineCountSpinBox->value();

    source_settings.format_settings.caption_insert_newlines = insertLinebreaksCheckBox->isChecked();
    new_settings.enabled = enabledCheckBox->isChecked();

    const int output_combobox_val = outputTargetComboBox->currentData().toInt();
    if (!set_streaming_recording_enabled(output_combobox_val,
                                         source_settings.streaming_output_enabled, source_settings.recording_output_enabled)) {
        error_log("invalid output target combobox value, wtf: %d", output_combobox_val);
    }

    source_settings.format_settings.caption_timeout_enabled = this->captionTimeoutEnabledCheckBox->isChecked();
    source_settings.format_settings.caption_timeout_seconds = this->captionTimeoutDoubleSpinBox->value();

    string banned_words_line = this->bannedWordsPlainTextEdit->toPlainText().toStdString();
    source_settings.format_settings.manual_banned_words = string_to_banned_words(banned_words_line);
//    info_log("acceptt %s, words: %lu", banned_words_line.c_str(), banned_words.size());

    info_log("accepting changes");
    new_settings.print();

    emit settings_accepted(new_settings);
}

static int combobox_set_data_str(QComboBox &combo_box, const char *data, int default_index) {
    int index = combo_box.findData(data);
    if (index == -1)
        index = default_index;

    combo_box.setCurrentIndex(index);
    return index;

}

static int combobox_set_data_int(QComboBox &combo_box, const int data, int default_index) {
    int index = combo_box.findData(data);
    if (index == -1)
        index = default_index;

    combo_box.setCurrentIndex(index);
    return index;
}

void CaptionSettingsWidget::updateUi() {
    SourceCaptionerSettings &source_settings = latest_settings.source_cap_settings;

    update_combobox_with_current_sources(*sourcesComboBox);
    update_combobox_with_current_sources(*muteSourceComboBox);
    sourcesComboBox->setCurrentText(QString(source_settings.caption_source_settings.caption_source_name.c_str()));
    muteSourceComboBox->setCurrentText(QString(source_settings.caption_source_settings.mute_source_name.c_str()));

    combobox_set_data_str(*languageComboBox, source_settings.stream_settings.stream_settings.language.c_str(), 0);
    combobox_set_data_int(*profanityFilterComboBox, source_settings.stream_settings.stream_settings.profanity_filter, 0);

    info_log("source_settings.caption_source_settings.mute_when %d", source_settings.caption_source_settings.mute_when);
    string mute_when_str = mute_setting_to_string(source_settings.caption_source_settings.mute_when, "own_source");

    int when_index = combobox_set_data_str(*captionWhenComboBox, mute_when_str.c_str(), 0);
    info_log("setting mute_when_str '%s', index %d", mute_when_str.c_str(), when_index);

    lineCountSpinBox->setValue(source_settings.format_settings.caption_line_count);
    insertLinebreaksCheckBox->setChecked(source_settings.format_settings.caption_insert_newlines);

    enabledCheckBox->setChecked(latest_settings.enabled);
    update_combobox_output_target(*outputTargetComboBox,
                                  source_settings.streaming_output_enabled, source_settings.recording_output_enabled);

    this->captionTimeoutEnabledCheckBox->setChecked(source_settings.format_settings.caption_timeout_enabled);
    this->captionTimeoutDoubleSpinBox->setValue(source_settings.format_settings.caption_timeout_seconds);

    string banned_words_line;
    words_to_string(source_settings.format_settings.manual_banned_words, banned_words_line);
    this->bannedWordsPlainTextEdit->setPlainText(QString(banned_words_line.c_str()));

    this->update_other_source_visibility(source_settings.caption_source_settings.mute_when);
}

void CaptionSettingsWidget::set_settings(const CaptionPluginSettings &new_settings) {
    latest_settings = new_settings;
    updateUi();
}

void CaptionSettingsWidget::update_other_source_visibility(CaptionSourceMuteType mute_state) {
    bool be_visible = mute_state == CAPTION_SOURCE_MUTE_TYPE_USE_OTHER_MUTE_SOURCE;
    this->muteSourceComboBox->setVisible(be_visible);
    this->muteSourceLabel->setVisible(be_visible);

}
