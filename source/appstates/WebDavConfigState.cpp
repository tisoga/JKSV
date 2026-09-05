#include "appstates/WebDavConfigState.hpp"

#include "StateManager.hpp"
#include "error.hpp"
#include "fslib.hpp"
#include "graphics/screen.hpp"
#include "input.hpp"
#include "json.hpp"
#include "keyboard/keyboard.hpp"
#include "remote/remote.hpp"
#include "stringutil.hpp"
#include "ui/PopMessageManager.hpp"

#include <array>

namespace
{
    enum MenuIndex
    {
        Origin = 0,
        BasePath,
        Username,
        Password,
        SaveConnect,
        DeleteConfig
    };
} // namespace

//                      ---- Construction ----

WebDavConfigState::WebDavConfigState()
    : BaseState()
{
    WebDavConfigState::initialize_static_members();
    WebDavConfigState::load_config();
    WebDavConfigState::initialize_menu();
    WebDavConfigState::refresh_menu();
}

//                      ---- Public functions ----

void WebDavConfigState::update()
{
    const bool hasFocus = BaseState::has_focus();
    const bool aPressed = input::button_pressed(HidNpadButton_A);
    const bool bPressed = input::button_pressed(HidNpadButton_B);

    sm_slidePanel->update(hasFocus);

    if (aPressed)
    {
        switch (m_menu->get_selected())
        {
            case MenuIndex::Origin:       WebDavConfigState::edit_origin(); break;
            case MenuIndex::BasePath:     WebDavConfigState::edit_basepath(); break;
            case MenuIndex::Username:     WebDavConfigState::edit_username(); break;
            case MenuIndex::Password:     WebDavConfigState::edit_password(); break;
            case MenuIndex::SaveConnect:  WebDavConfigState::save_and_connect(); break;
            case MenuIndex::DeleteConfig: WebDavConfigState::delete_config(); break;
            default:                      break;
        }
    }
    else if (sm_slidePanel->is_closed()) { WebDavConfigState::deactivate_state(); }
    else if (bPressed) { sm_slidePanel->close(); }
}

void WebDavConfigState::render()
{
    const bool hasFocus = BaseState::has_focus();
    sm_slidePanel->clear_target();
    sm_slidePanel->render(sdl::Texture::Null, hasFocus);
}

//                      ---- Private functions ----

void WebDavConfigState::initialize_static_members()
{
    if (sm_slidePanel) { return; }
    sm_slidePanel = std::make_unique<ui::SlideOutPanel>(560, ui::SlideOutPanel::Side::Right);
}

void WebDavConfigState::load_config()
{
    if (!fslib::file_exists(remote::PATH_WEBDAV_CONFIG)) { return; }

    json::Object config = json::new_object(json_object_from_file, remote::PATH_WEBDAV_CONFIG.data());
    if (!config) { return; }

    json_object *origin   = json::get_object(config, "origin");
    json_object *basepath = json::get_object(config, "basepath");
    json_object *username = json::get_object(config, "username");
    json_object *password = json::get_object(config, "password");

    if (origin) { m_origin = json_object_get_string(origin); }
    if (basepath) { m_basepath = json_object_get_string(basepath); }
    if (username) { m_username = json_object_get_string(username); }
    if (password) { m_password = json_object_get_string(password); }
}

void WebDavConfigState::initialize_menu()
{
    m_menu = std::make_shared<ui::Menu>(8, 8, 540, 22, graphics::SCREEN_HEIGHT);
    sm_slidePanel->push_new_element(m_menu);
}

void WebDavConfigState::refresh_menu()
{
    const int selected = m_menu->is_empty() ? 0 : m_menu->get_selected();
    m_menu->reset(false);

    const std::string urlOpt      = stringutil::get_formatted_string("URL: %s", m_origin.empty() ? "(Not Set)" : m_origin.c_str());
    const std::string basePathOpt = stringutil::get_formatted_string("Basepath: %s", m_basepath.empty() ? "JKSV" : m_basepath.c_str());
    const std::string userOpt     = stringutil::get_formatted_string("Username: %s", m_username.empty() ? "(Not Set)" : m_username.c_str());
    const std::string passOpt     = stringutil::get_formatted_string("Password: %s", m_password.empty() ? "(Not Set)" : "********");

    m_menu->add_option(urlOpt);
    m_menu->add_option(basePathOpt);
    m_menu->add_option(userOpt);
    m_menu->add_option(passOpt);
    m_menu->add_option("Save and Connect");
    m_menu->add_option("Delete Configuration");

    m_menu->set_selected(selected);
}

