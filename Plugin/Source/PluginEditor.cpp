/*
 * Copyright (c) 2020 Andreas Pohl
 * Licensed under MIT (https://github.com/apohl79/audiogridder/blob/master/COPYING)
 *
 * Author: Andreas Pohl
 */

#include "PluginEditor.hpp"
#include "Images.hpp"
#include "NewServerWindow.hpp"
#include "PluginProcessor.hpp"
#include "NumberConversion.hpp"
#include "Version.hpp"

using namespace e47;

AudioGridderAudioProcessorEditor::AudioGridderAudioProcessorEditor(AudioGridderAudioProcessor& p)
    : AudioProcessorEditor(&p), m_processor(p), m_newPluginButton("", "newPlug", false) {
    auto& lf = getLookAndFeel();
    lf.setUsingNativeAlertWindows(true);
    lf.setColour(ResizableWindow::backgroundColourId, Colour(DEFAULT_BG_COLOR));
    lf.setColour(PopupMenu::backgroundColourId, Colour(DEFAULT_BG_COLOR));
    lf.setColour(TextEditor::backgroundColourId, Colour(DEFAULT_BUTTON_COLOR));
    lf.setColour(TextButton::buttonColourId, Colour(DEFAULT_BUTTON_COLOR));

    addAndMakeVisible(m_srvIcon);
    m_srvIcon.setImage(ImageCache::getFromMemory(Images::server_png, Images::server_pngSize));
    m_srvIcon.setAlpha(0.5);
    m_srvIcon.setBounds(5, 5, 20, 20);
    m_srvIcon.addMouseListener(this, true);

    addAndMakeVisible(m_srvLabel);
    m_srvLabel.setText("not connected", NotificationType::dontSendNotification);

    setConnected(m_processor.getClient().isReadyLockFree());

    m_srvLabel.setBounds(30, 5, 170, 20);
    auto font = m_srvLabel.getFont();
    font.setHeight(font.getHeight() - 2);
    m_srvLabel.setFont(font);

    addAndMakeVisible(m_versionLabel);
    String v = "Version ";
    v << AUDIOGRIDDER_VERSION;
    m_versionLabel.setText(v, NotificationType::dontSendNotification);
    m_versionLabel.setBounds(0, 89, 190, 10);
    m_versionLabel.setFont(Font(10, Font::plain));
    m_versionLabel.setAlpha(0.4f);

    addAndMakeVisible(m_newPluginButton);
    m_newPluginButton.setButtonText("+");
    m_newPluginButton.setOnClickWithModListener(this);
    addAndMakeVisible(m_pluginScreen);
    m_pluginScreen.setBounds(200, SCREENTOOLS_HEIGHT + SCREENTOOLS_MARGIN * 2, 1, 1);
    m_pluginScreen.setWantsKeyboardFocus(true);
    m_pluginScreen.addMouseListener(&m_processor.getClient(), true);
    m_pluginScreen.addKeyListener(&m_processor.getClient());

    int idx = 0;
    for (auto& plug : m_processor.getLoadedPlugins()) {
        auto* b = addPluginButton(plug.id, plug.name);
        if (!plug.ok) {
            b->setEnabled(false);
        }
        if (plug.bypassed) {
            b->setButtonText("( " + m_processor.getLoadedPlugin(idx).name + " )");
            b->setColour(PluginButton::textColourOffId, Colours::grey);
        }
#if JucePlugin_IsSynth
        m_newPluginButton.setEnabled(false);
#endif
        idx++;
    }

    auto active = m_processor.getActivePlugin();
    if (active > -1) {
        m_pluginButtons[as<size_t>(active)]->setActive(true);
    }

    m_stPlus.setButtonText("+");
    m_stPlus.setBounds(201, 1, 1, 1);
    m_stPlus.setColour(ComboBox::outlineColourId, Colour(DEFAULT_BUTTON_COLOR));
    m_stPlus.setConnectedEdges(Button::ConnectedOnLeft | Button::ConnectedOnRight | Button::ConnectedOnTop |
                               Button::ConnectedOnBottom);
    m_stPlus.addListener(this);
    addAndMakeVisible(&m_stPlus);
    m_stMinus.setButtonText("-");
    m_stMinus.setBounds(201, 1, 1, 1);
    m_stMinus.setColour(ComboBox::outlineColourId, Colour(DEFAULT_BUTTON_COLOR));
    m_stMinus.setConnectedEdges(Button::ConnectedOnLeft | Button::ConnectedOnRight | Button::ConnectedOnTop |
                                Button::ConnectedOnBottom);
    m_stMinus.addListener(this);
    addAndMakeVisible(&m_stMinus);

    m_stA.setButtonText("A");
    m_stA.setBounds(201, 1, 1, 1);
    m_stA.setConnectedEdges(Button::ConnectedOnLeft | Button::ConnectedOnRight | Button::ConnectedOnTop |
                            Button::ConnectedOnBottom);
    m_stA.addListener(this);
    addAndMakeVisible(&m_stA);

    m_stB.setButtonText("B");
    m_stB.setBounds(201, 1, 1, 1);
    m_stB.setConnectedEdges(Button::ConnectedOnLeft | Button::ConnectedOnRight | Button::ConnectedOnTop |
                            Button::ConnectedOnBottom);
    m_stB.addListener(this);
    addAndMakeVisible(&m_stB);

    initStButtons();

    setSize(200, 100);
}

