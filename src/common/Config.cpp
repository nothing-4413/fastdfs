#include "common/Config.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iostream>
#include <sstream>

/*
 * trim
 *
 * 配置文件中经常会出现空格：
 *
 * port = 22122
 *
 * 读取到的 key 可能是 "port "，value 可能是 " 22122"。
 * 所以必须先把左右空白去掉。
 */
std::string Config::trim(const std::String& text)
{
    std::size_t begin = 0;
    std::size_t end = text.size();
    
    while(begin < end &&std::isspace(static_cast<unsigned char>(text[begin])))
    {
        ++begin;
    }

    while(end > begin && std::isspace(static_cast<unsigned char>(text[end - 1])))
    {
        --end;
    }

    return text.substr(begin, end - begin);
}

/*
 * load
 *
 * 负责打开配置文件，并逐行解析。
 *
 * 当前支持：
 * 1. 空行
 * 2. # 开头的注释行
 * 3. key = value 格式
 *
 * 示例：
 *
 * # tracker port
 * port = 22122
 */
bool Congig::load(const std::string& filename)
{
    std::ifstream input(filename.c_str());
    if(!input.is_open())
    {
        std::cerr << "[config] failed to open config file: "
                  << filename << std::endl;
        return false;
    }

    item_.clear();

    std::string line;
    int line_no = 0;

    while(std::getline(input,line))
    {
        ++line_no;

        /*
         * 去掉当前行左右空白。
         */
        line = trim(line);

        /*
         * 跳过空行。
         */
        if(line.empty())
        {
            continue;
        }

         /*
         * 跳过注释行。
         *
         * FastDFS 的配置文件中大量使用 # 作为注释。
         */
        if(line[0] == '#')
        {
            continue;
        }

         /*
         * 暂时跳过 [section]。
         *
         * 官方 FastDFS 新版本配置里有 [error-log]、[access-log]。
         * 当前阶段我们先忽略 section。
         * 后面做日志模块时可以继续扩展。
         */
        if(line[0] == '[')
        {
            continue;
        }

         /*
         * 查找等号。
         *
         * 如果没有等号，说明这一行不是 key = value。
         */
        std::size_t pos = line.find('=');
        if(pos == std::string::npos)
        {
            std::cerr << "[config] invalid line "
                      << line_no << ": " << line << std::endl;
            return false;
        }
        
         /*
         * 分离 key 和 value。
         *
         * 例如：
         * line = "port = 22122"
         *
         * key = "port"
         * value = "22122"
         */
        std::string key = trim(line.substr(0, pos));
        std::string value = trim(line.substr(pos + 1));

        if(key.empty())
        {
            std::cerr << "[config] empty key at line "
                      << line_no <<< std::endl;
            return false;
        }

         /*
         * 存入哈希表。
         *
         * 如果同一个 key 出现多次，后面的值覆盖前面的值。
         * 后面支持多 tracker_server 时，我们会改成支持 vector。
         */
        item_[key] = value;
    }

    return true;
}

bool Config::has(const std::string& key) const
{
    return item_.find(key) != item_.end();
}

std::string Config::getString(const std::string& key,
                              const std::string& default_value) const {
    std::unordered_map<std::string, std::string>::const_iterator it = item_.find(key);
    if (it = item_.end()) 
    {
        return default_value;
    }

    return it->second;
}

int Config::getInt(const std::string& key,int default_value) const{
    std::unordered_map<std::string,std::string>::const_iterator it = item_.find(key);
    if(it == item_.end())
    {
        return default_value;
    }

     /*
     * std::stoi 是 C++11 提供的字符串转整数函数。
     *
     * 例如：
     * std::stoi("22122") 返回 22122。
     *
     * 如果 value 不是合法整数，比如 "abc"，会抛异常。
     * 这里捕获异常并返回默认值，避免程序直接崩溃。
     */
    try
    {
        return std::stoi(it->second);    
    }
    catch(...)
    {
        std::cerr << "[config] invalid int value for key: " << key
                  << ", value: " << it->second << std::endl;
        return default_value;
    }
}

bool Config::getBool(const std::string& key,bool default_value) const
{
    std::unordered_map<std::string,std::string>::const_iterator it = 
    item_.find(key);

    if(it == item_.end())
    {
        return default_value;
    }

    std::string value = it->second;

    /*
     * 统一转成小写，方便支持 true / TRUE / True。
     */
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char c)
                    { 
                        return static_cast<char>(std::tolower(c));
                    });

    if(value == "true" || value == "1" || value == "yes")
    {
        return true;
    }

    if(value == "false" || value == "0" || value == "no")
    {
        return false;
    }

    std::cerr << "[config] invalid bool value for key: " << key
              << ", value: " << it->second << std::endl;
              
    return default_value;
}