void WebDavConfigState::edit_origin()
{
    std::array<char, 256> buffer = {0};
    const std::string defaultText = m_origin.empty() ? "https://" : m_origin;
    if (keyboard::get_input(SwkbdType_Normal, defaultText, "WebDAV Server URL (e.g. https://domain.com:8080)", buffer.data(), buffer.size()))
    {
        std::string result{buffer.data()};
        while (!result.empty() && result.back() == '/') { result.pop_back(); }
        m_origin = result;
        WebDavConfigState::refresh_menu();
    }
}

void WebDavConfigState::edit_basepath()
{
    std::array<char, 128> buffer = {0};
    const std::string defaultText = m_basepath.empty() ? "JKSV" : m_basepath;
    if (keyboard::get_input(SwkbdType_Normal, defaultText, "WebDAV Basepath (e.g. JKSV)", buffer.data(), buffer.size()))
    {
        std::string result{buffer.data()};
        while (!result.empty() && result.front() == '/') { result.erase(0, 1); }
        while (!result.empty() && result.back() == '/') { result.pop_back(); }
        m_basepath = result;
        WebDavConfigState::refresh_menu();
    }
}

void WebDavConfigState::edit_username()
{
    std::array<char, 128> buffer = {0};
    if (keyboard::get_input(SwkbdType_Normal, m_username, "WebDAV Username", buffer.data(), buffer.size()))
    {
        m_username = buffer.data();
        WebDavConfigState::refresh_menu();
    }
}

void WebDavConfigState::edit_password()
{
    std::array<char, 128> buffer = {0};
    if (keyboard::get_input(SwkbdType_Normal, m_password, "WebDAV Password", buffer.data(), buffer.size()))
    {
        m_password = buffer.data();
        WebDavConfigState::refresh_menu();
    }
}

void WebDavConfigState::save_and_connect()
{
    const int popTicks = ui::PopMessageManager::DEFAULT_TICKS;

    if (m_origin.empty())
    {
        ui::PopMessageManager::push_message(popTicks, "WebDAV Error: URL cannot be empty!");
        return;
    }

    if (!fslib::directory_exists("sdmc:/config/JKSV"))
    {
        fslib::create_directory("sdmc:/config/JKSV");
    }

    json::Object configJSON = json::new_object(json_object_new_object);
    if (!configJSON) { return; }

    json::add_object(configJSON, "origin", json_object_new_string(m_origin.c_str()));
    if (!m_basepath.empty())
    {
        json::add_object(configJSON, "basepath", json_object_new_string(m_basepath.c_str()));
    }
    if (!m_username.empty())
    {
        json::add_object(configJSON, "username", json_object_new_string(m_username.c_str()));
    }
    if (!m_password.empty())
    {
        json::add_object(configJSON, "password", json_object_new_string(m_password.c_str()));
    }

    const char *jsonStr      = json_object_to_json_string_ext(configJSON.get(), JSON_C_TO_STRING_PRETTY);
    const int64_t jsonLength = std::char_traits<char>::length(jsonStr);

    fslib::File configFile{remote::PATH_WEBDAV_CONFIG, FsOpenMode_Create | FsOpenMode_Write, jsonLength};
    if (configFile.is_open())
    {
        configFile << jsonStr;
    }

    ui::PopMessageManager::push_message(popTicks, "WebDAV config saved. Connecting...");
    remote::reinitialize_webdav();
}

void WebDavConfigState::delete_config()
{
    const int popTicks = ui::PopMessageManager::DEFAULT_TICKS;
    if (fslib::file_exists(remote::PATH_WEBDAV_CONFIG))
    {
        fslib::delete_file(remote::PATH_WEBDAV_CONFIG);
    }

    m_origin.clear();
    m_basepath.clear();
    m_username.clear();
    m_password.clear();

    remote::reset_storage();
    ui::PopMessageManager::push_message(popTicks, "WebDAV config deleted.");
    WebDavConfigState::refresh_menu();
}

void WebDavConfigState::deactivate_state()
{
    sm_slidePanel->clear_elements();
    sm_slidePanel->reset();
    BaseState::deactivate();
}
