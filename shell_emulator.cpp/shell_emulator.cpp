#include <iostream>
#include <string>
#include <fstream>
#include <filesystem>
#include <cstdlib>
#include <libtar.h>  // Библиотека для работы с tar-архивами
#include <tinyxml2.h> // Для создания xml логов
#include <vector>
#include <sstream>

namespace fs = std::filesystem;
using namespace tinyxml2;

// Функция для создания лог-файла
void logAction(const std::string& username, const std::string& command, const std::string& logPath) {
    XMLDocument doc;
    XMLElement* root;
    if (fs::exists(logPath)) {
        doc.LoadFile(logPath.c_str());
        root = doc.RootElement();
    }
    else {
        root = doc.NewElement("log");
        doc.InsertFirstChild(root);
    }

    XMLElement* action = doc.NewElement("action");
    action->SetAttribute("user", username.c_str());
    action->SetAttribute("command", command.c_str());
    root->InsertEndChild(action);

    doc.SaveFile(logPath.c_str());
}

// Функция для команды ls
void listDirectory(const std::string& currentPath) {
    for (const auto& entry : fs::directory_iterator(currentPath)) {
        std::cout << entry.path().filename().string() << std::endl;
    }
}

// Функция для команды cd
void changeDirectory(std::string& currentPath, const std::string& newPath) {
    fs::path newDirectory = currentPath / newPath;
    if (fs::exists(newDirectory) && fs::is_directory(newDirectory)) {
        currentPath = newDirectory.string();
        fs::current_path(currentPath);
    }
    else {
        std::cerr << "Directory does not exist: " << newDirectory.string() << std::endl;
    }
}

// Функция для команды rm
void removeFileOrDirectory(const std::string& path) {
    if (fs::exists(path)) {
        fs::remove_all(path);
        std::cout << "Removed: " << path << std::endl;
    }
    else {
        std::cerr << "File or directory does not exist: " << path << std::endl;
    }
}

// Функция для команды tree
void printTree(const fs::path& dirPath, const std::string& prefix = "") {
    for (const auto& entry : fs::directory_iterator(dirPath)) {
        std::cout << prefix << "|-- " << entry.path().filename().string() << std::endl;
        if (fs::is_directory(entry)) {
            printTree(entry.path(), prefix + "|   ");
        }
    }
}

// Основной цикл для эмулятора shell
int main(int argc, char* argv[]) {
    if (argc < 4) {
        std::cerr << "Usage: ./shell_emulator <username> <filesystem.tar> <log.xml>" << std::endl;
        return 1;
    }

    std::string username = argv[1];
    std::string tarPath = argv[2];
    std::string logPath = argv[3];
    std::string currentPath = fs::temp_directory_path().string(); // Временная директория для работы

    // Логируем запуск эмулятора
    logAction(username, "start emulator", logPath);

    // Монтирование tar-архива как виртуальной файловой системы
    // Пример с использованием libtar: вы должны установить libtar через пакетный менеджер
    TAR* tar;
    if (tar_open(&tar, tarPath.c_str(), NULL, O_RDONLY, 0, TAR_GNU) == -1) {
        std::cerr << "Error opening tar file: " << tarPath << std::endl;
        return 1;
    }

    if (tar_extract_all(tar, currentPath.c_str()) != 0) {
        std::cerr << "Error extracting tar file." << std::endl;
        return 1;
    }
    tar_close(tar);

    // Основной цикл CLI
    std::string command;
    while (true) {
        std::cout << username << "@virtual_shell:" << currentPath << "$ ";
        std::getline(std::cin, command);
        std::istringstream iss(command);
        std::string cmd;
        std::vector<std::string> args;
        while (iss >> cmd) {
            args.push_back(cmd);
        }

        if (args.empty()) continue;

        if (args[0] == "exit") {
            logAction(username, "exit", logPath);
            break;
        }
        else if (args[0] == "ls") {
            logAction(username, "ls", logPath);
            listDirectory(currentPath);
        }
        else if (args[0] == "cd") {
            if (args.size() > 1) {
                logAction(username, "cd " + args[1], logPath);
                changeDirectory(currentPath, args[1]);
            }
            else {
                std::cerr << "Usage: cd <directory>" << std::endl;
            }
        }
        else if (args[0] == "rm") {
            if (args.size() > 1) {
                logAction(username, "rm " + args[1], logPath);
                removeFileOrDirectory(currentPath + "/" + args[1]);
            }
            else {
                std::cerr << "Usage: rm <file/directory>" << std::endl;
            }
        }
        else if (args[0] == "tree") {
            logAction(username, "tree", logPath);
            printTree(currentPath);
        }
        else {
            std::cout << "Unknown command: " << args[0] << std::endl;
        }
    }

    return 0;
}