AudioGridderAudioProcessorEditor::~AudioGridderAudioProcessorEditor() {
    m_processor.hidePlugin();
    m_processor.getClient().setPluginScreenUpdateCallback(nullptr);
}

void AudioGridderAudioProcessorEditor::paint(Graphics& g) {
    g.fillAll(getLookAndFeel().findColour(ResizableWindow::backgroundColourId));
}

void AudioGridderAudioProcessorEditor::resized() {
    int buttonWidth = 197;
    int buttonHeight = 20;
    int num = 0;
    int top = 30;
    for (auto& b : m_pluginButtons) {
        b->setBounds(1, top, buttonWidth, buttonHeight);
        top += buttonHeight + 2;
        num++;
    }
    m_newPluginButton.setBounds(1, top, buttonWidth, buttonHeight);
    top += buttonHeight + 13;
    int windowHeight = jmax(100, top);
    int leftBarWidth = 200;
    int windowWidth = leftBarWidth;
    if (m_processor.getActivePlugin() != -1) {
        int screenHeight = m_pluginScreen.getHeight() + SCREENTOOLS_HEIGHT;
        windowHeight = jmax(windowHeight, screenHeight);
        windowWidth += m_pluginScreen.getWidth();
        m_stMinus.setBounds(windowWidth - SCREENTOOLS_HEIGHT - SCREENTOOLS_MARGIN * 2, SCREENTOOLS_MARGIN,
                            SCREENTOOLS_HEIGHT, SCREENTOOLS_HEIGHT);
        m_stPlus.setBounds(windowWidth - SCREENTOOLS_HEIGHT * 2 - SCREENTOOLS_MARGIN * 3, SCREENTOOLS_MARGIN,
                           SCREENTOOLS_HEIGHT, SCREENTOOLS_HEIGHT);
        m_stA.setBounds(leftBarWidth + SCREENTOOLS_MARGIN, SCREENTOOLS_MARGIN, SCREENTOOLS_AB_WIDTH,
                        SCREENTOOLS_HEIGHT);
        m_stB.setBounds(leftBarWidth + SCREENTOOLS_MARGIN + SCREENTOOLS_AB_WIDTH, SCREENTOOLS_MARGIN,
                        SCREENTOOLS_AB_WIDTH, SCREENTOOLS_HEIGHT);
        if (m_currentActiveAB != m_processor.getActivePlugin()) {
            initStButtons();
        }
    }
    if (getWidth() != windowWidth || getHeight() != windowHeight) {
        setSize(windowWidth, windowHeight);
    }
    m_versionLabel.setBounds(0, windowHeight - 11, m_versionLabel.getWidth(), m_versionLabel.getHeight());
}

