#include <QApplication>
#include <QMessageBox>
#include "main_window.hpp"
#include "config_manager.hpp"
#include "database_manager.hpp"
#include "telegram_bot.hpp"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    try {
        ConfigManager config;
        DatabaseManager db(config.get_db_path());

        auto telegramBot = std::make_unique<TelegramBot>(config, db);
        MainWindow window(config, db);

        QObject::connect(
            telegramBot.get(), 
            &TelegramBot::chatIdRegistered,
            &window, 
            &MainWindow::handleChatIdRegistered
        );

        window.show();
        return app.exec();

    } catch (const std::exception& e) {
        QMessageBox::critical(nullptr, "Fatal Error", e.what());
        return 1;
    } catch (...) {
        QMessageBox::critical(nullptr, "Error", "Unknown error occurred");
        return 2;
    }
}