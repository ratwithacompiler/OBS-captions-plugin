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
#include "uiutils.h"
#include "../data.h"

#define WORD_REPLACEMENT_TYPE_INDEX 0
#define WORD_REPLACEMENT_FROM_INDEX 1
#define WORD_REPLACEMENT_TO_INDEX 2
#define WORD_REPLACEMENT_DELETE_INDEX 3

#define AUDIO_TRACK_COUNT 6


static void setup_combobox_texts(QComboBox &comboBox,
                                 const vector<string> &items
) {
    while (comboBox.count())
        comboBox.removeItem(0);

    for (auto &a_item : items) {
        comboBox.addItem(QString::fromStdString(a_item));
    }
}

static void setup_combobox_texts_data(QComboBox &comboBox, const vector<string> &items
) {
    while (comboBox.count())
        comboBox.removeItem(0);

    for (auto &a_item : items) {
        auto str = QString::fromStdString(a_item);
        comboBox.addItem(str, str);
    }
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


QComboBox *wordReplacementTypeComboBox(const char *default_text) {
    auto *cb = new QComboBox();
    cb->addItem("Text", "text_case_insensitive"); //(Ignore Case)
//    cb->addItem("Text", "text_case_sensitive");
    cb->addItem("Regex", "regex_case_insensitive"); //(Ignore Case)
//    cb->addItem("Regex", "regex_case_sensitive");
    combobox_set_data_str(*cb, default_text, 0);

    return cb;
}

QPushButton *wordReplacementDeletePushButton(QTableWidget *tableWidget) {
    auto *button = new QPushButton("X");
    QObject::connect(button, &QPushButton::clicked, tableWidget, [=]() {
        int foundAt = -1;
        for (int row = 0; row < tableWidget->rowCount(); row++) {
            const QWidget *wid = tableWidget->cellWidget(row, WORD_REPLACEMENT_DELETE_INDEX);
            if (wid == button) {
                foundAt = row;
                break;
            }
        }

        if (foundAt != -1) {
            tableWidget->removeRow(foundAt);
//            printf("remove index %d\n", foundAt);
        } else {
//            printf("cant delete, index not found\n");
        }
    });
    return button;
}

void setupReplacementTableWidget(QTableWidget *tableWidget, const std::vector<WordReplacement> &replacements) {
    tableWidget->setRowCount(0);

    int row = 0;
//    printf("setupReplacementTableWidget %d\n", (int) replacements.size());
    for (auto &word : replacements) {
        tableWidget->setRowCount(row + 1);

        tableWidget->setCellWidget(row, WORD_REPLACEMENT_TYPE_INDEX, wordReplacementTypeComboBox(word.get_type().c_str()));

        auto from = new QTableWidgetItem(QString::fromStdString(word.get_from()));
        tableWidget->setItem(row, WORD_REPLACEMENT_FROM_INDEX, from);

        auto to = new QTableWidgetItem(QString::fromStdString(word.get_to()));
        tableWidget->setItem(row, WORD_REPLACEMENT_TO_INDEX, to);

        tableWidget->setCellWidget(row, WORD_REPLACEMENT_DELETE_INDEX, wordReplacementDeletePushButton(tableWidget));

        row++;
    }

}

void getReplacements(QTableWidget *tableWidget, std::vector<WordReplacement> &replacements) {
    replacements.clear();
    for (int row = 0; row < tableWidget->rowCount(); row++) {
        string type;
        string from;
        string to;

        QComboBox *typeBox = dynamic_cast<QComboBox *>(tableWidget->cellWidget(row, WORD_REPLACEMENT_TYPE_INDEX));
        if (typeBox) {
            auto typeString = typeBox->currentData().toString().toStdString();
            if (isValidWordReplacementTypeString(typeString))
                type = typeString;
        }

        if (QTableWidgetItem *item = tableWidget->item(row, WORD_REPLACEMENT_FROM_INDEX)) {
            from = item->text().toStdString();
        }

        if (QTableWidgetItem *item = tableWidget->item(row, WORD_REPLACEMENT_TO_INDEX)) {
            to = item->text().toStdString();
        }

        if (!type.empty() && !from.empty()) {
            replacements.push_back(WordReplacement(type, from, to));
        }
    }
}

CaptionSettingsWidget::CaptionSettingsWidget(const CaptionPluginSettings &latest_settings)
        : QWidget(),
          Ui_CaptionSettingsWidget(),
          current_settings(latest_settings) {
    setupUi(this);

    QString with_version = bottomTextBrowser->toPlainText().replace("${VERSION_STRING}", VERSION_STRING);
    bottomTextBrowser->setPlainText(with_version);

    this->verticalLayout->setAlignment(Qt::AlignTop);
    this->errorLabel->hide();
    this->updateUi();

#if ENABLE_CUSTOM_API_KEY
    this->apiKeyLabel->show();
    this->apiKeyWidget->show();
#else
    this->apiKeyLabel->hide();
    this->apiKeyWidget->hide();
#endif

    captionWhenComboBox->addItem("Caption Source is heard on stream", "own_source");
    captionWhenComboBox->addItem("Mute Source is heard on stream", "other_mute_source");

    setup_combobox_languages(*languageComboBox);
    setup_combobox_profanity(*profanityFilterComboBox);
    setup_combobox_capitalization(*capitalizationComboBox);
    setup_combobox_capitalization(*textSourceCapitalizationComboBox);
    setup_combobox_output_target(*outputTargetComboBox, true);
    setup_combobox_output_target(*transcriptSaveForComboBox, false);
    setup_combobox_transcript_format(*transcriptFormatComboBox);
    setup_combobox_recording_filename(*recordingTranscriptFilenameComboBox);
    setup_combobox_streaming_filename(*streamingTranscriptFilenameComboBox);
    setup_combobox_transcript_file_exists(*recordingTranscriptCustomNameExistsCombobox);
    setup_combobox_transcript_file_exists(*streamingTranscriptCustomNameExistsCombobox);

    QObject::connect(this->cancelPushButton, &QPushButton::clicked, this, &CaptionSettingsWidget::hide);
    QObject::connect(this->savePushButton, &QPushButton::clicked, this, &CaptionSettingsWidget::accept_current_settings);

    QObject::connect(captionWhenComboBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
                     this, &CaptionSettingsWidget::caption_when_index_change);

    QObject::connect(transcriptFormatComboBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
                     this, &CaptionSettingsWidget::transcript_format_index_change);

    QObject::connect(languageComboBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
                     this, &CaptionSettingsWidget::language_index_change);

    QObject::connect(recordingTranscriptFilenameComboBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
                     this, &CaptionSettingsWidget::recording_name_index_change);

    QObject::connect(streamingTranscriptFilenameComboBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
                     this, &CaptionSettingsWidget::streaming_name_index_change);

    QObject::connect(sourcesComboBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
                     this, &CaptionSettingsWidget::sources_combo_index_change);
//
//    QObject::connect(captionWhenComboBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
//                     this, &CaptionSettingsWidget::sources_combo_index_change);
//
//    QObject::connect(muteSourceComboBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
//                     this, &CaptionSettingsWidget::sources_combo_index_change);

//    QObject::connect(sceneCollectionComboBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
//                     this, &CaptionSettingsWidget::scene_collection_combo_index_change);

    QStringList labels = {"Type", "Replace This", "With This", ""};
    this->wordReplacementTableWidget->setHorizontalHeaderLabels(labels);
    this->wordReplacementTableWidget->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
    this->wordReplacementTableWidget->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    this->wordReplacementTableWidget->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    this->wordReplacementTableWidget->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Fixed);
    this->wordReplacementTableWidget->setColumnWidth(0, 80);
    this->wordReplacementTableWidget->setColumnWidth(3, 25);
}


