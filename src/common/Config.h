#pragma once

#include <string>
#include<unordered_map>

/*
 * Config
 *
 * 这是整个项目的配置解析类。
 *
 * FastDFS 中 tracker、storage、client 都依赖配置文件。
 * 所以我们把配置解析放在 src/common 下，表示它是公共模块。
 *
 * 当前支持的配置格式：
 *
 * key = value
 *
 * 例如：
 *
 * port = 22122
 * base_path = ./data/tracker
 * group_name = group1
 *
 * 当前暂不支持：
 * 1. [section] 分组配置
 * 2. include 其他配置文件
 * 3. 同名 key 出现多次
 *
 * 这些能力后面可以继续附着到这个类上，不需要推倒重写。
 */

class Config {
public:
    /*
     * 从指定路径加载配置文件。
     *
     * 返回 true 表示加载成功。
     * 返回 false 表示文件打不开或解析失败。
     */
    bool load(const std::string& filename);

    /*
     * 判断某个 key 是否存在。
     *
     * 例如：
     * conf.has("port")
     */
    bool has(const std::string& key) const;

    /*
     * 读取字符串配置。
     *
     * 如果 key 存在，返回配置文件中的值。
     * 如果 key 不存在，返回 default_value。
     */
    std::string getString(const std::string& key, const std::string& default_value = "") const;

     /*
     * 读取 int 配置。
     *
     * 例如：
     * port = 22122
     *
     * conf.getInt("port", 0) 会返回 22122。
     */
    int getInt(const std::string& key, int default_value = 0) const;

    /*
     * 读取 bool 配置。
     *
     * 支持：
     * true / false
     * yes / no
     * 1 / 0
     */
    bool getBool(const std::string& key, bool default_value = false) const;

private:
     /*
     * 去掉字符串左右两边的空白字符。
     *
     * 例如：
     * "  port  " 变成 "port"
     */
    static std::string trim(const std::string& text);

private:
    /*
     * 保存解析出来的配置项。
     *
     * key 是配置名，例如 port。
     * value 是配置值，例如 22122。
     */
    std::unordered_map<std::string, std::string> items_;
 };