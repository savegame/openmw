#include "settingspage.hpp"

#include <array>
#include <cmath>
#include <string>

#include <QCompleter>
#include <QFileDialog>
#include <QString>

#include <components/config/gamesettings.hpp>

#include "utils/openalutil.hpp"

Launcher::SettingsPage::SettingsPage(Config::GameSettings& gameSettings, QWidget* parent)
    : QWidget(parent)
    , mGameSettings(gameSettings)
{
    setObjectName("SettingsPage");
    setupUi(this);

    for (const std::string& name : Launcher::enumerateOpenALDevices())
    {
        audioDeviceSelectorComboBox->addItem(QString::fromStdString(name), QString::fromStdString(name));
    }
    for (const std::string& name : Launcher::enumerateOpenALDevicesHrtf())
    {
        hrtfProfileSelectorComboBox->addItem(QString::fromStdString(name), QString::fromStdString(name));
    }

    loadSettings();

    mCellNameCompleter.setModel(&mCellNameCompleterModel);
    startDefaultCharacterAtField->setCompleter(&mCellNameCompleter);
}

void Launcher::SettingsPage::loadCellsForAutocomplete(QStringList cellNames)
{
    // Update the list of suggestions for the "Start default character at" field
    mCellNameCompleterModel.setStringList(cellNames);
    mCellNameCompleter.setCompletionMode(QCompleter::PopupCompletion);
    mCellNameCompleter.setCaseSensitivity(Qt::CaseSensitivity::CaseInsensitive);
}

void Launcher::SettingsPage::on_skipMenuCheckBox_stateChanged(int state)
{
    startDefaultCharacterAtLabel->setEnabled(state == Qt::Checked);
    startDefaultCharacterAtField->setEnabled(state == Qt::Checked);
}

void Launcher::SettingsPage::on_runScriptAfterStartupBrowseButton_clicked()
{
    QString scriptFile = QFileDialog::getOpenFileName(
        this, QObject::tr("Select script file"), QDir::currentPath(), QString(tr("Text file (*.txt)")));

    if (scriptFile.isEmpty())
        return;

    QFileInfo info(scriptFile);

    if (!info.exists() || !info.isReadable())
        return;

    const QString path(QDir::toNativeSeparators(info.absoluteFilePath()));
    runScriptAfterStartupField->setText(path);
}

namespace
{
    constexpr double CellSizeInUnits = 8192;

    double convertToCells(double unitRadius)
    {
        return unitRadius / CellSizeInUnits;
    }

    int convertToUnits(double CellGridRadius)
    {
        return static_cast<int>(CellSizeInUnits * CellGridRadius);
    }
}