void CaptionSettingsWidget::set_show_key(bool set_to_show) {
    if (set_to_show) {
        apiKeyLineEdit->setEchoMode(QLineEdit::EchoMode::Normal);
        apiKeyShowPushButton->setText("Hide");
    } else {
        apiKeyLineEdit->setEchoMode(QLineEdit::EchoMode::Password);
        apiKeyShowPushButton->setText("Show");
    }
}

void CaptionSettingsWidget::on_apiKeyShowPushButton_clicked() {
    if (apiKeyLineEdit->echoMode() == QLineEdit::EchoMode::Password) {
        set_show_key(true);
    } else {
        set_show_key(false);
    }
}

void CaptionSettingsWidget::on_previewPushButton_clicked() {
    emit preview_requested();
}

void CaptionSettingsWidget::sources_combo_index_change(int new_index) {
    this->update_sources_visibilities();
}

void CaptionSettingsWidget::caption_when_index_change(int new_index) {
    this->update_sources_visibilities();
}

void CaptionSettingsWidget::update_sources_visibilities() {
    CaptionSourceMuteType mute_state = string_to_mute_setting(captionWhenComboBox->currentData().toString().toStdString(),
                                                              CAPTION_SOURCE_MUTE_TYPE_FROM_OWN_SOURCE);

    bool caption_when_visible = false;
    bool mute_source_visible = false;

    bool all_audio_capture_enabled = is_all_audio_output_capture_source_data(sourcesComboBox->currentData().toString().toStdString());
    if (all_audio_capture_enabled) {
        caption_when_visible = false;
        mute_source_visible = false;
    } else {
        caption_when_visible = true;
        mute_source_visible = mute_state == CAPTION_SOURCE_MUTE_TYPE_USE_OTHER_MUTE_SOURCE;
    }

    this->muteSourceComboBox->setVisible(mute_source_visible);
    this->muteSourceLabel->setVisible(mute_source_visible);

    captionWhenComboBox->setVisible(caption_when_visible);
    captionWhenLabel->setVisible(caption_when_visible);
}

