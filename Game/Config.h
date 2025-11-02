#pragma once
#include <fstream>
#include <nlohmann/json.hpp>
using json = nlohmann::json;

#include "../Models/Project_path.h"

class Config
{
  public:
    Config()
    {
        reload();
    }

    void reload()
    {
        std::ifstream fin(project_path + "settings.json"); // открывает файл "settings.json" по указанному пути
        fin >> config; // считывает все что написано в файле "settings.json" 
        fin.close();   // закрывает файл "settings.json" 
    }

    auto operator()(const string &setting_dir, const string &setting_name) const
    {
        return config[setting_dir][setting_name]; // идет пара ключ значение setting_dir - ключ, setting_name - значение. сделано чтобы считать все что есть в файле "settings.json" 
    }

  private:
    json config;
};