bool Launcher::SettingsPage::loadSettings()
{
    // Game mechanics
    {
        loadSettingBool(canLootDuringDeathAnimationCheckBox, "can loot during death animation", "Game");
        loadSettingBool(followersAttackOnSightCheckBox, "followers attack on sight", "Game");
        loadSettingBool(rebalanceSoulGemValuesCheckBox, "rebalance soul gem values", "Game");
        loadSettingBool(enchantedWeaponsMagicalCheckBox, "enchanted weapons are magical", "Game");
        loadSettingBool(permanentBarterDispositionChangeCheckBox, "barter disposition change is permanent", "Game");
        loadSettingBool(classicReflectedAbsorbSpellsCheckBox, "classic reflected absorb spells behavior", "Game");
        loadSettingBool(classicCalmSpellsCheckBox, "classic calm spells behavior", "Game");
        loadSettingBool(
            requireAppropriateAmmunitionCheckBox, "only appropriate ammunition bypasses resistance", "Game");
        loadSettingBool(uncappedDamageFatigueCheckBox, "uncapped damage fatigue", "Game");
        loadSettingBool(normaliseRaceSpeedCheckBox, "normalise race speed", "Game");
        loadSettingBool(swimUpwardCorrectionCheckBox, "swim upward correction", "Game");
        loadSettingBool(avoidCollisionsCheckBox, "NPCs avoid collisions", "Game");
        int unarmedFactorsStrengthIndex = Settings::Manager::getInt("strength influences hand to hand", "Game");
        if (unarmedFactorsStrengthIndex >= 0 && unarmedFactorsStrengthIndex <= 2)
            unarmedFactorsStrengthComboBox->setCurrentIndex(unarmedFactorsStrengthIndex);
        loadSettingBool(stealingFromKnockedOutCheckBox, "always allow stealing from knocked out actors", "Game");
        loadSettingBool(enableNavigatorCheckBox, "enable", "Navigator");
        int numPhysicsThreads = Settings::Manager::getInt("async num threads", "Physics");
        if (numPhysicsThreads >= 0)
            physicsThreadsSpinBox->setValue(numPhysicsThreads);
        loadSettingBool(allowNPCToFollowOverWaterSurfaceCheckBox, "allow actors to follow over water surface", "Game");
        loadSettingBool(unarmedCreatureAttacksDamageArmorCheckBox, "unarmed creature attacks damage armor", "Game");
        const int actorCollisionShapeType = Settings::Manager::getInt("actor collision shape type", "Game");
        if (0 <= actorCollisionShapeType && actorCollisionShapeType < actorCollisonShapeTypeComboBox->count())
            actorCollisonShapeTypeComboBox->setCurrentIndex(actorCollisionShapeType);
    }

    // Visuals
    {
        loadSettingBool(autoUseObjectNormalMapsCheckBox, "auto use object normal maps", "Shaders");
        loadSettingBool(autoUseObjectSpecularMapsCheckBox, "auto use object specular maps", "Shaders");
        loadSettingBool(autoUseTerrainNormalMapsCheckBox, "auto use terrain normal maps", "Shaders");
        loadSettingBool(autoUseTerrainSpecularMapsCheckBox, "auto use terrain specular maps", "Shaders");
        loadSettingBool(bumpMapLocalLightingCheckBox, "apply lighting to environment maps", "Shaders");
        loadSettingBool(softParticlesCheckBox, "soft particles", "Shaders");
        loadSettingBool(antialiasAlphaTestCheckBox, "antialias alpha test", "Shaders");
        if (Settings::Manager::getInt("antialiasing", "Video") == 0)
        {
            antialiasAlphaTestCheckBox->setCheckState(Qt::Unchecked);
        }
        loadSettingBool(adjustCoverageForAlphaTestCheckBox, "adjust coverage for alpha test", "Shaders");
        loadSettingBool(weatherParticleOcclusionCheckBox, "weather particle occlusion", "Shaders");
        loadSettingBool(magicItemAnimationsCheckBox, "use magic item animations", "Game");
        connect(animSourcesCheckBox, &QCheckBox::toggled, this, &SettingsPage::slotAnimSourcesToggled);
        loadSettingBool(animSourcesCheckBox, "use additional anim sources", "Game");
        if (animSourcesCheckBox->checkState() != Qt::Unchecked)
        {
            loadSettingBool(weaponSheathingCheckBox, "weapon sheathing", "Game");
            loadSettingBool(shieldSheathingCheckBox, "shield sheathing", "Game");
        }
        loadSettingBool(turnToMovementDirectionCheckBox, "turn to movement direction", "Game");
        loadSettingBool(smoothMovementCheckBox, "smooth movement", "Game");

        const bool distantTerrain = Settings::Manager::getBool("distant terrain", "Terrain");
        const bool objectPaging = Settings::Manager::getBool("object paging", "Terrain");
        if (distantTerrain && objectPaging)
        {
            distantLandCheckBox->setCheckState(Qt::Checked);
        }

        loadSettingBool(activeGridObjectPagingCheckBox, "object paging active grid", "Terrain");
        viewingDistanceComboBox->setValue(convertToCells(Settings::Manager::getInt("viewing distance", "Camera")));
        objectPagingMinSizeComboBox->setValue(Settings::Manager::getDouble("object paging min size", "Terrain"));

        loadSettingBool(nightDaySwitchesCheckBox, "day night switches", "Game");

        connect(postprocessEnabledCheckBox, &QCheckBox::toggled, this, &SettingsPage::slotPostProcessToggled);
        loadSettingBool(postprocessEnabledCheckBox, "enabled", "Post Processing");
        loadSettingBool(postprocessTransparentPostpassCheckBox, "transparent postpass", "Post Processing");
        postprocessHDRTimeComboBox->setValue(Settings::Manager::getDouble("auto exposure speed", "Post Processing"));

        connect(skyBlendingCheckBox, &QCheckBox::toggled, this, &SettingsPage::slotSkyBlendingToggled);
        loadSettingBool(radialFogCheckBox, "radial fog", "Fog");
        loadSettingBool(exponentialFogCheckBox, "exponential fog", "Fog");
        loadSettingBool(skyBlendingCheckBox, "sky blending", "Fog");
        skyBlendingStartComboBox->setValue(Settings::Manager::getDouble("sky blending start", "Fog"));
    }

    // Audio
    {
        const std::string& selectedAudioDevice = Settings::Manager::getString("device", "Sound");
        if (selectedAudioDevice.empty() == false)
        {
            int audioDeviceIndex = audioDeviceSelectorComboBox->findData(QString::fromStdString(selectedAudioDevice));
            if (audioDeviceIndex != -1)
            {
                audioDeviceSelectorComboBox->setCurrentIndex(audioDeviceIndex);
            }
        }
        int hrtfEnabledIndex = Settings::Manager::getInt("hrtf enable", "Sound");
        if (hrtfEnabledIndex >= -1 && hrtfEnabledIndex <= 1)
        {
            enableHRTFComboBox->setCurrentIndex(hrtfEnabledIndex + 1);
        }
        const std::string& selectedHRTFProfile = Settings::Manager::getString("hrtf", "Sound");
        if (selectedHRTFProfile.empty() == false)
        {
            int hrtfProfileIndex = hrtfProfileSelectorComboBox->findData(QString::fromStdString(selectedHRTFProfile));
            if (hrtfProfileIndex != -1)
            {
                hrtfProfileSelectorComboBox->setCurrentIndex(hrtfProfileIndex);
            }
        }
    }

    // Interface Changes
    {
        loadSettingBool(showEffectDurationCheckBox, "show effect duration", "Game");
        loadSettingBool(showEnchantChanceCheckBox, "show enchant chance", "Game");
        loadSettingBool(showMeleeInfoCheckBox, "show melee info", "Game");
        loadSettingBool(showProjectileDamageCheckBox, "show projectile damage", "Game");
        loadSettingBool(changeDialogTopicsCheckBox, "color topic enable", "GUI");
        int showOwnedIndex = Settings::Manager::getInt("show owned", "Game");
        // Match the index with the option (only 0, 1, 2, or 3 are valid). Will default to 0 if invalid.
        if (showOwnedIndex >= 0 && showOwnedIndex <= 3)
            showOwnedComboBox->setCurrentIndex(showOwnedIndex);
        loadSettingBool(stretchBackgroundCheckBox, "stretch menu background", "GUI");
        loadSettingBool(useZoomOnMapCheckBox, "allow zooming", "Map");
        loadSettingBool(graphicHerbalismCheckBox, "graphic herbalism", "Game");
        scalingSpinBox->setValue(Settings::Manager::getFloat("scaling factor", "GUI"));
        fontSizeSpinBox->setValue(Settings::Manager::getInt("font size", "GUI"));
    }

    // Bug fixes
    {
        loadSettingBool(preventMerchantEquippingCheckBox, "prevent merchant equipping", "Game");
        loadSettingBool(
            trainersTrainingSkillsBasedOnBaseSkillCheckBox, "trainers training skills based on base skill", "Game");
    }

    // Miscellaneous
    {
        // Saves
        loadSettingBool(timePlayedCheckbox, "timeplayed", "Saves");
        loadSettingInt(maximumQuicksavesComboBox, "max quicksaves", "Saves");

        // Other Settings
        QString screenshotFormatString
            = QString::fromStdString(Settings::Manager::getString("screenshot format", "General")).toUpper();
        if (screenshotFormatComboBox->findText(screenshotFormatString) == -1)
            screenshotFormatComboBox->addItem(screenshotFormatString);
        screenshotFormatComboBox->setCurrentIndex(screenshotFormatComboBox->findText(screenshotFormatString));

        loadSettingBool(notifyOnSavedScreenshotCheckBox, "notify on saved screenshot", "General");
    }

    // Testing
    {
        loadSettingBool(grabCursorCheckBox, "grab cursor", "Input");

        bool skipMenu = mGameSettings.value("skip-menu").toInt() == 1;
        if (skipMenu)
        {
            skipMenuCheckBox->setCheckState(Qt::Checked);
        }
        startDefaultCharacterAtLabel->setEnabled(skipMenu);
        startDefaultCharacterAtField->setEnabled(skipMenu);

        startDefaultCharacterAtField->setText(mGameSettings.value("start"));
        runScriptAfterStartupField->setText(mGameSettings.value("script-run"));
    }
    return true;
}