void CaptionSettingsWidget::apply_ui_scene_collection_settings() {
    string current_scene_collection_name = scene_collection_name;
//    string current_scene_collection_name = this->sceneCollectionComboBox->currentText().toStdString();

    debug_log("apply_ui_scene_collection_settings %s", current_scene_collection_name.c_str());

    SceneCollectionSettings scene_col_settings;
    CaptionSourceSettings &cap_source_settings = scene_col_settings.caption_source_settings;

    cap_source_settings.caption_source_name = sourcesComboBox->currentData().toString().toStdString();
    cap_source_settings.mute_source_name = muteSourceComboBox->currentText().toStdString();

    string when_str = captionWhenComboBox->currentData().toString().toStdString();
//    debug_log("accepting when: %s", when_str.c_str());
    cap_source_settings.mute_when = string_to_mute_setting(when_str, CAPTION_SOURCE_MUTE_TYPE_FROM_OWN_SOURCE);
//    debug_log("accepting when: %d", current_settings.caption_source_settings.mute_when);

    TextOutputSettings &text_output_settings = scene_col_settings.text_output_settings;
    text_output_settings.enabled = this->textSourceEnableOutputCheckBox->isChecked();
    text_output_settings.text_source_name = this->textSourceOutputComboBox->currentText().toStdString();
    text_output_settings.line_count = this->textSourceLineCountSpinBox->value();
    text_output_settings.line_length = this->textSourceLineLengthSpinBox->value();
    text_output_settings.capitalization = (CapitalizationType) this->textSourceCapitalizationComboBox->currentData().toInt();
//    text_output_settings.insert_newlines = this->textSourceForceLinebreaksCheckBox->isChecked();

    current_settings.source_cap_settings.update_setting(current_scene_collection_name, scene_col_settings);
}

void CaptionSettingsWidget::scene_collection_combo_index_change(int new_index) {
//    string current_scene_collection_name = this->sceneCollectionComboBox->currentText().toStdString();
//    debug_log("scene_collection_combo_index_change %s", current_scene_collection_name.c_str());
//    update_scene_collection_ui(current_scene_collection_name);
}