void AudioGridderAudioProcessorEditor::buttonClicked(Button* button, const ModifierKeys& modifiers) {
    if (!button->getName().compare("newPlug")) {
        auto addFn = [this](const ServerPlugin& plug) {
            if (m_processor.loadPlugin(plug.getId(), plug.getName())) {
                addPluginButton(plug.getId(), plug.getName());
#if JucePlugin_IsSynth
                m_newPluginButton.setEnabled(false);
#endif
                resized();
            } else {
                AlertWindow::showMessageBoxAsync(AlertWindow::WarningIcon, "Error",
                                                 "Failed to add " + plug.getName() + " plugin!", "OK");
            }
        };
        PopupMenu m;
        auto recents = m_processor.getClient().getRecents();
        for (const auto& plug : recents) {
            m.addItem(plug.getName(), [addFn, plug] { addFn(plug); });
        }
        if (recents.size() > 0) {
            m.addSeparator();
        }
        // create type menu
        std::map<String, PopupMenu> typeMenus;
        for (const auto& type : m_processor.getPluginTypes()) {
            PopupMenu& sub = typeMenus[type];
            std::map<String, PopupMenu> companyMenus;
            for (const auto& plug : m_processor.getPlugins(type)) {
                auto& menu = companyMenus[plug.getCompany()];
                menu.addItem(plug.getName(), [addFn, plug] { addFn(plug); });
            }
            for (auto& company : companyMenus) {
                sub.addSubMenu(company.first, company.second);
            }
        }
        if (typeMenus.size() > 1) {
            for (auto& sub : typeMenus) {
                m.addSubMenu(sub.first, sub.second);
            }
        } else if (typeMenus.size() > 0) {
            // skip the type menu if there is only one type
            for (auto& tm : typeMenus) {
                PopupMenu::MenuItemIterator it(tm.second);
                while (it.next()) {
                    auto& item = it.getItem();
                    m.addSubMenu(item.text, *item.subMenu);
                }
            }
        }
        m.showAt(button);
    } else {
        int idx = getPluginIndex(button->getName());
        int active = m_processor.getActivePlugin();
        PluginButton::AreaType area = m_pluginButtons[as<size_t>(idx)]->getAreaType();
        auto editFn = [this, idx, active] {
            if (m_processor.isBypassed(idx)) {
                return;
            }
            m_processor.editPlugin(idx);
            m_pluginButtons[as<size_t>(idx)]->setActive(true);
            m_pluginButtons[as<size_t>(idx)]->setColour(PluginButton::textColourOffId, Colours::yellow);
            auto* p_processor = &m_processor;
            m_processor.getClient().setPluginScreenUpdateCallback(
                [this, idx, p_processor](std::shared_ptr<Image> img, int width, int height) {
                    if (nullptr != img) {
                        MessageManager::callAsync([this, p_processor, img, width, height] {
                            auto p = dynamic_cast<AudioGridderAudioProcessorEditor*>(p_processor->getActiveEditor());
                            if (this == p) {  // make sure the editor hasn't been closed
                                m_pluginScreen.setSize(width, height);
                                m_pluginScreen.setImage(img->createCopy());
                                resized();
                            }
                        });
                    } else {
                        MessageManager::callAsync([this, idx, p_processor] {
                            auto p = dynamic_cast<AudioGridderAudioProcessorEditor*>(p_processor->getActiveEditor());
                            if (this == p && m_pluginButtons.size() > as<size_t>(idx)) {
                                m_processor.hidePlugin(false);
                                m_pluginButtons[as<size_t>(idx)]->setActive(false);
                                resized();
                            }
                        });
                    }
                });
            if (active > -1 && as<size_t>(active) < m_pluginButtons.size()) {
                m_pluginButtons[as<size_t>(active)]->setActive(false);
                m_pluginButtons[as<size_t>(active)]->setColour(PluginButton::textColourOffId, Colours::white);
                resized();
            }
        };
        auto hideFn = [this, idx](int i = -1) {
            m_processor.hidePlugin();
            size_t index = i > -1 ? as<size_t>(i) : as<size_t>(idx);
            m_pluginButtons[index]->setActive(false);
            m_pluginButtons[index]->setColour(PluginButton::textColourOffId, Colours::white);
            resized();
        };
        auto bypassFn = [this, idx, button] {
            m_processor.bypassPlugin(idx);
            button->setButtonText("( " + m_processor.getLoadedPlugin(idx).name + " )");
            button->setColour(PluginButton::textColourOffId, Colours::grey);
        };
        auto unBypassFn = [this, idx, active, button] {
            m_processor.unbypassPlugin(idx);
            button->setButtonText(m_processor.getLoadedPlugin(idx).name);
            if (idx == active) {
                button->setColour(PluginButton::textColourOffId, Colours::yellow);
            } else {
                button->setColour(PluginButton::textColourOffId, Colours::white);
            }
        };
        auto moveUpFn = [this, idx] {
            if (idx > 0) {
                m_processor.exchangePlugins(idx, idx - 1);
                std::swap(m_pluginButtons[as<size_t>(idx)], m_pluginButtons[as<size_t>(idx) - 1]);
                resized();
            }
        };
        auto moveDownFn = [this, idx] {
            if (as<size_t>(idx) < m_pluginButtons.size() - 1) {
                m_processor.exchangePlugins(idx, idx + 1);
                std::swap(m_pluginButtons[as<size_t>(idx)], m_pluginButtons[as<size_t>(idx) + 1]);
                resized();
            }
        };
        auto deleteFn = [this, idx] {
            m_processor.unloadPlugin(idx);
            int i = 0;
            for (auto it = m_pluginButtons.begin(); it < m_pluginButtons.end(); it++) {
                if (i++ == idx) {
                    m_pluginButtons.erase(it);
                    break;
                }
            }
#if JucePlugin_IsSynth
            m_newPluginButton.setEnabled(true);
#endif
            resized();
        };
        if (modifiers.isLeftButtonDown()) {
            switch (area) {
                case PluginButton::MAIN: {
                    bool edit = idx != active;
                    if (active > -1) {
                        hideFn(active);
                    }
                    if (edit) {
                        editFn();
                    }
                    break;
                }
                case PluginButton::BYPASS:
                    if (m_processor.isBypassed(idx)) {
                        unBypassFn();
                    } else {
                        bypassFn();
                    }
                    break;
                case PluginButton::MOVE_DOWN:
                    moveDownFn();
                    break;
                case PluginButton::MOVE_UP:
                    moveUpFn();
                    break;
                case PluginButton::DELETE:
                    deleteFn();
                    break;
                default:
                    break;
            }
        } else {
            PopupMenu m;
            if (m_processor.isBypassed(idx)) {
                m.addItem("Unbypass", unBypassFn);
            } else {
                m.addItem("Bypass", bypassFn);
            }
            m.addItem("Edit", editFn);
            m.addItem("Hide", idx == m_processor.getActivePlugin(), false, hideFn);
            m.addSeparator();
            m.addItem("Move Up", idx > 0, false, moveUpFn);
            m.addItem("Move Down", as<size_t>(idx) < m_pluginButtons.size() - 1, false, moveDownFn);
            m.addSeparator();
            m.addItem("Delete", deleteFn);
            m.addSeparator();
            PopupMenu presets;
            int preset = 0;
            for (auto& p : m_processor.getLoadedPlugin(idx).presets) {
                presets.addItem(p, [this, idx, preset] { m_processor.getClient().setPreset(idx, preset); });
                preset++;
            }
            m.addSubMenu("Presets", presets);
            m.addSeparator();
            PopupMenu params;
            for (auto& p : m_processor.getLoadedPlugin(idx).params) {
                int paramIdx = p.idx;
                String name = p.name;
                bool enabled = false;
                if (p.automationSlot > -1) {
                    name << " -> [" << p.automationSlot << "]";
                    enabled = true;
                }
                params.addItem(name, true, enabled, [this, idx, paramIdx, enabled] {
                    if (enabled) {
                        m_processor.disableParamAutomation(idx, paramIdx);
                    } else {
                        m_processor.enableParamAutomation(idx, paramIdx);
                    }
                });
            }
            m.addSubMenu("Automation", params);
            m.showAt(button);
        }
    }
}