void Launcher::SettingsPage::saveSettings()
{
    // Game mechanics
    {
        saveSettingBool(canLootDuringDeathAnimationCheckBox, "can loot during death animation", "Game");
        saveSettingBool(followersAttackOnSightCheckBox, "followers attack on sight", "Game");
        saveSettingBool(rebalanceSoulGemValuesCheckBox, "rebalance soul gem values", "Game");
        saveSettingBool(enchantedWeaponsMagicalCheckBox, "enchanted weapons are magical", "Game");
        saveSettingBool(permanentBarterDispositionChangeCheckBox, "barter disposition change is permanent", "Game");
        saveSettingBool(classicReflectedAbsorbSpellsCheckBox, "classic reflected absorb spells behavior", "Game");
        saveSettingBool(classicCalmSpellsCheckBox, "classic calm spells behavior", "Game");
        saveSettingBool(
            requireAppropriateAmmunitionCheckBox, "only appropriate ammunition bypasses resistance", "Game");
        saveSettingBool(uncappedDamageFatigueCheckBox, "uncapped damage fatigue", "Game");
        saveSettingBool(normaliseRaceSpeedCheckBox, "normalise race speed", "Game");
        saveSettingBool(swimUpwardCorrectionCheckBox, "swim upward correction", "Game");
        saveSettingBool(avoidCollisionsCheckBox, "NPCs avoid collisions", "Game");
        saveSettingInt(unarmedFactorsStrengthComboBox, "strength influences hand to hand", "Game");
        saveSettingBool(stealingFromKnockedOutCheckBox, "always allow stealing from knocked out actors", "Game");
        saveSettingBool(enableNavigatorCheckBox, "enable", "Navigator");
        saveSettingInt(physicsThreadsSpinBox, "async num threads", "Physics");
        saveSettingBool(allowNPCToFollowOverWaterSurfaceCheckBox, "allow actors to follow over water surface", "Game");
        saveSettingBool(unarmedCreatureAttacksDamageArmorCheckBox, "unarmed creature attacks damage armor", "Game");
        saveSettingInt(actorCollisonShapeTypeComboBox, "actor collision shape type", "Game");
    }

    // Visuals
    {
        saveSettingBool(autoUseObjectNormalMapsCheckBox, "auto use object normal maps", "Shaders");
        saveSettingBool(autoUseObjectSpecularMapsCheckBox, "auto use object specular maps", "Shaders");
        saveSettingBool(autoUseTerrainNormalMapsCheckBox, "auto use terrain normal maps", "Shaders");
        saveSettingBool(autoUseTerrainSpecularMapsCheckBox, "auto use terrain specular maps", "Shaders");
        saveSettingBool(bumpMapLocalLightingCheckBox, "apply lighting to environment maps", "Shaders");
        saveSettingBool(radialFogCheckBox, "radial fog", "Fog");
        saveSettingBool(softParticlesCheckBox, "soft particles", "Shaders");
        saveSettingBool(antialiasAlphaTestCheckBox, "antialias alpha test", "Shaders");
        saveSettingBool(adjustCoverageForAlphaTestCheckBox, "adjust coverage for alpha test", "Shaders");
        saveSettingBool(weatherParticleOcclusionCheckBox, "weather particle occlusion", "Shaders");
        saveSettingBool(magicItemAnimationsCheckBox, "use magic item animations", "Game");
        saveSettingBool(animSourcesCheckBox, "use additional anim sources", "Game");
        saveSettingBool(weaponSheathingCheckBox, "weapon sheathing", "Game");
        saveSettingBool(shieldSheathingCheckBox, "shield sheathing", "Game");
        saveSettingBool(turnToMovementDirectionCheckBox, "turn to movement direction", "Game");
        saveSettingBool(smoothMovementCheckBox, "smooth movement", "Game");

        const bool distantTerrain = Settings::Manager::getBool("distant terrain", "Terrain");
        const bool objectPaging = Settings::Manager::getBool("object paging", "Terrain");
        const bool wantDistantLand = distantLandCheckBox->checkState();
        if (wantDistantLand != (distantTerrain && objectPaging))
        {
            Settings::Manager::setBool("distant terrain", "Terrain", wantDistantLand);
            Settings::Manager::setBool("object paging", "Terrain", wantDistantLand);
        }

        saveSettingBool(activeGridObjectPagingCheckBox, "object paging active grid", "Terrain");
        int viewingDistance = convertToUnits(viewingDistanceComboBox->value());
        if (viewingDistance != Settings::Manager::getInt("viewing distance", "Camera"))
        {
            Settings::Manager::setInt("viewing distance", "Camera", viewingDistance);
        }
        double objectPagingMinSize = objectPagingMinSizeComboBox->value();
        if (objectPagingMinSize != Settings::Manager::getDouble("object paging min size", "Terrain"))
            Settings::Manager::setDouble("object paging min size", "Terrain", objectPagingMinSize);

        saveSettingBool(nightDaySwitchesCheckBox, "day night switches", "Game");

        saveSettingBool(postprocessEnabledCheckBox, "enabled", "Post Processing");
        saveSettingBool(postprocessTransparentPostpassCheckBox, "transparent postpass", "Post Processing");
        double hdrExposureTime = postprocessHDRTimeComboBox->value();
        if (hdrExposureTime != Settings::Manager::getDouble("auto exposure speed", "Post Processing"))
            Settings::Manager::setDouble("auto exposure speed", "Post Processing", hdrExposureTime);

        saveSettingBool(radialFogCheckBox, "radial fog", "Fog");
        saveSettingBool(exponentialFogCheckBox, "exponential fog", "Fog");
        saveSettingBool(skyBlendingCheckBox, "sky blending", "Fog");
        Settings::Manager::setDouble("sky blending start", "Fog", skyBlendingStartComboBox->value());
    }

    // Audio
    {
        int audioDeviceIndex = audioDeviceSelectorComboBox->currentIndex();
        const std::string& prevAudioDevice = Settings::Manager::getString("device", "Sound");
        if (audioDeviceIndex != 0)
        {
            const std::string& newAudioDevice = audioDeviceSelectorComboBox->currentText().toUtf8().constData();
            if (newAudioDevice != prevAudioDevice)
                Settings::Manager::setString("device", "Sound", newAudioDevice);
        }
        else if (!prevAudioDevice.empty())
        {
            Settings::Manager::setString("device", "Sound", {});
        }
        int hrtfEnabledIndex = enableHRTFComboBox->currentIndex() - 1;
        if (hrtfEnabledIndex != Settings::Manager::getInt("hrtf enable", "Sound"))
        {
            Settings::Manager::setInt("hrtf enable", "Sound", hrtfEnabledIndex);
        }
        int selectedHRTFProfileIndex = hrtfProfileSelectorComboBox->currentIndex();
        const std::string& prevHRTFProfile = Settings::Manager::getString("hrtf", "Sound");
        if (selectedHRTFProfileIndex != 0)
        {
            const std::string& newHRTFProfile = hrtfProfileSelectorComboBox->currentText().toUtf8().constData();
            if (newHRTFProfile != prevHRTFProfile)
                Settings::Manager::setString("hrtf", "Sound", newHRTFProfile);
        }
        else if (!prevHRTFProfile.empty())
        {
            Settings::Manager::setString("hrtf", "Sound", {});
        }
    }

    // Interface Changes
    {
        saveSettingBool(showEffectDurationCheckBox, "show effect duration", "Game");
        saveSettingBool(showEnchantChanceCheckBox, "show enchant chance", "Game");
        saveSettingBool(showMeleeInfoCheckBox, "show melee info", "Game");
        saveSettingBool(showProjectileDamageCheckBox, "show projectile damage", "Game");
        saveSettingBool(changeDialogTopicsCheckBox, "color topic enable", "GUI");
        saveSettingInt(showOwnedComboBox, "show owned", "Game");
        saveSettingBool(stretchBackgroundCheckBox, "stretch menu background", "GUI");
        saveSettingBool(useZoomOnMapCheckBox, "allow zooming", "Map");
        saveSettingBool(graphicHerbalismCheckBox, "graphic herbalism", "Game");

        float uiScalingFactor = scalingSpinBox->value();
        if (uiScalingFactor != Settings::Manager::getFloat("scaling factor", "GUI"))
            Settings::Manager::setFloat("scaling factor", "GUI", uiScalingFactor);

        int fontSize = fontSizeSpinBox->value();
        if (fontSize != Settings::Manager::getInt("font size", "GUI"))
            Settings::Manager::setInt("font size", "GUI", fontSize);
    }

    // Bug fixes
    {
        saveSettingBool(preventMerchantEquippingCheckBox, "prevent merchant equipping", "Game");
        saveSettingBool(
            trainersTrainingSkillsBasedOnBaseSkillCheckBox, "trainers training skills based on base skill", "Game");
    }

    // Miscellaneous
    {
        // Saves Settings
        saveSettingBool(timePlayedCheckbox, "timeplayed", "Saves");
        saveSettingInt(maximumQuicksavesComboBox, "max quicksaves", "Saves");

        // Other Settings
        std::string screenshotFormatString = screenshotFormatComboBox->currentText().toLower().toStdString();
        if (screenshotFormatString != Settings::Manager::getString("screenshot format", "General"))
            Settings::Manager::setString("screenshot format", "General", screenshotFormatString);

        saveSettingBool(notifyOnSavedScreenshotCheckBox, "notify on saved screenshot", "General");
    }

    // Testing
    {
        saveSettingBool(grabCursorCheckBox, "grab cursor", "Input");

        int skipMenu = skipMenuCheckBox->checkState() == Qt::Checked;
        if (skipMenu != mGameSettings.value("skip-menu").toInt())
            mGameSettings.setValue("skip-menu", QString::number(skipMenu));

        QString startCell = startDefaultCharacterAtField->text();
        if (startCell != mGameSettings.value("start"))
        {
            mGameSettings.setValue("start", startCell);
        }
        QString scriptRun = runScriptAfterStartupField->text();
        if (scriptRun != mGameSettings.value("script-run"))
            mGameSettings.setValue("script-run", scriptRun);
    }
}