void CaptionSettingsWidget::update_scene_collection_ui(const string &use_scene_collection_name) {
    SourceCaptionerSettings &source_settings = current_settings.source_cap_settings;

    SceneCollectionSettings use_settings = default_SceneCollectionSettings();
    {
        use_settings = source_settings.get_scene_collection_settings(use_scene_collection_name);
        debug_log("update_scene_collection_ui using specific settings, '%s'", use_scene_collection_name.c_str());
    }

    // audio input sources
    auto audio_sources = get_audio_sources();
    audio_sources.insert(audio_sources.begin(), "");
    audio_sources.insert(audio_sources.end(), "");

    const QSignalBlocker blocker1(sourcesComboBox);
    setup_combobox_texts_data(*sourcesComboBox, audio_sources);

    for (int i = 0; i < AUDIO_TRACK_COUNT; i++)
        sourcesComboBox->addItem(QString::fromStdString(all_audio_output_capture_source_name(i)),
                                 QString::fromStdString(all_audio_output_capture_source_data(i)));

    const QSignalBlocker blocker2(muteSourceComboBox);
    setup_combobox_texts(*muteSourceComboBox, audio_sources);

    const QSignalBlocker blocker3(captionWhenComboBox);

    combobox_set_data_str(*sourcesComboBox, use_settings.caption_source_settings.caption_source_name.c_str(), 0);
    muteSourceComboBox->setCurrentText(QString::fromStdString(use_settings.caption_source_settings.mute_source_name));

//    debug_log("use_settings.caption_source_settings.mute_when %d", use_settings.caption_source_settings.mute_when);
    string mute_when_str = mute_setting_to_string(use_settings.caption_source_settings.mute_when, "own_source");
    combobox_set_data_str(*captionWhenComboBox, mute_when_str.c_str(), 0);

    this->update_sources_visibilities();

    // text output source
    const QSignalBlocker blocker4(textSourceOutputComboBox);

    this->textSourceEnableOutputCheckBox->setChecked(use_settings.text_output_settings.enabled);
    on_textSourceEnableOutputCheckBox_stateChanged(0);

    auto text_sources = get_text_sources();
    text_sources.insert(text_sources.begin(), "");
    setup_combobox_texts(*textSourceOutputComboBox, text_sources);

    this->textSourceLineLengthSpinBox->setValue(use_settings.text_output_settings.line_length);
    this->textSourceLineCountSpinBox->setValue(use_settings.text_output_settings.line_count);
    combobox_set_data_int(*textSourceCapitalizationComboBox, use_settings.text_output_settings.capitalization, 0);
//    this->textSourceForceLinebreaksCheckBox->setChecked(use_settings.text_output_settings.insert_newlines);

    this->textSourceOutputComboBox->setCurrentText(QString::fromStdString(use_settings.text_output_settings.text_source_name));
}

void CaptionSettingsWidget::transcript_format_index_change(int new_index) {
    const bool isSrt = transcriptFormatComboBox->currentData().toString().toStdString() == "srt";
    transcriptSrtSettingsWidget->setVisible(isSrt);

    const auto format = transcriptFormatComboBox->currentData().toString().toStdString();
    const QString extension = QString::fromStdString(transcript_format_extension(format, "[ext]"));
    update_combobox_recording_filename(*recordingTranscriptFilenameComboBox, extension);
    update_combobox_streaming_filename(*streamingTranscriptFilenameComboBox, extension);
}

void CaptionSettingsWidget::language_index_change(int new_index) {
    string lang_str = languageComboBox->currentData().toString().toStdString();

    const bool native_output = supports_native_output(lang_str);
    languageWarningLabel->setVisible(!native_output);

//    this->nativeForceLinebreaksLabel->setVisible(native_output);
//    this->nativeLineCountLabel->setVisible(native_output);
//    this->nativeLinebreaksNotRecommendendLabel->setVisible(native_output);
//    this->nativeOutputToLabel->setVisible(native_output);
//
//    this->outputTargetComboBox->setVisible(native_output);
//    this->lineCountSpinBox->setVisible(native_output);
//    this->insertLinebreaksCheckBox->setVisible(native_output);
}

void CaptionSettingsWidget::recording_name_index_change(int new_index) {
    bool isCustom = this->recordingTranscriptFilenameComboBox->currentData().toString() == "custom";

    recordingTranscriptCustomNameWidget->setVisible(isCustom);
    recordingTranscriptCustomNameOverwriteLineEdit->setVisible(isCustom);
}