void AudioGridderAudioProcessorEditor::buttonClicked(Button* button) {
    TextButton* tb = reinterpret_cast<TextButton*>(button);
    if (tb == &m_stPlus) {
        m_processor.increaseSCArea();
    } else if (tb == &m_stMinus) {
        m_processor.decreaseSCArea();
    } else if (tb == &m_stA || tb == &m_stB) {
        m_currentActiveAB = m_processor.getActivePlugin();
        if (isHilightedStButton(&m_stB)) {
            m_processor.storeSettingsB();
            m_processor.restoreSettingsA();
            hilightStButton(&m_stA);
            enableStButton(&m_stB);
        } else {
            m_processor.storeSettingsA();
            m_processor.restoreSettingsB();
            hilightStButton(&m_stB);
            enableStButton(&m_stA);
        }
    }
}

Button* AudioGridderAudioProcessorEditor::addPluginButton(const String& id, const String& name) {
    int num = 0;
    for (auto& plug : m_pluginButtons) {
        if (!id.compare(plug->getPluginId())) {
            num++;
        }
    }
    String suffix;
    if (num > 0) {
        suffix << " (" << num + 1 << ")";
    }
    auto but = std::make_unique<PluginButton>(id, name + suffix);
    auto* ret = but.get();
    but->setOnClickWithModListener(this);
    addAndMakeVisible(*but);
    m_pluginButtons.push_back(std::move(but));
    return ret;
}