void Launcher::SettingsPage::loadSettingBool(QCheckBox* checkbox, const std::string& setting, const std::string& group)
{
    if (Settings::Manager::getBool(setting, group))
        checkbox->setCheckState(Qt::Checked);
}

void Launcher::SettingsPage::saveSettingBool(QCheckBox* checkbox, const std::string& setting, const std::string& group)
{
    bool cValue = checkbox->checkState();
    if (cValue != Settings::Manager::getBool(setting, group))
        Settings::Manager::setBool(setting, group, cValue);
}

void Launcher::SettingsPage::loadSettingInt(QComboBox* comboBox, const std::string& setting, const std::string& group)
{
    int currentIndex = Settings::Manager::getInt(setting, group);
    comboBox->setCurrentIndex(currentIndex);
}

void Launcher::SettingsPage::saveSettingInt(QComboBox* comboBox, const std::string& setting, const std::string& group)
{
    int currentIndex = comboBox->currentIndex();
    if (currentIndex != Settings::Manager::getInt(setting, group))
        Settings::Manager::setInt(setting, group, currentIndex);
}

void Launcher::SettingsPage::loadSettingInt(QSpinBox* spinBox, const std::string& setting, const std::string& group)
{
    int value = Settings::Manager::getInt(setting, group);
    spinBox->setValue(value);
}