void CaptionSettingsWidget::streaming_name_index_change(int new_index) {
    bool isCustom = this->streamingTranscriptFilenameComboBox->currentData().toString() == "custom";

    streamingTranscriptCustomNameWidget->setVisible(isCustom);
    streamingTranscriptCustomNameOverwriteLineEdit->setVisible(isCustom);
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
    source_settings.format_settings.capitalization = (CapitalizationType) capitalizationComboBox->currentData().toInt();

    source_settings.format_settings.caption_insert_newlines = insertLinebreaksCheckBox->isChecked();
    current_settings.enabled = enabledCheckBox->isChecked();

    // just for now as the foreign langs list might have errors or some only partially use non native charsets
    const bool still_try_foreign_langs = true;

//    const bool native_output = supports_native_output(lang_str);
    if (still_try_foreign_langs || supports_native_output(lang_str)) {
        const int output_combobox_val = outputTargetComboBox->currentData().toInt();
        if (!set_streaming_recording_enabled(output_combobox_val,
                                             source_settings.streaming_output_enabled, source_settings.recording_output_enabled)) {
            error_log("invalid output target combobox value, wtf: %d", output_combobox_val);
        }
    } else {
        source_settings.streaming_output_enabled = false;
        source_settings.recording_output_enabled = false;
    }

    source_settings.format_settings.caption_timeout_enabled = this->captionTimeoutEnabledCheckBox->isChecked();
    source_settings.format_settings.caption_timeout_seconds = this->captionTimeoutDoubleSpinBox->value();

    auto reps = std::vector<WordReplacement>();
    getReplacements(wordReplacementTableWidget, reps);
    source_settings.format_settings.replacer = makeDefaultReplacer(reps);

    auto &transcript_settings = source_settings.transcript_settings;
    transcript_settings.enabled = saveTranscriptsCheckBox->isChecked();
    const int transcript_combobox_val = transcriptSaveForComboBox->currentData().toInt();
    if (!set_streaming_recording_enabled(transcript_combobox_val,
                                         transcript_settings.streaming_transcripts_enabled,
                                         transcript_settings.recording_transcripts_enabled)) {
        error_log("invalid transcript output target combobox value, wtf: %d", transcript_combobox_val);
    }
    transcript_settings.output_path = transcriptFolderPathLineEdit->text().toStdString();
    transcript_settings.format = transcriptFormatComboBox->currentData().toString().toStdString();
    transcript_settings.srt_target_duration_secs = srtDurationSpinBox->value();
    transcript_settings.srt_target_line_length = srtLineLenghtSpinBox->value();

    transcript_settings.recording_filename_type = recordingTranscriptFilenameComboBox->currentData().toString().toStdString();
    transcript_settings.recording_filename_custom = recordingTranscriptCustomNameOverwriteLineEdit->text().toStdString();
    transcript_settings.recording_filename_exists = recordingTranscriptCustomNameExistsCombobox->currentData().toString().toStdString();

    transcript_settings.streaming_filename_type = streamingTranscriptFilenameComboBox->currentData().toString().toStdString();
    transcript_settings.streaming_filename_custom = streamingTranscriptCustomNameOverwriteLineEdit->text().toStdString();
    transcript_settings.streaming_filename_exists = streamingTranscriptCustomNameExistsCombobox->currentData().toString().toStdString();

    apply_ui_scene_collection_settings();

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
//    update_scene_collection_ui(sceneCollectionComboBox->currentText().toStdString());

//    sceneCollectionSelectLabel->hide();
//    sceneCollectionComboBox->hide();

    sceneCollectionNameLabel_GeneralRight->setText(QString::fromStdString(scene_collection_name));
    sceneCollectionNameLabel_TextSourceRight->setText(QString::fromStdString(scene_collection_name));
    update_scene_collection_ui(scene_collection_name);

    combobox_set_data_str(*languageComboBox, source_settings.stream_settings.stream_settings.language.c_str(), 0);
    language_index_change(0);
    combobox_set_data_int(*profanityFilterComboBox, source_settings.stream_settings.stream_settings.profanity_filter, 0);

    apiKeyLineEdit->setText(QString::fromStdString(source_settings.stream_settings.stream_settings.api_key));

    lineCountSpinBox->setValue(source_settings.format_settings.caption_line_count);
    insertLinebreaksCheckBox->setChecked(source_settings.format_settings.caption_insert_newlines);
    combobox_set_data_int(*capitalizationComboBox, source_settings.format_settings.capitalization, 0);

    enabledCheckBox->setChecked(current_settings.enabled);
    on_enabledCheckBox_stateChanged(0);
    update_combobox_output_target(*outputTargetComboBox,
                                  source_settings.streaming_output_enabled,
                                  source_settings.recording_output_enabled,
                                  0, true);

    this->captionTimeoutEnabledCheckBox->setChecked(source_settings.format_settings.caption_timeout_enabled);
    this->captionTimeoutDoubleSpinBox->setValue(source_settings.format_settings.caption_timeout_seconds);

    setupReplacementTableWidget(this->wordReplacementTableWidget, source_settings.format_settings.replacer.user_replacements());

    saveTranscriptsCheckBox->setChecked(source_settings.transcript_settings.enabled);
    on_saveTranscriptsCheckBox_stateChanged(0);
    update_combobox_output_target(*transcriptSaveForComboBox,
                                  source_settings.transcript_settings.streaming_transcripts_enabled,
                                  source_settings.transcript_settings.recording_transcripts_enabled,
                                  0, false);
    transcriptFolderPathLineEdit->setText(QString::fromStdString(source_settings.transcript_settings.output_path));
    combobox_set_data_str(*transcriptFormatComboBox, source_settings.transcript_settings.format.c_str(), 0);
    transcript_format_index_change(0);
    srtDurationSpinBox->setValue(source_settings.transcript_settings.srt_target_duration_secs);
    srtLineLenghtSpinBox->setValue(source_settings.transcript_settings.srt_target_line_length);

    combobox_set_data_str(*recordingTranscriptFilenameComboBox, source_settings.transcript_settings.recording_filename_type.c_str(), 0);
    combobox_set_data_str(*recordingTranscriptCustomNameExistsCombobox,
                          source_settings.transcript_settings.recording_filename_exists.c_str(), 0);
    recordingTranscriptCustomNameOverwriteLineEdit->setText(
            QString::fromStdString(source_settings.transcript_settings.recording_filename_custom));


    combobox_set_data_str(*streamingTranscriptFilenameComboBox, source_settings.transcript_settings.streaming_filename_type.c_str(), 0);
    combobox_set_data_str(*streamingTranscriptCustomNameExistsCombobox,
                          source_settings.transcript_settings.streaming_filename_exists.c_str(), 0);
    streamingTranscriptCustomNameOverwriteLineEdit->setText(
            QString::fromStdString(source_settings.transcript_settings.streaming_filename_custom));

    recording_name_index_change(0);
    streaming_name_index_change(0);

    set_show_key(false);
}