std::vector<Button*> AudioGridderAudioProcessorEditor::getPluginButtons(const String& id) {
    std::vector<Button*> ret;
    for (auto& b : m_pluginButtons) {
        if (b->getPluginId() == id) {
            ret.push_back(b.get());
        }
    }
    return ret;
}

int AudioGridderAudioProcessorEditor::getPluginIndex(const String& name) {
    int idx = 0;
    for (auto& plug : m_pluginButtons) {
        if (!name.compare(plug->getName())) {
            return idx;
        }
        idx++;
    }
    return -1;
}

void AudioGridderAudioProcessorEditor::focusOfChildComponentChanged(FocusChangeType /* cause */) {
    if (hasKeyboardFocus(true)) {
        // reactivate the plugin screen
        int active = m_processor.getActivePlugin();
        if (active > -1) {
            m_processor.editPlugin(active);
        }
    }
}

void AudioGridderAudioProcessorEditor::setConnected(bool connected) {
    m_connected = connected;
    if (connected) {
        String srvTxt = m_processor.getActiveServerName();
        srvTxt << " (+" << m_processor.getLatencyMillis() << "ms)";
        m_srvLabel.setText(srvTxt, NotificationType::dontSendNotification);
        auto& plugins = m_processor.getLoadedPlugins();
        for (size_t i = 0; i < m_pluginButtons.size(); i++) {
            m_pluginButtons[i]->setEnabled(plugins[i].ok);
        }
    } else {
        m_srvLabel.setText("not connected", NotificationType::dontSendNotification);
        for (auto& but : m_pluginButtons) {
            but->setEnabled(false);
        }
    }
}

