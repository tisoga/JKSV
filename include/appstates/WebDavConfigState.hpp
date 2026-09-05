#pragma once
#include "StateManager.hpp"
#include "appstates/BaseState.hpp"
#include "ui/ui.hpp"

#include <memory>
#include <string>

class WebDavConfigState final : public BaseState
{
    public:
        /// @brief Constructor.
        WebDavConfigState();

        /// @brief Creates and returns a new WebDavConfigState instance.
        static inline std::shared_ptr<WebDavConfigState> create() { return std::make_shared<WebDavConfigState>(); }

        /// @brief Creates, pushes to StateManager, and returns a new WebDavConfigState instance.
        static inline std::shared_ptr<WebDavConfigState> create_and_push()
        {
            auto newState = WebDavConfigState::create();
            StateManager::push_state(newState);
            return newState;
        }

        /// @brief Update override.
        void update() override;

        /// @brief Render override.
        void render() override;

    private:
        /// @brief Current WebDAV origin / server URL.
        std::string m_origin{};

        /// @brief Current WebDAV base path.
        std::string m_basepath{};

        /// @brief Current WebDAV username.
        std::string m_username{};

        /// @brief Current WebDAV password.
        std::string m_password{};

        /// @brief Menu displaying the options and values.
        std::shared_ptr<ui::Menu> m_menu{};

        /// @brief Slide-out panel for rendering the menu.
        static inline std::unique_ptr<ui::SlideOutPanel> sm_slidePanel{};

        /// @brief Initializes static members if needed.
        void initialize_static_members();

        /// @brief Loads existing config from webdav.json if present.
        void load_config();

        /// @brief Initializes the UI menu.
        void initialize_menu();

        /// @brief Refreshes menu options with current values.
        void refresh_menu();

        /// @brief Keyboard prompt to edit origin URL.
        void edit_origin();

        /// @brief Keyboard prompt to edit basepath.
        void edit_basepath();

        /// @brief Keyboard prompt to edit username.
        void edit_username();

        /// @brief Keyboard prompt to edit password.
        void edit_password();

        /// @brief Saves the current settings to webdav.json and attempts connection.
        void save_and_connect();

        /// @brief Deletes webdav.json and clears the settings.
        void delete_config();

        /// @brief Cleans up and deactivates state.
        void deactivate_state();
};