void CaptionSettingsWidget::set_settings(const CaptionPluginSettings &new_settings) {
    debug_log("CaptionSettingsWidget set_settings");
    current_settings = new_settings;
    scene_collection_name = current_scene_collection_name();
    updateUi();
}


void CaptionSettingsWidget::on_replacementWordsAddWordPushButton_clicked() {
    const int row = wordReplacementTableWidget->rowCount();
    wordReplacementTableWidget->setRowCount(row + 1);
    wordReplacementTableWidget->setCellWidget(row, WORD_REPLACEMENT_TYPE_INDEX,
                                              wordReplacementTypeComboBox("text_case_insensitive"));
    wordReplacementTableWidget->setCellWidget(row, WORD_REPLACEMENT_DELETE_INDEX,
                                              wordReplacementDeletePushButton(wordReplacementTableWidget));
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

void CaptionSettingsWidget::on_saveTranscriptsCheckBox_stateChanged(int new_state) {
    const bool disabled = !saveTranscriptsCheckBox->isChecked();
    this->transcripWidget1->setDisabled(disabled);
    this->transcripWidget2->setDisabled(disabled);
    this->transcripWidget3->setDisabled(disabled);
    this->transcripWidget4->setDisabled(disabled);
    this->transcriptSrtSettingsWidget->setDisabled(disabled);
}

void CaptionSettingsWidget::on_textSourceEnableOutputCheckBox_stateChanged(int new_state) {
    const bool disabled = !textSourceEnableOutputCheckBox->isChecked();
    this->textSourceWidget1->setDisabled(disabled);
    this->textSourceWidget2->setDisabled(disabled);
}

void CaptionSettingsWidget::on_enabledCheckBox_stateChanged(int new_state) {
//    const bool disabled = !enabledCheckBox->isChecked();
//    this->tabWidget->setDisabled(disabled);
}