void Launcher::SettingsPage::saveSettingInt(QSpinBox* spinBox, const std::string& setting, const std::string& group)
{
    int value = spinBox->value();
    if (value != Settings::Manager::getInt(setting, group))
        Settings::Manager::setInt(setting, group, value);
}

void Launcher::SettingsPage::slotLoadedCellsChanged(QStringList cellNames)
{
    loadCellsForAutocomplete(cellNames);
}

void Launcher::SettingsPage::slotAnimSourcesToggled(bool checked)
{
    weaponSheathingCheckBox->setEnabled(checked);
    shieldSheathingCheckBox->setEnabled(checked);
    if (!checked)
    {
        weaponSheathingCheckBox->setCheckState(Qt::Unchecked);
        shieldSheathingCheckBox->setCheckState(Qt::Unchecked);
    }
}

void Launcher::SettingsPage::slotPostProcessToggled(bool checked)
{
    postprocessTransparentPostpassCheckBox->setEnabled(checked);
    postprocessHDRTimeComboBox->setEnabled(checked);
    postprocessHDRTimeLabel->setEnabled(checked);
}

void Launcher::SettingsPage::slotSkyBlendingToggled(bool checked)
{
    skyBlendingStartComboBox->setEnabled(checked);
    skyBlendingStartLabel->setEnabled(checked);
}
