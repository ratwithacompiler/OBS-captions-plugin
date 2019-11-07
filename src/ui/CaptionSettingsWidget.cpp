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
#include <QLineEdit>
#include <QFileDialog>
#include "../storage_utils.h"
#include "../caption_stream_helper.cpp"

static void update_combobox_with_current_audio_sources(QComboBox &comboBox) {
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

static int update_combobox_with_current_scene_collections(QComboBox &comboBox) {
    while (comboBox.count())
        comboBox.removeItem(0);

    char **names = obs_frontend_get_scene_collections();
    char **name = names;

    int cnt = 0;
    while (name && *name) {
        comboBox.addItem(QString(*name));
        name++;
        cnt++;
    }
    bfree(names);

    return cnt;
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

CaptionSettingsWidget::CaptionSettingsWidget(const CaptionPluginSettings &latest_settings)
        : QWidget(),
          Ui_CaptionSettingsWidget(),
          current_settings(latest_settings) {
    setupUi(this);

    QString with_version = bottomTextBrowser->toPlainText().replace("${VERSION_STRING}", VERSION_STRING);
    bottomTextBrowser->setPlainText(with_version);

    this->updateUi();

#if ENABLE_CUSTOM_API_KEY
    this->apiKeyLabel->show();
    this->apiKeyWidget->show();
#else
    this->apiKeyLabel->hide();
    this->apiKeyWidget->hide();
#endif

    captionWhenComboBox->addItem("When Caption Source is streamed", "own_source");
    captionWhenComboBox->addItem("When Other Source is streamed", "other_mute_source");

    setup_combobox_languages(*languageComboBox);
    setup_combobox_profanity(*profanityFilterComboBox);
    setup_combobox_output_target(*outputTargetComboBox);
    setup_combobox_output_target(*transcriptSaveForComboBox);

    QObject::connect(this->cancelPushButton, &QPushButton::clicked, this, &CaptionSettingsWidget::hide);
    QObject::connect(this->savePushButton, &QPushButton::clicked, this, &CaptionSettingsWidget::accept_current_settings);

    QObject::connect(captionWhenComboBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
                     this, &CaptionSettingsWidget::caption_when_index_change);

    QObject::connect(sourcesComboBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
                     this, &CaptionSettingsWidget::sources_combo_index_change);

    QObject::connect(captionWhenComboBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
                     this, &CaptionSettingsWidget::sources_combo_index_change);

    QObject::connect(muteSourceComboBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
                     this, &CaptionSettingsWidget::sources_combo_index_change);

//    QObject::connect(sceneCollectionComboBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
//                     this, &CaptionSettingsWidget::scene_collection_combo_index_change);
}


void CaptionSettingsWidget::set_show_key(bool set_to_show) {
    if (set_to_show) {
        apiKeyLineEdit->setEchoMode(QLineEdit::EchoMode::Password);
        apiKeyShowPushButton->setText("Show");
    } else {
        apiKeyLineEdit->setEchoMode(QLineEdit::EchoMode::Normal);
        apiKeyShowPushButton->setText("Hide");

    }
}

void CaptionSettingsWidget::on_apiKeyShowPushButton_clicked() {
    if (apiKeyLineEdit->echoMode() == QLineEdit::EchoMode::Password) {
        set_show_key(false);
    } else {
        set_show_key(true);
    }
}

void CaptionSettingsWidget::on_previewPushButton_clicked() {
    emit preview_requested();
}

void CaptionSettingsWidget::sources_combo_index_change(int new_index) {
    apply_ui_scene_collection_settings();
}

void CaptionSettingsWidget::apply_ui_scene_collection_settings() {
    string current_scene_collection_name = scene_collection_name;
//    string current_scene_collection_name = this->sceneCollectionComboBox->currentText().toStdString();

    debug_log("apply_ui_scene_collection_settings %s", current_scene_collection_name.c_str());

    CaptionSourceSettings cap_source_settings;
    cap_source_settings.caption_source_name = sourcesComboBox->currentText().toStdString();
    cap_source_settings.mute_source_name = muteSourceComboBox->currentText().toStdString();

    string when_str = captionWhenComboBox->currentData().toString().toStdString();
//    debug_log("accepting when: %s", when_str.c_str());
    cap_source_settings.mute_when = string_to_mute_setting(when_str, CAPTION_SOURCE_MUTE_TYPE_FROM_OWN_SOURCE);
//    debug_log("accepting when: %d", current_settings.caption_source_settings.mute_when);

    current_settings.source_cap_settings.update_setting(current_scene_collection_name, cap_source_settings);
}

void CaptionSettingsWidget::scene_collection_combo_index_change(int new_index) {
//    string current_scene_collection_name = this->sceneCollectionComboBox->currentText().toStdString();
//    debug_log("scene_collection_combo_index_change %s", current_scene_collection_name.c_str());
//    update_source_combo_boxes(current_scene_collection_name);
}

void CaptionSettingsWidget::update_source_combo_boxes(const string &use_scene_collection_name) {
    SourceCaptionerSettings &source_settings = current_settings.source_cap_settings;

    CaptionSourceSettings use_settings = default_CaptionSourceSettings();
    {
        const CaptionSourceSettings *specific_settings = source_settings.get_caption_source_settings_ptr(use_scene_collection_name);
        if (specific_settings) {
            use_settings = *specific_settings;
            debug_log("update_source_combo_boxes using specific settings, '%s'", use_scene_collection_name.c_str());
        } else
            debug_log("update_source_combo_boxes using default settings, '%s' not found", use_scene_collection_name.c_str());
    }

    const QSignalBlocker blocker1(sourcesComboBox);
    update_combobox_with_current_audio_sources(*sourcesComboBox);

    const QSignalBlocker blocker2(muteSourceComboBox);
    update_combobox_with_current_audio_sources(*muteSourceComboBox);

    const QSignalBlocker blocker3(captionWhenComboBox);

    sourcesComboBox->setCurrentText(QString(use_settings.caption_source_name.c_str()));
    muteSourceComboBox->setCurrentText(QString(use_settings.mute_source_name.c_str()));

    debug_log("use_settings.mute_when %d", use_settings.mute_when);
    string mute_when_str = mute_setting_to_string(use_settings.mute_when, "own_source");
    int when_index = combobox_set_data_str(*captionWhenComboBox, mute_when_str.c_str(), 0);
    debug_log("setting mute_when_str '%s', index %d", mute_when_str.c_str(), when_index);

    this->update_other_source_visibility(use_settings.mute_when);
}

void CaptionSettingsWidget::caption_when_index_change(int new_index) {
    QVariant data = captionWhenComboBox->currentData();
    string val_str = data.toString().toStdString();
    debug_log("caption_when_index_change current index changed: %d, %s", new_index, val_str.c_str());

    this->update_other_source_visibility(string_to_mute_setting(val_str, CAPTION_SOURCE_MUTE_TYPE_FROM_OWN_SOURCE));
}

void CaptionSettingsWidget::accept_current_settings() {
    SourceCaptionerSettings &source_settings = current_settings.source_cap_settings;

    string lang_str = languageComboBox->currentData().toString().toStdString();
    source_settings.stream_settings.stream_settings.language = lang_str;
//    debug_log("lang: %s", lang_str.c_str());

    const int profanity_filter = profanityFilterComboBox->currentData().toInt();
    source_settings.stream_settings.stream_settings.profanity_filter = profanity_filter;
//    debug_log("profanity_filter: %d", profanity_filter);
    source_settings.stream_settings.stream_settings.api_key = apiKeyLineEdit->text().toStdString();

    source_settings.format_settings.caption_line_count = lineCountSpinBox->value();

    source_settings.format_settings.caption_insert_newlines = insertLinebreaksCheckBox->isChecked();
    current_settings.enabled = enabledCheckBox->isChecked();

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

    auto &transcript_settings = source_settings.transcript_settings;
    transcript_settings.enabled = saveTranscriptsCheckBox->isChecked();
    const int transcript_combobox_val = transcriptSaveForComboBox->currentData().toInt();
    if (!set_streaming_recording_enabled(transcript_combobox_val,
                                         transcript_settings.streaming_transcripts_enabled,
                                         transcript_settings.recording_transcripts_enabled)) {
        error_log("invalid transcript output target combobox value, wtf: %d", output_combobox_val);
    }
    transcript_settings.output_path = transcriptFolderPathLineEdit->text().toStdString();

    debug_log("accepting changes");
//    current_settings.print();
    emit settings_accepted(current_settings);
}


void CaptionSettingsWidget::updateUi() {
    SourceCaptionerSettings &source_settings = current_settings.source_cap_settings;

//    const QSignalBlocker blocker1(sceneCollectionComboBox);
//    int scene_collections_cnt = update_combobox_with_current_scene_collections(*sceneCollectionComboBox);
//    if (scene_collections_cnt <= 1) {
//        sceneCollectionSelectLabel->hide();
//        sceneCollectionComboBox->hide();
//    }
//    sceneCollectionComboBox->setCurrentText(QString::fromStdString(scene_collection_name));
//    update_source_combo_boxes(sceneCollectionComboBox->currentText().toStdString());

    sceneCollectionSelectLabel->hide();
    sceneCollectionComboBox->hide();
    sceneCollectionNameLabel_2->setText(QString::fromStdString(scene_collection_name));
    update_source_combo_boxes(scene_collection_name);

    combobox_set_data_str(*languageComboBox, source_settings.stream_settings.stream_settings.language.c_str(), 0);
    combobox_set_data_int(*profanityFilterComboBox, source_settings.stream_settings.stream_settings.profanity_filter, 0);

    apiKeyLineEdit->setText(QString::fromStdString(source_settings.stream_settings.stream_settings.api_key));

    lineCountSpinBox->setValue(source_settings.format_settings.caption_line_count);
    insertLinebreaksCheckBox->setChecked(source_settings.format_settings.caption_insert_newlines);

    enabledCheckBox->setChecked(current_settings.enabled);
    update_combobox_output_target(*outputTargetComboBox,
                                  source_settings.streaming_output_enabled, source_settings.recording_output_enabled);

    this->captionTimeoutEnabledCheckBox->setChecked(source_settings.format_settings.caption_timeout_enabled);
    this->captionTimeoutDoubleSpinBox->setValue(source_settings.format_settings.caption_timeout_seconds);

    string banned_words_line;
    words_to_string(source_settings.format_settings.manual_banned_words, banned_words_line);
    this->bannedWordsPlainTextEdit->setPlainText(QString(banned_words_line.c_str()));

    saveTranscriptsCheckBox->setChecked(source_settings.transcript_settings.enabled);
    update_combobox_output_target(*transcriptSaveForComboBox,
                                  source_settings.transcript_settings.streaming_transcripts_enabled,
                                  source_settings.transcript_settings.recording_transcripts_enabled);
    transcriptFolderPathLineEdit->setText(QString::fromStdString(source_settings.transcript_settings.output_path));

    set_show_key(true);
}

void CaptionSettingsWidget::set_settings(const CaptionPluginSettings &new_settings) {
    debug_log("CaptionSettingsWidget set_settings");
    current_settings = new_settings;
    scene_collection_name = current_scene_collection_name();
    updateUi();
}

void CaptionSettingsWidget::update_other_source_visibility(CaptionSourceMuteType mute_state) {
    bool be_visible = mute_state == CAPTION_SOURCE_MUTE_TYPE_USE_OTHER_MUTE_SOURCE;
    this->muteSourceComboBox->setVisible(be_visible);
    this->muteSourceLabel->setVisible(be_visible);

}

void CaptionSettingsWidget::on_transcriptFolderPickerPushButton_clicked() {
    QString dir_path = QFileDialog::getExistingDirectory(this,
                                                         tr("Open Directory"),
                                                         transcriptFolderPathLineEdit->text(),
                                                         QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

    if (dir_path.length()) {
        transcriptFolderPathLineEdit->setText(dir_path);
    }
}