void AudioGridderAudioProcessorEditor::mouseUp(const MouseEvent& event) {
    if (!m_srvIcon.contains(event.getMouseDownPosition())) {
        return;
    }
    PopupMenu m;
    m.addSectionHeader("Buffering");
    PopupMenu bufMenu;
    int rate = as<int>(lround(m_processor.getSampleRate()));
    int iobuf = m_processor.getBlockSize();
    auto getName = [rate, iobuf](int blocks) -> String {
        String n;
        n << blocks << " Blocks (+" << blocks * iobuf * 1000 / rate << "ms)";
        return n;
    };
    bufMenu.addItem("Disabled (+0ms)", true, m_processor.getClient().NUM_OF_BUFFERS == 0,
                    [this] { m_processor.saveConfig(0); });
    bufMenu.addItem(getName(2), true, m_processor.getClient().NUM_OF_BUFFERS == 2,
                    [this] { m_processor.saveConfig(2); });
    bufMenu.addItem(getName(4), true, m_processor.getClient().NUM_OF_BUFFERS == 4,
                    [this] { m_processor.saveConfig(4); });
    bufMenu.addItem(getName(8), true, m_processor.getClient().NUM_OF_BUFFERS == 8,
                    [this] { m_processor.saveConfig(8); });
    bufMenu.addItem(getName(12), true, m_processor.getClient().NUM_OF_BUFFERS == 12,
                    [this] { m_processor.saveConfig(12); });
    bufMenu.addItem(getName(16), true, m_processor.getClient().NUM_OF_BUFFERS == 16,
                    [this] { m_processor.saveConfig(16); });
    bufMenu.addItem(getName(20), true, m_processor.getClient().NUM_OF_BUFFERS == 20,
                    [this] { m_processor.saveConfig(20); });
    bufMenu.addItem(getName(24), true, m_processor.getClient().NUM_OF_BUFFERS == 24,
                    [this] { m_processor.saveConfig(24); });
    bufMenu.addItem(getName(28), true, m_processor.getClient().NUM_OF_BUFFERS == 28,
                    [this] { m_processor.saveConfig(28); });
    bufMenu.addItem(getName(30), true, m_processor.getClient().NUM_OF_BUFFERS == 30,
                    [this] { m_processor.saveConfig(30); });
    m.addSubMenu("Buffer Size", bufMenu);
    m.addSectionHeader("Servers");
    auto& servers = m_processor.getServers();
    auto active = m_processor.getActiveServerHost();
    for (auto s : servers) {
        if (s == active) {
            PopupMenu srvMenu;
            srvMenu.addItem("Reconnect", [this] { m_processor.getClient().reconnect(); });
            m.addSubMenu(s, srvMenu, true, nullptr, true, 0);
        } else {
            PopupMenu srvMenu;
            srvMenu.addItem("Connect", [this, s] {
                m_processor.setActiveServer(s);
                m_processor.saveConfig();
            });
            srvMenu.addItem("Remove", [this, s] {
                m_processor.delServer(s);
                m_processor.saveConfig();
            });
            m.addSubMenu(s, srvMenu);
        }
    }
    auto serversMDNS = m_processor.getServersMDNS();
    if (serversMDNS.size() > 0) {
        for (auto s : serversMDNS) {
            if (servers.contains(s.getHostAndID())) {
                continue;
            }
            if (s.getHostAndID() == active) {
                PopupMenu srvMenu;
                srvMenu.addItem("Reconnect", [this] { m_processor.getClient().reconnect(); });
                m.addSubMenu(s.getNameAndID(), srvMenu, true, nullptr, true, 0);
            } else {
                PopupMenu srvMenu;
                srvMenu.addItem("Connect", [this, s] {
                    m_processor.setActiveServer(s);
                    m_processor.saveConfig();
                });
                m.addSubMenu(s.getNameAndID(), srvMenu);
            }
        }
    }
    m.addSeparator();
    m.addItem("Add", [this] {
        auto w = new NewServerWindow(as<float>(getScreenX() + 2), as<float>(getScreenY() + 30));
        w->onOk([this](String server) {
            m_processor.addServer(server);
            m_processor.setActiveServer(server);
            m_processor.saveConfig();
        });
        w->setAlwaysOnTop(true);
        w->runModalLoop();
    });
    m.showAt(&m_srvIcon);
}

void AudioGridderAudioProcessorEditor::initStButtons() {
    enableStButton(&m_stA);
    disableStButton(&m_stB);
    m_processor.resetSettingsAB();
    m_hilightedStButton = nullptr;
}

void AudioGridderAudioProcessorEditor::enableStButton(TextButton* b) {
    b->setColour(PluginButton::textColourOffId, Colours::white);
    b->setColour(ComboBox::outlineColourId, Colour(DEFAULT_BUTTON_COLOR));
}

void AudioGridderAudioProcessorEditor::disableStButton(TextButton* b) {
    b->setColour(PluginButton::textColourOffId, Colours::grey);
    b->setColour(ComboBox::outlineColourId, Colour(DEFAULT_BUTTON_COLOR));
}

void AudioGridderAudioProcessorEditor::hilightStButton(TextButton* b) {
    b->setColour(PluginButton::textColourOffId, Colours::yellow);
    b->setColour(ComboBox::outlineColourId, Colours::yellow);
    m_hilightedStButton = b;
}

bool AudioGridderAudioProcessorEditor::isHilightedStButton(TextButton* b) { return b == m_hilightedStButton; }
